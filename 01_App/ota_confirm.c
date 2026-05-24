/**
 * @file    ota_confirm.c
 * @brief   OTA boot confirmation implementation
 * @author  GS_Mark
 *
 * @par dependencies
 * - elog.h
 * - ota_confirm.h
 * - bsp_flash.h
 * - task_ota.h
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
 * @brief  确认 OTA 启动成功，通知 Bootloader 无需回滚。
 *
 * Steps:
 *  1. 从 Flash 读取 OTA 配置，失败则直接返回。
 *  2. 检查状态是否为 OTA_STATE_CONFIRMING（Bootloader 置位）。
 *  3. 对比 prev_fw_version 与当前版本，打印升级结果。
 *  4. 更新状态为 OTA_STATE_CONFIRMED，清零 boot_count。
 *  5. 写回 Flash Config 区。
 *
 * @note  应在应用程序启动后尽早调用此函数。
 * */
void OTA_ConfirmBoot(void)
{
    ota_config_t cfg;

    if (BspFlash_ReadConfig(&cfg) != 0)
    {
        return;
    }

    /* 仅在 Bootloader 标记为 CONFIRMING 时处理 */
    if (cfg.state == OTA_STATE_CONFIRMING)
    {
        /* 版本不同表示确实发生了升级 */
        if (strncmp(cfg.prev_fw_version, OTA_FW_VERSION, sizeof(cfg.prev_fw_version)) != 0)
        {
            log_i("OTA OK: %s -> %s", cfg.prev_fw_version, OTA_FW_VERSION);
        }
        else
        {
            log_i("OTA OK: version unchanged (%s)", OTA_FW_VERSION);
        }

        /* 标记为已确认，防止 Bootloader 下次启动回滚 */
        cfg.state      = OTA_STATE_CONFIRMED;
        cfg.boot_count = 0;
        BspFlash_WriteConfig(&cfg);
        log_i("Boot confirmed.");
    }
}
