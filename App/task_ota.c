/**
 * @file    task_ota.c
 * @brief   OTA upgrade task implementation
 * @author  GS_Mark
 *
 * @par dependencies
 * - elog.h
 * - task_ota.h
 * - bsp_flash.h
 * - bsp_led.h
 * - bsp_key.h
 * - ymodem.h
 * - ota_transport.h
 * - ota_verify.h
 * - ota_ecdsa.h
 *
 * Runs as a FreeRTOS task. Waits for key short press trigger, then receives
 * firmware via YMODEM, writes to Slot B, verifies SHA-256, and reboots.
 */

//*** Includes ***//
#define LOG_TAG "OTA"
#include "elog.h"
#include "task_ota.h"
#include "bsp_flash.h"
#include "bsp_led.h"
#include "bsp_key.h"
#include "ymodem.h"
#include "usart.h"
#include "iwdg.h"
#include "cmsis_os.h"
#include "ota_transport.h"
#include "ota_verify.h"
#include "ota_ecdsa.h"
#include <string.h>

//*** Private Variables ***//

static ota_task_state_t s_state;
static uint32_t         s_fw_size;
static volatile uint8_t s_trigger_flag;
static bsp_led_driver_t s_led;
static bsp_key_driver_t s_key;

//*** Key Callback ***//

/**
 * @brief  按键事件回调，短按时置位触发标志。
 *
 * Steps:
 *  1. 判断事件类型为 BSP_KEY_EVT_SHORT_PRESS。
 *  2. 置位 s_trigger_flag 唤醒 OTA 任务。
 *
 * @param[in] evt     : 按键事件类型。
 * @param[in] p_user  : 用户上下文（未使用）。
 * */
static void OnKey(bsp_key_event_t evt, void *p_user)
{
    (void)p_user;

    if (evt == BSP_KEY_EVT_SHORT_PRESS)
    {
        log_i("Triggered!");
        s_trigger_flag = 1;
    }
}

//*** Phase Functions ***//

/**
 * @brief  Phase 1: 等待按键短按触发（无限期）。
 *
 * Steps:
 *  1. 设置任务状态为 TRIGGER_WAIT，打印当前固件版本。
 *  2. 轮询按键扫描 + osDelay(10)，直到 s_trigger_flag 被置位。
 *
 * @return  0 : 触发成功。
 * */
static int WaitForTrigger(void)
{
    s_state = OTA_TASK_TRIGGER_WAIT;
    log_i("Firmware %s", OTA_FW_VERSION);
    log_i("Waiting for trigger... (short press key)");

    while (!s_trigger_flag)
    {
        BspKey_Scan(&s_key);
        osDelay(10);
    }

    log_i("Starting upgrade...");
    return 0;
}

/**
 * @brief  Phase 2: 擦除 Slot B。
 *
 * Steps:
 *  1. 设置状态 PREPARING，LED 500ms 闪烁。
 *  2. 喂看门狗后擦除 Slot B。
 *  3. 停止 LED 闪烁。
 *
 * @return   0 : 擦除成功。
 * @return  -1 : 擦除失败。
 * */
static int EraseSlotB(void)
{
    s_state = OTA_TASK_PREPARING;
    BspLed_BlinkStart(&s_led, 500);
    log_i("Erasing Slot B...");

    /* 擦除前喂狗，防止擦除等待期间触发看门狗复位 */
    HAL_IWDG_Refresh(&hiwdg);

    if (BspFlash_EraseSlot(OTA_SLOT_B) != 0)
    {
        log_e("Erase failed!");
        return -1;
    }

    BspLed_BlinkStop(&s_led);
    log_i("Slot B erased.");
    return 0;
}

/**
 * @brief  Phase 3: YMODEM 接收固件写入 Slot B。
 *
 * Steps:
 *  1. 设置状态 RECEIVING，LED 常亮。
 *  2. 清空 UART DR 寄存器，初始化传输模块。
 *  3. 调用 Ymodem_Receive 通过回调逐块写入 Flash。
 *  4. 检查返回值，打印接收字节数。
 *
 * @return   0 : 接收成功。
 * @return  -1 : YMODEM 错误。
 * */
