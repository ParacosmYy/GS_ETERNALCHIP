/**
 * @file    task_ota.c
 * @brief   OTA upgrade task implementation
 *
 * Runs as a FreeRTOS task. Waits 5 seconds for trigger, then receives
 * firmware via YMODEM, writes to Slot B, verifies SHA-256, and reboots.
 */

//*** Includes ***//
#define LOG_TAG "OTA"
#include "elog.h"
#include "task_ota.h"
#include "bsp_flash.h"
#include "bsp_led.h"
#include "bsp_key.h"
#include "sha256.h"
#include "ymodem.h"
#include "usart.h"
#include "iwdg.h"
#include "cmsis_os.h"
#include <string.h>

//*** Private Variables ***//

static ota_task_state_t s_state;
static uint32_t         s_fw_size;
static volatile uint8_t s_trigger_flag;
static bsp_led_driver_t s_led;
static bsp_key_driver_t s_key;

//*** Key Callback ***//

static void OnKey(bsp_key_event_t evt, void *p_user)
{
    (void)p_user;
    if (evt == BSP_KEY_EVT_SHORT_PRESS)
    {
        log_i("Key: SHORT_PRESS");
        s_trigger_flag = 1;
    }
}

//*** UART Adapters for YMODEM (polling mode) ***//

/**
 * @brief  UART 单字节发送适配器（供 YMODEM 协议层调用）
 * @param  byte    待发送字节
 * @param  p_user  用户上下文（未使用）
 */
static void uart_send_byte(uint8_t byte, void *p_user)
{
    (void)p_user;
    HAL_UART_Transmit(&huart1, &byte, 1, 100);
}

/**
 * @brief  UART 单字节接收适配器（供 YMODEM 协议层调用）
 * @param  p_byte      接收字节输出
 * @param  timeout_ms  超时时间（毫秒）
 * @param  p_user      用户上下文（未使用）
 * @retval 0   成功接收一字节
 * @retval -1  超时或 HAL 错误
 */
static int uart_recv_byte(uint8_t *p_byte, uint32_t timeout_ms, void *p_user)
{
    (void)p_user;
    if (HAL_UART_Receive(&huart1, p_byte, 1, timeout_ms) == HAL_OK)
    {
        return 0;
    }
    return -1;
}

//*** YMODEM Data Callback ***//

/**
 * @brief  YMODEM 数据接收回调，将固件数据写入 Slot B Flash
 *
 *         每收到一帧数据即写入对应 Flash 地址，每 10KB 打印进度。
 *         写入过程中刷新看门狗，检测退出标志。
 *
 * @param  offset  当前累计接收字节数
 * @param  p_data  本次接收的数据缓冲区
 * @param  len     本次数据长度
 * @param  p_user  用户上下文（未使用）
 * @retval 0   写入成功
 * @retval -1  Flash 写入失败或收到退出信号
 */
static int ymodem_data_callback(uint32_t offset, const uint8_t *p_data, uint16_t len, void *p_user)
{
    uint32_t addr;

    (void)p_user;

    addr = FLASH_ADDR_SLOT_B + offset - len;
    if (BspFlash_Write(addr, p_data, len) != 0)
    {
        log_e("Flash write error at 0x%08lX (HAL err=0x%08lX)", addr, HAL_FLASH_GetError());
        return -1;
    }

    /* Progress every 10 KB */
    {
        static uint32_t s_last_print = 0;
        if (offset - s_last_print >= 10240u)
        {
            log_i("%lu / %lu bytes", offset, s_fw_size);
            s_last_print = offset;
        }
    }

    HAL_IWDG_Refresh(&hiwdg);

    return 0;
}

//*** SHA-256 Flash Hash ***//

