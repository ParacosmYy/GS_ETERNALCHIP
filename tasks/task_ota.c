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
    uint32_t addr;

    (void)p_user;

    addr = FLASH_ADDR_SLOT_B + offset - len;
    if (BspFlash_Write(addr, p_data, len) != 0)
    {
        log_e("Flash write error at 0x%08lX", addr);
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

    if (s_exit_flag)
    {
        return -1;
    }

    return 0;
}

//*** SHA-256 of Flash Region ***//

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
    s_state        = OTA_TASK_NORMAL;
    s_fw_size      = 0;
    s_trigger_flag = 0;
    s_exit_flag    = 0;
    BspFlash_Init();

    /* Initialize EasyLogger */
    elog_init();
    elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL);
    elog_set_fmt(ELOG_LVL_ERROR,  ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_WARN,   ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_INFO,   ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_DEBUG,  ELOG_FMT_LVL | ELOG_FMT_TAG);
    elog_set_fmt(ELOG_LVL_VERBOSE,ELOG_FMT_TAG);
    elog_start();
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
    int            ret;
    int            cfg_ret;
    uint8_t        hash[32];
    ota_config_t   cfg;
    uint32_t       start;

    (void)p_argument;
    TaskOta_Init();

    /* ---- Phase 1: 5-second trigger window ---- */
    s_state = OTA_TASK_TRIGGER_WAIT;
    log_i("Waiting for trigger (5s)...");

    start = xTaskGetTickCount();
    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(5000))
    {
        if (s_trigger_flag) break;
        if (s_exit_flag)
        {
            s_state = OTA_TASK_NORMAL;
            log_i("Exit during trigger window.");
            vTaskDelete(NULL);
            return;
        }
        osDelay(50);
    }

    if (!s_trigger_flag)
    {
        s_state = OTA_TASK_NORMAL;
        log_i("No trigger. Task exiting.");
        vTaskDelete(NULL);
        return;
    }

    log_i("Triggered! Starting upgrade...");

    /* ---- Phase 2: Erase Slot B ---- */
    s_state = OTA_TASK_PREPARING;
    log_i("Erasing Slot B...");

    if (BspFlash_EraseSlot(OTA_SLOT_B) != 0)
    {
        log_e("Erase failed!");
        goto error;
    }
    log_i("Slot B erased.");

    /* ---- Phase 3: YMODEM receive ---- */
    s_state = OTA_TASK_RECEIVING;
    log_i("Waiting for YMODEM transfer...");

    s_exit_flag = 0;
    ret = Ymodem_Receive(uart_send_byte, uart_recv_byte,
                         NULL, &s_fw_size,
                         SLOT_MAX_APP_SIZE,
                         ymodem_data_callback, NULL);

    if (ret != YMODEM_OK)
    {
        log_e("YMODEM error: %d", ret);
        goto error;
    }
    log_i("Received %lu bytes.", s_fw_size);

    /* ---- Phase 4: SHA-256 verify ---- */
    s_state = OTA_TASK_VERIFYING;
    log_i("Verifying SHA-256...");

    sha256_flash_region(FLASH_ADDR_SLOT_B, s_fw_size, hash);

    /* Print hash for debug */
    log_i("SHA-256: %02X%02X%02X%02X%02X%02X%02X%02X"
          "%02X%02X%02X%02X%02X%02X%02X%02X"
          "%02X%02X%02X%02X%02X%02X%02X%02X"
          "%02X%02X%02X%02X%02X%02X%02X%02X",
          hash[0],hash[1],hash[2],hash[3],hash[4],hash[5],hash[6],hash[7],
          hash[8],hash[9],hash[10],hash[11],hash[12],hash[13],hash[14],hash[15],
          hash[16],hash[17],hash[18],hash[19],hash[20],hash[21],hash[22],hash[23],
          hash[24],hash[25],hash[26],hash[27],hash[28],hash[29],hash[30],hash[31]);

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

    cfg.state          = OTA_STATE_UPGRADE_PENDING;
    cfg.fw_size        = s_fw_size;
    memcpy(cfg.fw_sha256, hash, 32);
    cfg.boot_count     = 0;
    cfg.upgrade_count += 1;

    if (BspFlash_WriteConfig(&cfg) != 0)
    {
        log_e("Config write failed!");
        goto error;
    }
    log_i("Config updated.");

    /* ---- Phase 6: Reboot ---- */
    s_state = OTA_TASK_REBOOT_PENDING;
    log_i("Upgrade ready. Rebooting in 3s...");
    osDelay(3000);
    NVIC_SystemReset();

    /* Should never reach here */
    vTaskDelete(NULL);
    return;

error:
    s_state = OTA_TASK_ERROR;
    log_e("ERROR! Long-press key or send UART cmd to exit.");

    s_exit_flag = 0;
    while (!s_exit_flag)
    {
        HAL_IWDG_Refresh(&hiwdg);
        osDelay(100);
    }

    s_state = OTA_TASK_NORMAL;
    log_i("Exited. Task ending.");
    vTaskDelete(NULL);
}
