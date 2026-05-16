/**
 * @file    ota_confirm.c
 * @brief   OTA boot confirmation implementation
 */

//*** Includes ***//
#define LOG_TAG "OTA"
#include "elog.h"
#include "ota_confirm.h"
#include "bsp_flash.h"

//*** Public API ***//

void OTA_ConfirmBoot(void)
{
    ota_config_t cfg;

    if (BspFlash_ReadConfig(&cfg) != 0)
    {
        return;
    }

    if (cfg.state == OTA_STATE_CONFIRMING)
    {
        cfg.state      = OTA_STATE_CONFIRMED;
        cfg.boot_count = 0;
        BspFlash_WriteConfig(&cfg);
        log_i("Boot confirmed.");
    }
}