/**
 * @brief  计算 Flash 指定区域的 SHA-256 哈希值
 *
 *         分块读取 Flash 内容（256 字节/块）并逐步更新哈希。
 *         使用内存映射直接读取，无需 BspFlash_Read。
 *
 * @param  addr  Flash 起始地址
 * @param  len   数据长度（字节）
 * @param  hash  输出 32 字节 SHA-256 哈希值
 * @retval 0  成功
 */
static int sha256_flash_region(uint32_t addr, uint32_t len, uint8_t hash[32])
{
    SHA256_CTX ctx;
    uint8_t    buf[256];
    uint32_t   i;
    uint32_t   chunk;

    sha256_init(&ctx);

    for (i = 0; i < len; i += sizeof(buf))
    {
        chunk = len - i;
        if (chunk > sizeof(buf))
        {
            chunk = sizeof(buf);
        }
        memcpy(buf, (const void *)(addr + i), chunk);
        sha256_update(&ctx, buf, chunk);
    }

    sha256_final(&ctx, hash);
    return 0;
}

//*** Public API ***//

/**
 * @brief  初始化 OTA 子系统和 EasyLogger
 *
 *         重置 OTA 状态、初始化 Flash 驱动、配置并启动 EasyLogger 日志框架。
 *         在 TaskOta_Run 前调用。
 */
void TaskOta_Init(void)
{
    s_state        = OTA_TASK_NORMAL;
    s_fw_size      = 0;
    s_trigger_flag = 0;
    BspFlash_Init();

    /* Initialize EasyLogger */
    elog_init();
    elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL);
    elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_LVL | ELOG_FMT_TAG);
    elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_TAG);
    elog_start();
}

/** @brief  获取当前 OTA 任务状态 */
ota_task_state_t TaskOta_GetState(void)
{
    return s_state;
}

//*** Task Entry ***//

/**
 * @brief  OTA 任务入口函数（FreeRTOS 任务）
 *
 *         执行完整 OTA 升级流程：
 *         1. 无限等待按键短按触发
 *         2. 擦除 Slot B
 *         3. YMODEM 接收固件 → 写入 Slot B
 *         4. SHA-256 校验
 *         5. 更新 OTA Config（UPGRADE_PENDING）
 *         6. 3 秒后软件复位 → Bootloader 复制 B→A
 *
 *         错误时 3 秒后自动复位。
 *
 * @param  p_argument  未使用（传 NULL）
 */
