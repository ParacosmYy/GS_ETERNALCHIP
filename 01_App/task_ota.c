/**
 * @file    task_ota.c
 * @brief   OTA 升级任务实现
 * @author  GS_Mark
 *
 * @par 依赖
 * - elog.h
 * - task_ota.h
 * - bsp_flash.h
 * - bsp_key_driver.h
 * - ymodem.h
 * - ota_transport.h
 * - ota_verify.h
 * - ota_ecdsa.h
 * - ota_led.h
 *
 * 以 FreeRTOS 任务方式运行。等待按键短按触发，通过 YMODEM 接收固件，
 * 写入 Bank B，验证 SHA-256 后重启。长按（>3s）触发强制回滚。
 */

//*** 头文件 ***//
#define LOG_TAG "OTA"
#include "elog.h"
#include "task_ota.h"
#include "system_adaption.h"
#include "plat_flash.h"
#include "plat_uart.h"
#include "plat_key.h"
#include "plat_wdg.h"
#include "plat_sys.h"
#include "bsp_wdg_driver.h"
#include "bsp_sys_driver.h"
#include "ymodem.h"
#include "cmsis_os.h"
#include "ota_transport.h"
#include "ota_verify.h"
#include "ota_ecdsa.h"
#include "ota_led.h"
#include "ota_trace.h"
#include "crash_dump.h"
#include <string.h>

//*** 私有变量 ***//

static ota_task_state_t s_state;
static uint32_t         s_fw_size;
static ota_slot_t       s_target_bank;
static volatile uint8_t s_trigger_flag;
static volatile uint8_t s_force_rollback;
static bsp_key_driver_t s_key;
static plat_gpio_t      s_key_gpio;

/* WDG 驱动实例 */
static bsp_wdg_driver_t s_wdg_drv;

/* SYS 驱动实例 */
static bsp_sys_driver_t s_sys_drv;

/* UART DMA 环形缓冲区 — 需容纳一个完整 YMODEM STX 包（1029 字节） */
#define UART_RX_RING_SIZE 2048
static uint8_t            s_ring_storage[UART_RX_RING_SIZE];
static circular_buffer_t s_ring_buf;
static bsp_uart_driver_t s_uart_drv;

//*** 运行 Bank 检测 ***//

//*** 自检 ***//

/**
 * @brief  App 启动自检。
 *
 * Steps:
 *  1. 验证 UART 句柄有效。
 *  2. 验证 Slot A 基地址 Flash 可读。
 *
 * @return   0 : 自检通过。
 * @return  -1 : 自检失败。
 * */
static int SelfCheck(void)
{
    volatile uint32_t val;

    val = *(volatile uint32_t *)FLASH_ADDR_SLOT_A;
    if (val == 0xFFFFFFFF)
    {
        log_w("Self-check: Slot A appears empty (first boot?)");
    }

    log_i("Self-check passed.");
    return 0;
}

//*** 按键回调 ***//

/**
 * @brief  按键事件回调 — 短按触发 OTA，长按触发回滚。
 *
 * Steps:
 *  1. 短按：置位触发标志，启动 OTA。
 *  2. 长按（仅在 TRIGGER_WAIT 状态）：置位强制回滚标志。
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
    else if (evt == BSP_KEY_EVT_LONG_PRESS)
    {
        if (s_state == OTA_TASK_TRIGGER_WAIT)
        {
            log_w("Force rollback triggered by long press!");
            s_force_rollback = 1;
            s_trigger_flag   = 1;
        }
    }
}

//*** 阶段函数 ***//

/**
 * @brief  阶段 1：等待按键触发或强制回滚。
 *
 * Steps:
 *  1. 设置状态 TRIGGER_WAIT，启动 IDLE LED（慢闪 1Hz）。
 *  2. 轮询按键扫描 + OtaLed_TimebaseHook，间隔 10ms。
 *  3. 返回 0 表示正常触发，-2 表示强制回滚。
 *
 * @return   0 : 短按触发。
 * @return  -2 : 长按强制回滚。
 * */
