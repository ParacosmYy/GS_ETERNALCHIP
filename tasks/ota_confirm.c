/**
 * @file    ota_confirm.c
 * @brief   OTA boot confirmation implementation
 */

//*** Includes ***//
#define LOG_TAG "OTA"
#include "elog.h"
#include "ota_confirm.h"
#include "bsp_flash.h"
#include "task_ota.h"
#include <string.h>

//*** Public API ***//

/**
 * @brief  确认 OTA 启动成功
 *
 *         从 Flash 读取 OTA 配置，若当前状态为 OTA_STATE_CONFIRMING，
 *         则将其更新为 OTA_STATE_CONFIRMED 并清零启动计数，
 *         表示新固件运行正常，无需回滚
 *
 * @note   应在应用程序启动后尽早调用此函数
 */
void OTA_ConfirmBoot(void)
{
    ota_config_t cfg;

    if (BspFlash_ReadConfig(&cfg) != 0)
    {
        return;
    }

    if (cfg.state == OTA_STATE_CONFIRMING)
    {
        if (strncmp(cfg.prev_fw_version, OTA_FW_VERSION, sizeof(cfg.prev_fw_version)) != 0)
        {
            log_i("OTA OK: %s -> %s", cfg.prev_fw_version, OTA_FW_VERSION);
        }
        else
        {
            log_i("OTA OK: version unchanged (%s)", OTA_FW_VERSION);
        }

        cfg.state      = OTA_STATE_CONFIRMED;
        cfg.boot_count = 0;
        BspFlash_WriteConfig(&cfg);
        log_i("Boot confirmed.");
    }
}
