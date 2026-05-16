/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 * 
 * @file ota_config.h
 * 
 * @par dependencies 
 * - stdint.h
 * -stddef.h
 * 
 * @author GS_Mark
 *
 * @brief OTA partition layout, config structure and state definitions.
 *        Single source of truth shared by Bootloader and App projects.
 * 
 * Flash Partition Layout (STM32F411CEUx, 512 KB):
 *   Sector 0  (16KB,  0x08000000)  Bootloader
 *   Sector 1  (16KB,  0x08004000)  OTA Config (this struct + CRC-32)
 *   Sector 2-5(224KB, 0x08008000)  Slot A (active app)
 *   Sector 6-7(256KB, 0x08040000)  Slot B (download staging)
 * 
 * State Machine:
 *   IDLE --(App YMODEM receive)--> UPGRADE_PENDING
 *                                       |
 *                                 reset |
 *                                       v
 *                             Bootloader copies B->A
 *                                       |
 *                                       v
 *                                CONFIRMING ---- boot_count>=3 --> ROLLBACK
 *                                   |                              |
 *                              App  |                        Recovery YMODEM
 *                           confirm |                              |
 *                                   v                              v
 *                               CONFIRMED                     CONFIRMING
 * 
 * @version V1.0 2025-05-17
 * 
 * @note ARM Compiler V5 compatible -- English only in this file.
 * @note 1 tab == 4 spaces
 * 
 *****************************************************************************/
#ifndef OTA_CONFIG_H
#define OTA_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include <stdint.h>
#include <stddef.h>
//******************************* Includes **********************************//

//******************************* Defines ***********************************//

/** @brief Flash partition base addresses */
#define FLASH_ADDR_BOOT   0x08000000u
#define FLASH_ADDR_CONFIG 0x08004000u
#define FLASH_ADDR_SLOT_A 0x08008000u
#define FLASH_ADDR_SLOT_B 0x08040000u

/** @brief Slot sizes in bytes */
#define SLOT_A_SIZE       (224u * 1024u)   /* Sectors 2-5: 4*16K + 64K + 128K */
#define SLOT_B_SIZE       (256u * 1024u)   /* Sectors 6-7: 2*128K */
#define SLOT_MAX_APP_SIZE SLOT_A_SIZE      /* App must fit in Slot A */

/** @brief OTA Config magic and version */
#define OTA_CONFIG_MAGIC   0x4F544131u     /* "OTA1" */
#define OTA_CONFIG_VERSION 1u

/** @brief Max unconfirmed boots before entering recovery */
#define OTA_BOOT_COUNT_THRESHOLD 3u

//******************************* Types *************************************//

/** @brief OTA slot identifier */
typedef enum
{
    OTA_SLOT_A = 0,  /**< Active app slot */
    OTA_SLOT_B = 1,  /**< Download staging slot */
} ota_slot_t;

/** @brief OTA state machine states */
typedef enum
{
    OTA_STATE_IDLE             = 0,  /**< Normal operation */
    OTA_STATE_UPGRADE_PENDING  = 1,  /**< App received firmware, waiting for Bootloader copy */
    OTA_STATE_COPY_IN_PROGRESS = 2,  /**< Reserved for compatibility */
    OTA_STATE_CONFIRMED        = 3,  /**< App confirmed successful boot */
    OTA_STATE_CONFIRMING       = 4,  /**< Bootloader copied, waiting for App confirmation */
    OTA_STATE_ROLLBACK         = 5,  /**< Boot failed, enter recovery mode */
} ota_state_t;

/** @brief OTA config structure stored in Sector 1 (CRC-32 protected) */
typedef struct
{
    uint32_t    magic;          /**< Must equal OTA_CONFIG_MAGIC */
    uint32_t    version;        /**< Config structure version */
    ota_slot_t  active_slot;    /**< Currently active slot (always OTA_SLOT_A) */
    ota_state_t state;          /**< Current OTA state */
    uint32_t    fw_size;        /**< Firmware size in bytes */
    uint8_t     fw_sha256[32];  /**< SHA-256 hash of firmware */
    uint32_t    boot_count;     /**< Unconfirmed boot counter */
    uint32_t    upgrade_count;  /**< Total upgrade counter */
    uint32_t    crc32;          /**< CRC-32 over all fields above this */
} ota_config_t;

//******************************* Declaring *********************************//

#ifdef __cplusplus
}
#endif

#endif /* OTA_CONFIG_H */