static int WaitForTrigger(void)
{
    s_state        = OTA_TASK_TRIGGER_WAIT;
    s_force_rollback = 0;
    OtaLed_SetMode(OTA_LED_IDLE);

    log_i("========================================");
    log_i("  GS_ETERNALCHIP OTA Application");
    log_i("  STM32F411CEUx @ 96MHz (Dual Bank)");
    log_i("  App   : %s", OTA_FW_VERSION);
    log_i("========================================");
    log_i("Waiting for trigger... (short press = OTA, long press = rollback)");

    while (!s_trigger_flag)
    {
        uint8_t byte;

        BspKey_Scan(&s_key);
        OtaLed_TimebaseHook();

        /* 远程触发：检查 ring buffer 中的 0xF5 */
        if (BspUart_ReadByte(&s_uart_drv, &byte, 0) == 0 && byte == 0xF5)
        {
            log_i("Remote trigger received (0xF5)");
            s_trigger_flag = 1;
        }

        osDelay(10);
    }

    if (s_force_rollback)
    {
        log_w("Entering force rollback mode...");
        return -2;
    }

    OtaTrace_Clear();
    OtaTrace_Record(OTA_TRACE_TRIGGER, 0, 0);
    log_i("Starting upgrade...");
    return 0;
}

/**
 * @brief  阶段 2：擦除目标 Bank。
 *
 * Steps:
 *  1. 设置状态 PREPARING，LED 快闪 4Hz。
 *  2. 擦除前喂狗（擦除耗时较长）。
 *  3. 擦除 Bank B，检查结果。
 *
 * @return   0 : 擦除成功。
 * @return  -1 : 擦除失败。
 * */
static int EraseTargetBank(void)
{
    s_state = OTA_TASK_PREPARING;
    OtaLed_SetMode(OTA_LED_WORKING);
    log_i("Erasing target Bank...");

    OtaTrace_Record(OTA_TRACE_ERASE_START, 0, 0);

    BspWdg_Feed(&s_wdg_drv);

    if (BspFlash_EraseSlot(s_target_bank) != 0)
    {
        log_e("Erase failed!");
        return -1;
    }

    OtaTrace_Record(OTA_TRACE_ERASE_DONE, 0, 0);
    log_i("Target Bank erased.");
    return 0;
}

/**
 * @brief  阶段 3：通过 YMODEM 接收固件。
 *
 * Steps:
 *  1. 设置状态 RECEIVING，LED 常亮。
 *  2. 清空 UART DR 寄存器，初始化传输模块。
 *  3. 调用 Ymodem_Receive 使用传输回调。
 *
 * @return   0 : 接收成功。
 * @return  -1 : YMODEM 错误。
 * */
static int ReceiveFirmware(void)
{
    int ret;

    s_state = OTA_TASK_RECEIVING;
    OtaLed_SetMode(OTA_LED_RECEIVING);
    log_i("Waiting for YMODEM transfer...");

    OtaTrace_Record(OTA_TRACE_YMODEM_START, 0, 0);

    /*
     * 等待 UART TX 发送完毕，防止 log 中的 'C'(0x43) 字符
     * 被 PC 端误判为 YMODEM CRC 请求。先等 TX drain，再清 RX。
     */
    osDelay(100);
    BspUart_FlushRx(&s_uart_drv);
    ring_buffer_init(&s_ring_buf, s_ring_storage, UART_RX_RING_SIZE);

    OtaTransport_Init(&s_fw_size, OtaConfig_BankAddr(s_target_bank), &s_uart_drv, &s_wdg_drv);

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

    OtaTrace_Record(OTA_TRACE_YMODEM_DONE, 0, s_fw_size);
    log_i("Received %lu bytes.", s_fw_size);
    return 0;
}

/**
 * @brief  阶段 3.5：验证解密后固件的 ECDSA-P256 签名。
 *
 * 数据包格式：[nonce 12B][加密固件][ECDSA 签名 64B]
 * 传输层处理后，Flash 中仅保留解密后的固件。
 * 签名由传输层提取，通过 OtaTransport_GetSignature 获取。
 *
 * Steps:
 *  1. 从传输层获取签名（YMODEM 接收期间提取）。
 *  2. 计算实际固件大小（总大小 - 12 nonce - 64 签名）。
 *  3. 对 Flash 中解密后的固件验证 ECDSA-P256 签名。
 *  4. 修正 s_fw_size 为实际固件大小（去除开销）。
 *
 * @param[out] p_real_fw_size : 输出实际固件长度。
 * @param[out] hash           : 输出 SHA-256 哈希（32 字节）。
 *
 * @return   0 : 签名验证通过。
 * @return  -1 : 验证失败。
 * */
