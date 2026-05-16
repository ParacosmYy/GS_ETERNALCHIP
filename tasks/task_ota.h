/**
 * @file    task_ota.h
 * @brief   OTA upgrade task — receives firmware via YMODEM, writes to Slot B
 *
 * Flow:
 *   1. 5-second trigger window (key short press or UART "OTA" command)
 *   2. Erase Slot B
 *   3. YMODEM receive → write to Slot B
 *   4. SHA-256 verify
 *   5. Update Config (UPGRADE_PENDING)
 *   6. Software reset → Bootloader copies Slot B → Slot A
 */

#ifndef TASK_OTA_H
#define TASK_OTA_H

#include <stdint.h>

//*** OTA Task States ***//

typedef enum {
    OTA_TASK_NORMAL,
    OTA_TASK_TRIGGER_WAIT,
    OTA_TASK_PREPARING,
    OTA_TASK_RECEIVING,
    OTA_TASK_VERIFYING,
    OTA_TASK_UPDATING_CONFIG,
    OTA_TASK_REBOOT_PENDING,
    OTA_TASK_ERROR,
} ota_task_state_t;

//*** Public API ***//

void             TaskOta_Init(void);
void             TaskOta_Run(void *p_argument);
ota_task_state_t TaskOta_GetState(void);

void TaskOta_NotifyKeyTrigger(void);
void TaskOta_NotifyUartTrigger(void);
void TaskOta_NotifyExit(void);

#endif /* TASK_OTA_H */