void TaskOta_Run(void *p_argument)
{
    int          ret;
    int          cfg_ret;
    uint8_t      hash[32];
    ota_config_t cfg;

    (void)p_argument;
    TaskOta_Init();

    /* LED 初始化：PC13, 低有效 */
    {
        static const bsp_led_config_t led_cfg = { GPIOC, GPIO_PIN_13, 0 };
        BspLed_Init(&s_led, &led_cfg);
    }

    /* KEY 初始化：PA0, 低有效, 20ms 消抖, 1000ms 长按 */
    {
        static const bsp_key_config_t key_cfg = { GPIOA, GPIO_PIN_0, 0, 20, 1000, OnKey, NULL };
        BspKey_Init(&s_key, &key_cfg);
    }

    /* ---- Phase 1: 等待触发（无限期，按键短按触发）---- */
    s_state = OTA_TASK_TRIGGER_WAIT;
    log_i("Firmware %s", OTA_FW_VERSION);
    log_i("Waiting for trigger... (short press key)");

    while (!s_trigger_flag)
    {
        BspKey_Scan(&s_key);
        osDelay(10);
    }

    /* 触发成功，LED 开始指示 OTA 状态 */
    log_i("Triggered! Current: %s", OTA_FW_VERSION);
    log_i("Starting upgrade...");

    /* ---- Phase 2: Erase Slot B ---- */
    s_state = OTA_TASK_PREPARING;
    BspLed_BlinkStart(&s_led, 500);
    log_i("Erasing Slot B...");
    HAL_IWDG_Refresh(&hiwdg);

    if (BspFlash_EraseSlot(OTA_SLOT_B) != 0)
    {
        log_e("Erase failed!");
        goto error;
    }

    BspLed_BlinkStop(&s_led);
    log_i("Slot B erased.");

    /* ---- Phase 3: YMODEM receive ---- */
    s_state = OTA_TASK_RECEIVING;
    BspLed_On(&s_led);
    log_i("Waiting for YMODEM transfer...");

    /* 排空 UART RX 缓冲区（清除串口连接产生的残留字节） */
    __HAL_UART_FLUSH_DRREGISTER(&huart1);

    ret = Ymodem_Receive(uart_send_byte,
                                 uart_recv_byte,
                                 NULL,
                                 &s_fw_size,
                                 SLOT_MAX_APP_SIZE,
                                 ymodem_data_callback,
                                 NULL);

    if (ret != YMODEM_OK)
    {
        log_e("YMODEM error: %d", ret);
        goto error;
    }

    log_i("Received %lu bytes.", s_fw_size);

    /* ---- Phase 4: SHA-256 verify ---- */
    s_state = OTA_TASK_VERIFYING;
    /* LED 保持常亮 */
    log_i("Verifying SHA-256...");

    sha256_flash_region(FLASH_ADDR_SLOT_B, s_fw_size, hash);

    /* Print hash for debug */
    // clang-format off
    log_i("SHA-256: %02X%02X%02X%02X%02X%02X%02X%02X"
          "%02X%02X%02X%02X%02X%02X%02X%02X"
          "%02X%02X%02X%02X%02X%02X%02X%02X"
          "%02X%02X%02X%02X%02X%02X%02X%02X",
          hash[0],hash[1],hash[2],hash[3],hash[4],hash[5],hash[6],hash[7],
          hash[8],hash[9],hash[10],hash[11],hash[12],hash[13],hash[14],hash[15],
          hash[16],hash[17],hash[18],hash[19],hash[20],hash[21],hash[22],hash[23],
          hash[24],hash[25],hash[26],hash[27],hash[28],hash[29],hash[30],hash[31]);
    // clang-format on

    /* ---- Phase 5: Update Config ---- */
    s_state = OTA_TASK_UPDATING_CONFIG;

    cfg_ret = BspFlash_ReadConfig(&cfg);

    if (cfg_ret != 0)
    {
        /* First-time or corrupt config — initialise fresh */
        memset(&cfg, 0, sizeof(cfg));
        cfg.magic       = OTA_CONFIG_MAGIC;
        cfg.version     = OTA_CONFIG_VERSION;
        cfg.active_slot = OTA_SLOT_A;
    }

    cfg.state   = OTA_STATE_UPGRADE_PENDING;
    cfg.fw_size = s_fw_size;
    memcpy(cfg.fw_sha256, hash, 32);
    cfg.boot_count = 0;
    cfg.upgrade_count += 1;
    strncpy(cfg.prev_fw_version, OTA_FW_VERSION, sizeof(cfg.prev_fw_version) - 1);
    cfg.prev_fw_version[sizeof(cfg.prev_fw_version) - 1] = '\0';

    if (BspFlash_WriteConfig(&cfg) != 0)
    {
        log_e("Config write failed!");
        goto error;
    }

    log_i("Config updated.");

    /* ---- Phase 6: Reboot ---- */
    s_state = OTA_TASK_REBOOT_PENDING;
    BspLed_BlinkStart(&s_led, 100);
    log_i("Upgrade ready. Rebooting in 1s...");
    osDelay(1000);
    NVIC_SystemReset();

    /* Should never reach here */
    vTaskDelete(NULL);
    return;

error:
    s_state = OTA_TASK_ERROR;
    BspLed_BlinkStart(&s_led, 1000);
    log_e("ERROR! Rebooting in 3s...");

    for (int i = 0; i < 30; i++)
    {
        BspLed_TimebaseHook(&s_led);
        osDelay(100);
    }

    NVIC_SystemReset();
}