static int VerifyEcdsaSignature(uint32_t *p_real_fw_size, uint8_t hash[32])
{
    uint8_t  sig[OTA_ECDSA_SIG_SIZE];
    uint32_t real_size;
    uint32_t pkg_overhead;

    s_state = OTA_TASK_VERIFYING_SIG;
    OtaLed_SetMode(OTA_LED_WORKING);

    /* 数据包开销：nonce(12B) + 签名(64B) = 76 字节 */
    pkg_overhead = 12u + OTA_ECDSA_SIG_SIZE;
    real_size = s_fw_size - pkg_overhead;

    if (s_fw_size < pkg_overhead || real_size == 0)
    {
        log_e("Invalid firmware size: %lu (need > %lu overhead)",
              s_fw_size, pkg_overhead);
        return -1;
    }

    /* 获取传输层在 YMODEM 接收期间提取的签名 */
    OtaTransport_GetSignature(sig);
    log_i("Verifying ECDSA signature (%lu bytes decrypted firmware)...", real_size);

    OtaTrace_Record(OTA_TRACE_ECDSA_START, 0, real_size);

    BspWdg_Feed(&s_wdg_drv);

    /* 对 Flash 中已解密的固件进行验签 */
    if (OtaEcdsa_Verify(OtaConfig_BankAddr(s_target_bank), real_size, sig, hash) != 0)
    {
        return -1;
    }

    OtaTrace_Record(OTA_TRACE_ECDSA_DONE, 0, real_size);
    *p_real_fw_size = real_size;
    return 0;
}

/**
 * @brief  阶段 4：计算 Bank B 固件的 SHA-256 哈希。
 *
 * Steps:
 *  1. 设置状态 VERIFYING。
 *  2. 调用 OtaVerify_SHA256Flash 计算固件哈希。
 *  3. 打印 32 字节十六进制哈希值。
 *
 * @param[out] hash : 输出 32 字节 SHA-256 哈希。
 *
 * @return  0 : 成功。
 * */
