/**
 * @file    task_ota.c
 * @brief   OTA upgrade task implementation
 *
 * Runs as a FreeRTOS task. Waits 5 seconds for trigger, then receives
 * firmware via YMODEM, writes to Slot B, verifies SHA-256, and reboots.
 */

//*** Includes ***//
#include "task_ota.h"
#include "bsp_flash.h"
#include "bsp_rtt.h"
#include "sha256.h"
#include "ymodem.h"
#include "usart.h"
#include "iwdg.h"
#include "cmsis_os.h"
#include <string.h>

//*** Debug Switch ***//
#define TASK_OTA_DEBUG  1

#if TASK_OTA_DEBUG
#define DBG_PRINT(fmt, ...)  BspRtt_Printf(fmt, ##__VA_ARGS__)
#else
#define DBG_PRINT(fmt, ...)  ((void)0)
#endif

//*** Private Variables ***//

static ota_task_state_t s_state;
static uint32_t         s_fw_size;
static volatile uint8_t s_trigger_flag;
static volatile uint8_t s_exit_flag;

//*** UART Adapters for YMODEM (polling mode) ***//

static void uart_send_byte(uint8_t byte, void *p_user)
{
    (void)p_user;
    HAL_UART_Transmit(&huart1, &byte, 1, 100);
}

static int uart_recv_byte(uint8_t *p_byte, uint32_t timeout_ms, void *p_user)
{
    (void)p_user;
    return (HAL_UART_Receive(&huart1, p_byte, 1, timeout_ms) == HAL_OK) ? 0 : -1;
}

//*** YMODEM Data Callback ***//

static int ymodem_data_callback(uint32_t offset, const uint8_t *p_data,
                                uint16_t len, void *p_user)
{
    (void)p_user;

    uint32_t addr = FLASH_ADDR_SLOT_B + offset - len;
    if (BspFlash_Write(addr, p_data, len) != 0) {
        DBG_PRINT("[OTA] Flash write error at 0x%08lX\r\n", addr);
        return -1;
    }

    /* Progress every 10 KB */
    static uint32_t s_last_print = 0;
    if (offset - s_last_print >= 10240u) {
        DBG_PRINT("[OTA] %lu / %lu bytes\r\n", offset, s_fw_size);
        s_last_print = offset;
    }

    HAL_IWDG_Refresh(&hiwdg);

    if (s_exit_flag) return -1;

    return 0;
}

//*** SHA-256 of Flash Region ***//

static int sha256_flash_region(uint32_t addr, uint32_t len, uint8_t hash[32])
{
    SHA256_CTX ctx;
    sha256_init(&ctx);

    uint8_t buf[256];
    for (uint32_t i = 0; i < len; i += sizeof(buf)) {
        uint32_t chunk = len - i;
        if (chunk > sizeof(buf)) chunk = sizeof(buf);
        memcpy(buf, (const void *)(addr + i), chunk);
        sha256_update(&ctx, buf, chunk);
    }

    sha256_final(&ctx, hash);
    return 0;
}

//*** Public API ***//

void TaskOta_Init(void)
{
    s_state       = OTA_TASK_NORMAL;
    s_fw_size     = 0;
    s_trigger_flag = 0;
    s_exit_flag    = 0;
    BspFlash_Init();
}

ota_task_state_t TaskOta_GetState(void)
{
    return s_state;
}

void TaskOta_NotifyKeyTrigger(void)  { s_trigger_flag = 1; }
void TaskOta_NotifyUartTrigger(void) { s_trigger_flag = 1; }
void TaskOta_NotifyExit(void)        { s_exit_flag = 1; }

//*** Task Entry ***//

