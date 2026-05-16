/**
 * @file    ota_confirm.h
 * @brief   OTA boot confirmation — App notifies Bootloader that boot succeeded
 */

#ifndef OTA_CONFIRM_H
#define OTA_CONFIRM_H

/**
 * @brief  Confirm successful boot to Bootloader
 *
 *         Reads OTA Config; if state is CONFIRMING, sets CONFIRMED
 *         and resets boot_count. Called once after App init completes.
 */
void OTA_ConfirmBoot(void);

#endif /* OTA_CONFIRM_H */