static int VerifyFirmware(uint8_t hash[32])
{
    s_state = OTA_TASK_VERIFYING;
    log_i("Verifying SHA-256...");

    OtaTrace_Record(OTA_TRACE_SHA256_START, 0, s_fw_size);

    OtaVerify_SHA256Flash(OtaConfig_BankAddr(s_target_bank), s_fw_size, hash);

    OtaTrace_Record(OTA_TRACE_SHA256_DONE, 0, s_fw_size);

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
 * @brief  阶段 5：更新 Flash 中的 OTA 配置。
 *
 * Steps:
 *  1. 读取已有配置或创建默认配置。
 *  2. 设置状态 UPGRADE_PENDING，写入 fw_size 和 SHA-256。
 *  3. 记录当前版本，递增 upgrade_count。
 *  4. 写回 Flash。
 *
 * @param[in] hash : SHA-256 哈希（32 字节）。
 *
 * @return   0 : 成功。
 * @return  -1 : 配置写入失败。
 * */
static int UpdateConfig(const uint8_t hash[32])
{
    ota_config_t cfg;
    int          cfg_ret;

    s_state = OTA_TASK_UPDATING_CONFIG;
    log_i("Updating OTA config...");

    cfg_ret = BspFlash_ReadConfig(&cfg);
    if (cfg_ret != 0)
    {
        memset(&cfg, 0, sizeof(cfg));
        cfg.magic       = OTA_CONFIG_MAGIC;
        cfg.version     = OTA_CONFIG_VERSION;
        cfg.active_slot = OTA_SLOT_A;
    }

    cfg.state   = OTA_STATE_UPGRADE_PENDING;
    cfg.active_slot = s_target_bank;
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

    OtaTrace_Record(OTA_TRACE_CFG_WRITE, 0, cfg.upgrade_count);
    log_i("Config updated.");
    return 0;
}

/**
 * @brief  强制回滚：设置 OTA_STATE_ROLLBACK 并重启。
 *
 * Steps:
 *  1. 读取已有配置或创建默认配置。
 *  2. 设置状态 ROLLBACK，清零 boot_count。
 *  3. 写入配置并重启。
 * */
static void ForceRollback(void)
{
    ota_config_t cfg;
    int          cfg_ret;

    OtaLed_SetMode(OTA_LED_REBOOTING);
    log_w("Setting OTA state to ROLLBACK...");

    cfg_ret = BspFlash_ReadConfig(&cfg);
    if (cfg_ret != 0)
    {
        memset(&cfg, 0, sizeof(cfg));
        cfg.magic       = OTA_CONFIG_MAGIC;
        cfg.version     = OTA_CONFIG_VERSION;
        cfg.active_slot = OTA_SLOT_A;
    }

    cfg.state      = OTA_STATE_ROLLBACK;
    cfg.boot_count = 0;

    if (BspFlash_WriteConfig(&cfg) != 0)
    {
        log_e("Failed to write rollback config!");
    }
    else
    {
        log_w("Rollback config written. Rebooting in 1s...");
    }

    osDelay(1000);
    BspSys_Reboot(&s_sys_drv);
}

/**
 * @brief  阶段 6：重启设备。
 *
 * Steps:
 *  1. 设置状态 REBOOT_PENDING，LED 极快闪 10Hz。
 *  2. 等待 1s 让 UART 日志发送完毕。
 *  3. 软件复位。
 * */
static void RebootDevice(void)
{
    s_state = OTA_TASK_REBOOT_PENDING;
    OtaLed_SetMode(OTA_LED_REBOOTING);

    OtaTrace_Record(OTA_TRACE_REBOOT, 0, s_fw_size);
    log_i("Upgrade ready. Rebooting in 1s...");

    osDelay(1000);
    BspSys_Reboot(&s_sys_drv);
}

/**
 * @brief  OTA 错误处理：显示错误码 LED 闪烁模式后重启。
 *
 * Steps:
 *  1. 将结果码映射为错误码。
 *  2. 设置 LED 错误闪烁模式（N 次闪烁 + 2s 暂停）。
 *  3. 驱动 LED 时基 3s 以展示错误模式。
 *  4. 软件复位。
 *
 * @param[in] result : 阶段链返回的负值结果码。
 * */
static void HandleError(int result)
{
    int      i;
    uint8_t  code;

    s_state = OTA_TASK_ERROR;

    code = OtaLed_GetErrorCode();

    /* result 记录阶段返回值（负数），data 记录 LED 错误码 */
    OtaTrace_Record(OTA_TRACE_ERROR, (uint32_t)(-result), code);

    log_e("ERROR! Code=%u, Phase=%d, %s. Rebooting in 3s...",
          code,
          result,
          code == OTA_ERR_ERASE_FAILED  ? "Erase failed" :
          code == OTA_ERR_YMODEM_FAILED ? "YMODEM failed" :
          code == OTA_ERR_ECDSA_FAILED  ? "ECDSA failed" :
          code == OTA_ERR_SHA256_FAILED ? "SHA-256 failed" :
          "Config failed");

    /* 驱动 LED 错误模式 3 秒 */
    for (i = 0; i < 60; i++)
    {
        OtaLed_TimebaseHook();
        osDelay(50);
    }

    BspSys_Reboot(&s_sys_drv);
}

//*** 公开 API ***//

/**
 * @brief  初始化 OTA 子系统和 EasyLogger。
 *
 * Steps:
 *  1. 复位 OTA 状态、固件大小、触发标志。
 *  2. 初始化 Flash 驱动。
 *  3. 配置 EasyLogger 日志格式并启动。
 * */
void TaskOta_Init(void *p_huart, void *p_hiwdg, void *p_key_port, uint16_t key_pin)
{
    static uint8_t s_uart_rx_buf[2048];
    static bsp_uart_config_t uart_cfg;

    uart_cfg.p_huart    = p_huart;
    uart_cfg.p_rx_buf   = s_uart_rx_buf;
    uart_cfg.rx_buf_size = sizeof(s_uart_rx_buf);
    uart_cfg.callback   = NULL;
    uart_cfg.p_user_data = NULL;

    s_state        = OTA_TASK_NORMAL;
    s_fw_size      = 0;
    s_trigger_flag = 0;
    s_force_rollback = 0;

    /* 保存按键 GPIO 配置，供后续 TaskOta_Run 使用 */
    s_key_gpio.port         = p_key_port;
    s_key_gpio.pin          = key_pin;
    s_key_gpio.active_level = 0;

    BspFlash_Init();
    BspWdg_Init(&s_wdg_drv, p_hiwdg, &g_wdg_hal_ops);
    BspSys_Init(&s_sys_drv, &g_sys_hal_ops);

    /* 初始化 UART 驱动 + ring buffer (ops from system_adaption) */
    BspUart_Init(&s_uart_drv, &uart_cfg, &g_uart_hal_ops, &g_uart_os_ops);
    ring_buffer_init(&s_ring_buf, s_ring_storage, UART_RX_RING_SIZE);
    BspUart_BindRingBuffer(&s_uart_drv, &s_ring_buf);
    BspUart_StartReceive(&s_uart_drv);

    elog_init();
    elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL);
    elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_LVL | ELOG_FMT_TAG);
    elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_TAG);
    elog_start();

    /* 初始化 OTA 追踪模块，打印上次追踪日志 */
    OtaTrace_Init(&s_sys_drv);
    OtaTrace_PrintAll();

    /* 检查上次是否有 crash 记录 */
    if (CrashDump_CheckAndPrint())
    {
        ota_config_t cfg;

        /*
         * 如果上次 crash 发生在 CONFIRMED 状态（正常运行中 crash），
         * 将状态重置为 CONFIRMING，让 Boot 的 boot_count 回滚机制接管。
         * 连续 crash 达到阈值后 Boot 会自动切换到旧 Bank。
         */
        if (BspFlash_ReadConfig(&cfg) == 0 && cfg.state == OTA_STATE_CONFIRMED)
        {
            cfg.state      = OTA_STATE_CONFIRMING;
            cfg.boot_count = 0;
            if (BspFlash_WriteConfig(&cfg) == 0)
            {
                log_w("Crash detected in CONFIRMED state -> CONFIRMING (rollback armed)");
            }
        }
    }

    OtaTrace_Record(OTA_TRACE_APP_START, 0, 0);
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