static int ReceiveFirmware(void)
{
    int ret;

    s_state = OTA_TASK_RECEIVING;
    BspLed_On(&s_led);
    log_i("Waiting for YMODEM transfer...");

    /* 清空 UART 接收寄存器残留数据，防止 YMODEM 握手异常 */
    __HAL_UART_FLUSH_DRREGISTER(&huart1);
    OtaTransport_Init(&s_fw_size);

    ret = Ymodem_Receive(OtaTransport_SendByte,
                          OtaTransport_RecvByte,
                          NULL,
                          &s_fw_size,
                          SLOT_MAX_APP_SIZE,
                          OtaTransport_DataCallback,
                          NULL);

    if (ret != YMODEM_OK)
    {
        log_e("YMODEM error: %d", ret);
        return -1;
    }

    log_i("Received %lu bytes.", s_fw_size);
    return 0;
}

/**
 * @brief  Phase 3.5: 从 Slot B 末尾提取 ECDSA 签名并验签。
 *
 * Steps:
 *  1. 计算实际固件长度（总接收大小 - 64 字节签名）。
 *  2. 从 Slot B 末尾 64 字节读取 ECDSA-P256 签名。
 *  3. 调用 OtaEcdsa_Verify 验证固件区域签名。
 *  4. 修正 s_fw_size 为实际固件大小（去掉签名）。
 *
 * @param[out] p_real_fw_size : 输出实际固件长度（去除签名）。
 *
 * @return   0 : 验签成功。
 * @return  -1 : 验签失败。
 * */
static int VerifyEcdsaSignature(uint32_t *p_real_fw_size)
{
    uint8_t  sig[OTA_ECDSA_SIG_SIZE];
    uint32_t real_size;

    s_state = OTA_TASK_VERIFYING_SIG;

    /* YMODEM 接收的是 firmware + signature，固件实际大小 = 总大小 - 64 */
    real_size = s_fw_size - OTA_ECDSA_SIG_SIZE;

    if (s_fw_size < OTA_ECDSA_SIG_SIZE || real_size == 0)
    {
        log_e("Invalid firmware size: %lu", s_fw_size);
        return -1;
    }

    /* 从 Slot B 末尾读取 ECDSA 签名 */
    memcpy(sig, (const void *)(FLASH_ADDR_SLOT_B + real_size), OTA_ECDSA_SIG_SIZE);

    log_i("Verifying ECDSA signature (%lu bytes firmware)...", real_size);

    /* 喂狗：验签计算耗时约 150~300ms */
    HAL_IWDG_Refresh(&hiwdg);

    if (OtaEcdsa_Verify(FLASH_ADDR_SLOT_B, real_size, sig) != 0)
    {
        return -1;
    }

    /* 修正 fw_size 为实际固件大小，后续 SHA-256 和 Config 使用 */
    *p_real_fw_size = real_size;
    return 0;
}

/**
 * @brief  Phase 4: 计算 Slot B 的 SHA-256 哈希。
 *
 * Steps:
 *  1. 设置状态 VERIFYING。
 *  2. 调用 OtaVerify_SHA256Flash 读取 Slot B 并计算哈希。
 *  3. 打印 32 字节哈希值（十六进制）。
 *
 * @param[out] hash : 输出 32 字节 SHA-256 哈希值。
 *
 * @return  0 : 成功。
 * */
static int VerifyFirmware(uint8_t hash[32])
{
    s_state = OTA_TASK_VERIFYING;
    log_i("Verifying SHA-256...");

    /* 对 Slot B 全部固件数据计算 SHA-256，用于升级后校验 */
    OtaVerify_SHA256Flash(FLASH_ADDR_SLOT_B, s_fw_size, hash);

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

    return 0;
}

/**
 * @brief  Phase 5: 更新 OTA Config（状态、哈希、版本号）。
 *
 * Steps:
 *  1. 尝试读取现有 Config，失败则创建默认配置。
 *  2. 设置 state = UPGRADE_PENDING，写入固件大小和 SHA-256。
 *  3. 记录当前版本号到 prev_fw_version，递增 upgrade_count。
 *  4. 写回 Flash Config 区。
 *
 * @param[in] hash : SHA-256 哈希值（32 字节）。
 *
 * @return   0 : 成功。
 * @return  -1 : 配置写入失败。
 * */
static int UpdateConfig(const uint8_t hash[32])
{
    ota_config_t cfg;
    int          cfg_ret;

    s_state = OTA_TASK_UPDATING_CONFIG;

    cfg_ret = BspFlash_ReadConfig(&cfg);
    if (cfg_ret != 0)
    {
        /* 首次升级或 Config 区损坏，使用默认值 */
        memset(&cfg, 0, sizeof(cfg));
        cfg.magic       = OTA_CONFIG_MAGIC;
        cfg.version     = OTA_CONFIG_VERSION;
        cfg.active_slot = OTA_SLOT_A;
    }

    /* 填充升级信息：状态、大小、哈希、版本号 */
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
        return -1;
    }

    log_i("Config updated.");
    return 0;
}

