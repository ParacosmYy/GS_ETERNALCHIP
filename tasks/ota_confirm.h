/**
 * @file    ota_confirm.h
 * @brief   OTA boot confirmation — App notifies Bootloader that boot succeeded
 */

#ifndef OTA_CONFIRM_H
#define OTA_CONFIRM_H

/**
 * @brief  确认启动成功，通知 Bootloader
 *
 *         读取 OTA Config，若状态为 CONFIRMING，则置为 CONFIRMED
 *         并清零 boot_count。在 App 初始化完成后调用一次。
 */
void OTA_ConfirmBoot(void);

#endif /* OTA_CONFIRM_H */