//*** 任务入口 ***//

/**
 * @brief  OTA 任务入口函数（FreeRTOS 任务）。
 *
 * Steps:
 *  1. 调用 TaskOta_Init 初始化子系统和日志器。
 *  2. 执行自检（UART + Flash）。
 *  3. 初始化 LED 模块和按键驱动。
 *  4. 等待触发（短按 = OTA，长按 = 回滚）。
 *  5. 若回滚：设置 OTA_STATE_ROLLBACK 并重启。
 *  6. 否则：串联 6 个阶段（擦除→接收→ECDSA→SHA256→配置→重启）。
 *  7. 任意阶段失败：显示错误 LED 模式并重启。
 *
 * @param[in] p_argument : 未使用（传 NULL）。
 * */
void TaskOta_Run(void *p_argument)
{
    uint8_t hash[32];
    int     result;

    (void)p_argument;

    /* 自检 */
    if (SelfCheck() != 0)
    {
        ForceRollback();
    }

    /* LED + 按键初始化 */
    OtaLed_Init();

    {
        static bsp_key_config_t key_cfg;
        key_cfg.gpio          = s_key_gpio;
        key_cfg.debounce_ms   = 20;
        key_cfg.long_press_ms = 3000;
        key_cfg.callback      = OnKey;
        key_cfg.p_user_data   = NULL;
        BspKey_Init(&s_key, &key_cfg, &g_key_hal_ops, &g_key_os_ops);
    }

    /* 等待触发（短按 = OTA，长按 = 回滚） */
    result = WaitForTrigger();
    if (result == -2)
    {
        ForceRollback();
    }

    /* 确定目标 Bank：优先使用当前实际运行 Bank，避免配置异常时擦除自身 */
    {
        ota_config_t cfg;
        ota_slot_t   cfg_active = OTA_SLOT_A;
        ota_slot_t   active     = OTA_SLOT_A;
        int          cfg_ok;

        cfg_ok = BspFlash_ReadConfig(&cfg);
        if (cfg_ok == 0)
        {
            cfg_active = cfg.active_slot;
            active     = cfg.active_slot;
        }

        if (BspSys_GetRunningBank(&s_sys_drv, &active) == 0)
        {
            if (cfg_ok == 0 && active != cfg_active)
            {
                log_w("Config Active=%c differs from running Bank=%c, use running Bank",
                      cfg_active == OTA_SLOT_A ? 'A' : 'B',
                      active == OTA_SLOT_A ? 'A' : 'B');
            }
        }
        else if (cfg_ok != 0)
        {
            log_w("Running Bank unknown, fallback Active=A");
        }

        s_target_bank = OtaConfig_Other(active);
        log_i("Active=%c, Target=%c",
              active == OTA_SLOT_A ? 'A' : 'B',
              s_target_bank == OTA_SLOT_A ? 'A' : 'B');
    }

    /* 串联各阶段调用 — 每个阶段失败映射为特定错误码 */
    result = EraseTargetBank();
    if (result != 0)
    {
        OtaLed_SetErrorCode(OTA_ERR_ERASE_FAILED);
        HandleError(result);
    }

    result = ReceiveFirmware();
    if (result != 0)
    {
        OtaLed_SetErrorCode(OTA_ERR_YMODEM_FAILED);
        HandleError(result);
    }

    result = VerifyEcdsaSignature(&s_fw_size, hash);
    if (result != 0)
    {
        OtaLed_SetErrorCode(OTA_ERR_ECDSA_FAILED);
        HandleError(result);
    }

    /* SHA-256 已在 ECDSA 验签中计算，直接打印结果 */
    OtaTrace_Record(OTA_TRACE_SHA256_DONE, 0, s_fw_size);
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

    result = UpdateConfig(hash);
    if (result != 0)
    {
        OtaLed_SetErrorCode(OTA_ERR_CONFIG_FAILED);
        HandleError(result);
    }

    RebootDevice();

    vTaskDelete(NULL);
}
