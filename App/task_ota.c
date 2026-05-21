/**
 * @file    task_ota.c
 * @brief   OTA upgrade task implementation
 * @author  GS_Mark
 *
 * @par dependencies
 * - elog.h
 * - task_ota.h
 * - bsp_flash.h
 * - bsp_key.h
 * - ymodem.h
 * - ota_transport.h
 * - ota_verify.h
 * - ota_ecdsa.h
 * - ota_led.h
 *
 * Runs as a FreeRTOS task. Waits for key short press trigger, then receives
 * firmware via YMODEM, writes to Slot B, verifies SHA-256, and reboots.
 * Long press (>3s) triggers force rollback.
 */

//*** Includes ***//
#define LOG_TAG "OTA"
#include "elog.h"
#include "task_ota.h"
#include "bsp_flash.h"
#include "bsp_uart.h"
#include "bsp_key.h"
#include "ymodem.h"
#include "usart.h"
#include "iwdg.h"
#include "cmsis_os.h"
#include "ota_transport.h"
#include "ota_verify.h"
#include "ota_ecdsa.h"
#include "ota_led.h"
#include <string.h>

//*** Private Variables ***//

static ota_task_state_t s_state;
static uint32_t         s_fw_size;
static ota_slot_t       s_target_bank;
static volatile uint8_t s_trigger_flag;
static volatile uint8_t s_force_rollback;
static bsp_key_driver_t s_key;

/* Ring buffer for UART DMA — must hold one full YMODEM STX packet (1029 bytes) */
#define UART_RX_RING_SIZE 2048
static uint8_t            s_ring_storage[UART_RX_RING_SIZE];
static circular_buffer_t s_ring_buf;
static bsp_uart_driver_t s_uart_drv;

//*** Self Check ***//

/**
 * @brief  App startup self-check.
 *
 * Steps:
 *  1. Verify UART handle is valid.
 *  2. Verify Flash is readable at Slot A base address.
 *
 * @return   0 : self-check passed.
 * @return  -1 : self-check failed.
 * */
static int SelfCheck(void)
{
    volatile uint32_t val;

    if (&huart1 == NULL)
    {
        log_e("Self-check FAILED: UART handle invalid");
        return -1;
    }

    val = *(volatile uint32_t *)FLASH_ADDR_SLOT_A;
    if (val == 0xFFFFFFFF)
    {
        log_w("Self-check: Slot A appears empty (first boot?)");
    }

    log_i("Self-check passed.");
    return 0;
}

//*** Key Callback ***//

/**
 * @brief  Key event callback — short press triggers OTA, long press triggers rollback.
 *
 * Steps:
 *  1. On short press: set trigger flag to start OTA.
 *  2. On long press (only in TRIGGER_WAIT state): set force rollback flag.
 *
 * @param[in] evt     : Key event type.
 * @param[in] p_user  : User context (unused).
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

//*** Phase Functions ***//

/**
 * @brief  Phase 1: Wait for key trigger or force rollback.
 *
 * Steps:
 *  1. Set state TRIGGER_WAIT, start IDLE LED (slow blink 1Hz).
 *  2. Poll key scan + OtaLed_TimebaseHook every 10ms.
 *  3. Return 0 for normal trigger, -2 for force rollback.
 *
 * @return   0 : short press trigger.
 * @return  -2 : long press force rollback.
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
        BspKey_Scan(&s_key);
        OtaLed_TimebaseHook();
        osDelay(10);
    }

    if (s_force_rollback)
    {
        log_w("Entering force rollback mode...");
        return -2;
    }

    log_i("Starting upgrade...");
    return 0;
}

/**
 * @brief  Phase 2: Erase Slot B.
 *
 * Steps:
 *  1. Set state PREPARING, LED fast blink 4Hz.
 *  2. Feed IWDG before erase (erase takes time).
 *  3. Erase Slot B, check result.
 *
 * @return   0 : erase success.
 * @return  -1 : erase failed.
 * */
static int EraseTargetBank(void)
{
    s_state = OTA_TASK_PREPARING;
    OtaLed_SetMode(OTA_LED_WORKING);
    log_i("Erasing target Bank...");

    HAL_IWDG_Refresh(&hiwdg);

    if (BspFlash_EraseSlot(s_target_bank) != 0)
    {
        log_e("Erase failed!");
        return -1;
    }

    log_i("Target Bank erased.");
    return 0;
}

/**
 * @brief  Phase 3: Receive firmware via YMODEM.
 *
 * Steps:
 *  1. Set state RECEIVING, LED solid on.
 *  2. Flush UART DR register, init transport module.
 *  3. Call Ymodem_Receive with transport callbacks.
 *
 * @return   0 : receive success.
 * @return  -1 : YMODEM error.
 * */