/**
 * @brief  Phase 6: 重启设备。
 *
 * Steps:
 *  1. 设置状态 REBOOT_PENDING，LED 快闪 (100ms)。
 *  2. 延时 1 秒等待日志输出完成。
 *  3. 调用 NVIC_SystemReset 软件复位。
 * */
static void RebootDevice(void)
{
    s_state = OTA_TASK_REBOOT_PENDING;
    BspLed_BlinkStart(&s_led, 100);
    log_i("Upgrade ready. Rebooting in 1s...");

    /* 等待 1s 确保 UART 日志完全发送 */
    osDelay(1000);
    NVIC_SystemReset();
}

/**
 * @brief  OTA 错误处理：LED 慢闪，1 秒后重启。
 *
 * Steps:
 *  1. 设置状态 ERROR，LED 1000ms 慢闪。
 *  2. 循环 10 次调用 LED TimebaseHook + 100ms 延时（共 1 秒）。
 *  3. 软件复位。
 * */
static void HandleError(void)
{
    int i;

    s_state = OTA_TASK_ERROR;
    BspLed_BlinkStart(&s_led, 1000);
    log_e("ERROR! Rebooting in 1s...");

    /* 手动驱动 LED 时基，因为此时可能不在 FreeRTOS 调度中 */
    for (i = 0; i < 10; i++)
    {
        BspLed_TimebaseHook(&s_led);
        osDelay(100);
    }

    NVIC_SystemReset();
}

//*** Public API ***//

/**
 * @brief  初始化 OTA 子系统和 EasyLogger。
 *
 * Steps:
 *  1. 重置 OTA 状态、固件大小、触发标志。
 *  2. 初始化 Flash 驱动。
 *  3. 配置 EasyLogger 各级别日志格式并启动。
 * */
void TaskOta_Init(void)
{
    s_state        = OTA_TASK_NORMAL;
    s_fw_size      = 0;
    s_trigger_flag = 0;
    BspFlash_Init();

    elog_init();
    elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL);
    elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_LVL | ELOG_FMT_TAG);
    elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_TAG);
    elog_start();
}

/**
 * @brief  获取当前 OTA 任务状态。
 *
 * @return  ota_task_state_t : 当前任务状态。
 * */
ota_task_state_t TaskOta_GetState(void)
{
    return s_state;
}

//*** Task Entry ***//

/**
 * @brief  OTA 任务入口函数（FreeRTOS 任务）。
 *
 * Steps:
 *  1. 调用 TaskOta_Init 初始化子系统和日志。
 *  2. 初始化 LED（PC13 低有效）和按键（PA0 低有效）。
 *  3. 链式执行 6 个阶段：触发→擦除→接收→校验→配置→重启。
 *  4. 任何阶段失败则调用 HandleError 延时重启。
 *  5. 任务结束后调用 vTaskDelete 自删除。
 *
 * @param[in] p_argument : 未使用（传 NULL）。
 * */
void TaskOta_Run(void *p_argument)
{
    uint8_t hash[32];
    int     result;

    (void)p_argument;
    TaskOta_Init();

    /* LED: PC13 低有效 */
    {
        static const bsp_led_config_t led_cfg = { GPIOC, GPIO_PIN_13, 0 };
        BspLed_Init(&s_led, &led_cfg);
    }

    /* KEY: PA0 低有效, 20ms 消抖, 1000ms 长按 */
    {
        static const bsp_key_config_t key_cfg = { GPIOA, GPIO_PIN_0, 0, 20, 1000, OnKey, NULL };
        BspKey_Init(&s_key, &key_cfg);
    }

    /* 链式 phase 调用：任何一步失败跳到 HandleError */
    result = WaitForTrigger();
    if (result == 0) result = EraseSlotB();
    if (result == 0) result = ReceiveFirmware();
    if (result == 0) result = VerifyEcdsaSignature(&s_fw_size);
    if (result == 0) result = VerifyFirmware(hash);
    if (result == 0) result = UpdateConfig(hash);

    if (result == 0)
    {
        RebootDevice();
    }
    else
    {
        HandleError();
    }

    vTaskDelete(NULL);
}