void TaskOta_Run(void *p_argument)
{
    (void)p_argument;
    TaskOta_Init();

    int       ret;
    int       cfg_ret;
    uint8_t   hash[32];
    ota_config_t cfg;
    uint32_t  start;

    /* ---- Phase 1: 5-second trigger window ---- */
    s_state = OTA_TASK_TRIGGER_WAIT;
    DBG_PRINT("[OTA] Waiting for trigger (5s)...\r\n");

    start = xTaskGetTickCount();
    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(5000)) {
        if (s_trigger_flag) break;
        if (s_exit_flag) {
            s_state = OTA_TASK_NORMAL;
            DBG_PRINT("[OTA] Exit during trigger window.\r\n");
            vTaskDelete(NULL);
            return;
        }
        osDelay(50);
    }

    if (!s_trigger_flag) {
        s_state = OTA_TASK_NORMAL;
        DBG_PRINT("[OTA] No trigger. Task exiting.\r\n");
        vTaskDelete(NULL);
        return;
    }

    DBG_PRINT("[OTA] Triggered! Starting upgrade...\r\n");

    /* ---- Phase 2: Erase Slot B ---- */
    s_state = OTA_TASK_PREPARING;
    DBG_PRINT("[OTA] Erasing Slot B...\r\n");

    if (BspFlash_EraseSlot(OTA_SLOT_B) != 0) {
        DBG_PRINT("[OTA] Erase failed!\r\n");
        goto error;
    }
    DBG_PRINT("[OTA] Slot B erased.\r\n");

    /* ---- Phase 3: YMODEM receive ---- */
    s_state = OTA_TASK_RECEIVING;
    DBG_PRINT("[OTA] Waiting for YMODEM transfer...\r\n");

    s_exit_flag = 0;
    ret = Ymodem_Receive(uart_send_byte, uart_recv_byte,
                         NULL, &s_fw_size,
                         SLOT_MAX_APP_SIZE,
                         ymodem_data_callback, NULL);

    if (ret != YMODEM_OK) {
        DBG_PRINT("[OTA] YMODEM error: %d\r\n", ret);
        goto error;
    }
    DBG_PRINT("[OTA] Received %lu bytes.\r\n", s_fw_size);

    /* ---- Phase 4: SHA-256 verify ---- */
    s_state = OTA_TASK_VERIFYING;
    DBG_PRINT("[OTA] Verifying SHA-256...\r\n");

    sha256_flash_region(FLASH_ADDR_SLOT_B, s_fw_size, hash);

    /* Print hash for debug */
    DBG_PRINT("[OTA] SHA-256: ");
    {
        int i;
        for (i = 0; i < 32; i++) {
            DBG_PRINT("%02X", hash[i]);
        }
    }
    DBG_PRINT("\r\n");

    /* ---- Phase 5: Update Config ---- */
    s_state = OTA_TASK_UPDATING_CONFIG;

    cfg_ret = BspFlash_ReadConfig(&cfg);

    if (cfg_ret != 0) {
        /* First-time or corrupt config — initialise fresh */
        memset(&cfg, 0, sizeof(cfg));
        cfg.magic   = OTA_CONFIG_MAGIC;
        cfg.version = OTA_CONFIG_VERSION;
        cfg.active_slot = OTA_SLOT_A;
    }

    cfg.state          = OTA_STATE_UPGRADE_PENDING;
    cfg.fw_size        = s_fw_size;
    memcpy(cfg.fw_sha256, hash, 32);
    cfg.boot_count     = 0;
    cfg.upgrade_count += 1;

    if (BspFlash_WriteConfig(&cfg) != 0) {
        DBG_PRINT("[OTA] Config write failed!\r\n");
        goto error;
    }
    DBG_PRINT("[OTA] Config updated.\r\n");

    /* ---- Phase 6: Reboot ---- */
    s_state = OTA_TASK_REBOOT_PENDING;
    DBG_PRINT("[OTA] Upgrade ready. Rebooting in 3s...\r\n");
    osDelay(3000);
    NVIC_SystemReset();

    /* Should never reach here */
    vTaskDelete(NULL);
    return;

error:
    s_state = OTA_TASK_ERROR;
    DBG_PRINT("[OTA] ERROR! Long-press key or send UART cmd to exit.\r\n");

    s_exit_flag = 0;
    while (!s_exit_flag) {
        HAL_IWDG_Refresh(&hiwdg);
        osDelay(100);
    }

    s_state = OTA_TASK_NORMAL;
    DBG_PRINT("[OTA] Exited. Task ending.\r\n");
    vTaskDelete(NULL);
}