static int ReceiveFirmware(void)
{
    int ret;

    s_state = OTA_TASK_RECEIVING;
    OtaLed_SetMode(OTA_LED_RECEIVING);
    log_i("Waiting for YMODEM transfer...");

    /*
     * 等待 UART TX 发送完毕，防止 log 中的 'C'(0x43) 字符
     * 被 PC 端误判为 YMODEM CRC 请求。先等 TX drain，再清 RX。
     */
    osDelay(100);
    __HAL_UART_FLUSH_DRREGISTER(&huart1);

    OtaTransport_Init(&s_fw_size, OtaConfig_BankAddr(s_target_bank), &s_uart_drv);

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
 * @brief  Phase 3.5: Extract and verify ECDSA-P256 signature.
 *
 * Steps:
 *  1. Calculate real firmware size (total - 64 bytes signature).
 *  2. Read signature from Slot B tail.
 *  3. Call OtaEcdsa_Verify with embedded public key.
 *  4. Fix s_fw_size to actual firmware size (strip signature).
 *
 * @param[out] p_real_fw_size : Output actual firmware length.
 *
 * @return   0 : signature verified.
 * @return  -1 : verification failed.
 * */
static int VerifyEcdsaSignature(uint32_t *p_real_fw_size)
{
    uint8_t  sig[OTA_ECDSA_SIG_SIZE];
    uint32_t real_size;

    s_state = OTA_TASK_VERIFYING_SIG;
    OtaLed_SetMode(OTA_LED_WORKING);

    real_size = s_fw_size - OTA_ECDSA_SIG_SIZE;

    if (s_fw_size < OTA_ECDSA_SIG_SIZE || real_size == 0)
    {
        log_e("Invalid firmware size: %lu", s_fw_size);
        return -1;
    }

    memcpy(sig, (const void *)(OtaConfig_BankAddr(s_target_bank) + real_size), OTA_ECDSA_SIG_SIZE);
    log_i("Verifying ECDSA signature (%lu bytes firmware)...", real_size);

    HAL_IWDG_Refresh(&hiwdg);

    if (OtaEcdsa_Verify(OtaConfig_BankAddr(s_target_bank), real_size, sig) != 0)
    {
        return -1;
    }

    *p_real_fw_size = real_size;
    return 0;
}

/**
 * @brief  Phase 4: Compute SHA-256 hash of Slot B firmware.
 *
 * Steps:
 *  1. Set state VERIFYING.
 *  2. Call OtaVerify_SHA256Flash to hash firmware data.
 *  3. Print 32-byte hex hash.
 *
 * @param[out] hash : Output 32-byte SHA-256 hash.
 *
 * @return  0 : success.
 * */
static int VerifyFirmware(uint8_t hash[32])
{
    s_state = OTA_TASK_VERIFYING;
    log_i("Verifying SHA-256...");

    OtaVerify_SHA256Flash(OtaConfig_BankAddr(s_target_bank), s_fw_size, hash);

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
 * @brief  Phase 5: Update OTA Config in Flash.
 *
 * Steps:
 *  1. Read existing config or create default.
 *  2. Set state UPGRADE_PENDING, write fw_size and SHA-256.
 *  3. Record current version, increment upgrade_count.
 *  4. Write back to Flash.
 *
 * @param[in] hash : SHA-256 hash (32 bytes).
 *
 * @return   0 : success.
 * @return  -1 : config write failed.
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

    log_i("Config updated.");
    return 0;
}

/**
 * @brief  Force rollback: set OTA_STATE_ROLLBACK and reboot.
 *
 * Steps:
 *  1. Read existing config or create default.
 *  2. Set state ROLLBACK, clear boot_count.
 *  3. Write config and reboot.
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
    NVIC_SystemReset();
}

/**
 * @brief  Phase 6: Reboot device.
 *
 * Steps:
 *  1. Set state REBOOT_PENDING, LED very fast blink 10Hz.
 *  2. Wait 1s for UART log to flush.
 *  3. Software reset.
 * */
static void RebootDevice(void)
{
    s_state = OTA_TASK_REBOOT_PENDING;
    OtaLed_SetMode(OTA_LED_REBOOTING);
    log_i("Upgrade ready. Rebooting in 1s...");

    osDelay(1000);
    NVIC_SystemReset();
}

/**
 * @brief  OTA error handler: show error code LED pattern, then reboot.
 *
 * Steps:
 *  1. Map result code to error code.
 *  2. Set LED error blink pattern (N blinks + 2s pause).
 *  3. Drive LED timebase for 3s to show error pattern.
 *  4. Software reset.
 *
 * @param[in] result : Negative result code from phase chain.
 * */
static void HandleError(int result)
{
    int      i;
    uint8_t  code;

    (void)result;

    s_state = OTA_TASK_ERROR;

    code = OtaLed_GetErrorCode();

    log_e("ERROR! Code=%u, %s. Rebooting in 3s...",
          code,
          code == OTA_ERR_ERASE_FAILED  ? "Erase failed" :
          code == OTA_ERR_YMODEM_FAILED ? "YMODEM failed" :
          code == OTA_ERR_ECDSA_FAILED  ? "ECDSA failed" :
          code == OTA_ERR_SHA256_FAILED ? "SHA-256 failed" :
          "Config failed");

    /* Drive LED error pattern for 3 seconds */
    for (i = 0; i < 60; i++)
    {
        OtaLed_TimebaseHook();
        osDelay(50);
    }

    NVIC_SystemReset();
}

//*** Public API ***//

/**
 * @brief  Initialize OTA subsystem and EasyLogger.
 *
 * Steps:
 *  1. Reset OTA state, firmware size, trigger flag.
 *  2. Initialize Flash driver.
 *  3. Configure EasyLogger log formats and start.
 * */
void TaskOta_Init(void)
{
    static uint8_t s_uart_rx_buf[2048];
    static const bsp_uart_config_t uart_cfg = {
        &huart1, s_uart_rx_buf, sizeof(s_uart_rx_buf), NULL, NULL
    };

    s_state        = OTA_TASK_NORMAL;
    s_fw_size      = 0;
    s_trigger_flag = 0;
    s_force_rollback = 0;
    BspFlash_Init();

    /* 初始化 UART 驱动 + ring buffer */
    BspUart_Init(&s_uart_drv, &uart_cfg);
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
}

/**
 * @brief  Get current OTA task state.
 *
 * @return  ota_task_state_t : Current task state.
 * */
ota_task_state_t TaskOta_GetState(void)
{
    return s_state;
}

//*** Task Entry ***//

/**
 * @brief  OTA task entry function (FreeRTOS task).
 *
 * Steps:
 *  1. Call TaskOta_Init to init subsystem and logger.
 *  2. Run self-check (UART + Flash).
 *  3. Init LED module and key driver.
 *  4. Wait for trigger (short press = OTA, long press = rollback).
 *  5. If rollback: set OTA_STATE_ROLLBACK and reboot.
 *  6. Otherwise: chain 6 phases (erase→receive→ECDSA→SHA256→config→reboot).
 *  7. Any phase failure: show error LED pattern and reboot.
 *
 * @param[in] p_argument : Unused (pass NULL).
 * */
void TaskOta_Run(void *p_argument)
{
    uint8_t hash[32];
    int     result;

    (void)p_argument;
    TaskOta_Init();

    /* Self-check */
    if (SelfCheck() != 0)
    {
        ForceRollback();
    }

    /* LED + Key init */
    OtaLed_Init();

    {
        static const bsp_key_config_t key_cfg = { GPIOA, GPIO_PIN_0, 0, 20, 3000, OnKey, NULL };
        BspKey_Init(&s_key, &key_cfg);
    }

    /* Wait for trigger (short press = OTA, long press = rollback) */
    result = WaitForTrigger();
    if (result == -2)
    {
        ForceRollback();
    }

    /* 确定目标 Bank：下载到非活跃 Bank */
    {
        ota_config_t cfg;
        ota_slot_t   active;

        if (BspFlash_ReadConfig(&cfg) == 0)
        {
            active = cfg.active_slot;
        }
        else
        {
            active = OTA_SLOT_A;
        }

        s_target_bank = OtaConfig_Other(active);
        log_i("Active=%c, Target=%c",
              active == OTA_SLOT_A ? 'A' : 'B',
              s_target_bank == OTA_SLOT_A ? 'A' : 'B');
    }

    /* Chain phase calls — map each failure to specific error code */
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

    result = VerifyEcdsaSignature(&s_fw_size);
    if (result != 0)
    {
        OtaLed_SetErrorCode(OTA_ERR_ECDSA_FAILED);
        HandleError(result);
    }

    result = VerifyFirmware(hash);
    if (result != 0)
    {
        OtaLed_SetErrorCode(OTA_ERR_SHA256_FAILED);
        HandleError(result);
    }

    result = UpdateConfig(hash);
    if (result != 0)
    {
        OtaLed_SetErrorCode(OTA_ERR_CONFIG_FAILED);
        HandleError(result);
    }

    RebootDevice();

    vTaskDelete(NULL);
}
