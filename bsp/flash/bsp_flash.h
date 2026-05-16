/**
 * @file    bsp_flash.h
 * @brief   Internal Flash BSP driver — partition management, Config R/W
 *
 * Partition layout (STM32F411CEUx, 512 KB):
 *   Sector 0 (16KB)  0x08000000  Bootloader
 *   Sector 1 (16KB)  0x08004000  App Config
 *   Sector 2-5(224KB)0x08008000  Slot A (active app)
 *   Sector 6-7(256KB)0x08040000  Slot B (download area)
 */

#ifndef BSP_FLASH_H
#define BSP_FLASH_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

//*** Partition Addresses ***//

#define FLASH_ADDR_BOOT        0x08000000u
#define FLASH_ADDR_CONFIG      0x08004000u
#define FLASH_ADDR_SLOT_A      0x08008000u
#define FLASH_ADDR_SLOT_B      0x08040000u

#define SLOT_A_SIZE            (224u * 1024u)
#define SLOT_B_SIZE            (256u * 1024u)
#define SLOT_MAX_APP_SIZE      SLOT_A_SIZE

//*** OTA Config Types ***//

#define OTA_CONFIG_MAGIC       0x4F544131u   /* "OTA1" */
#define OTA_CONFIG_VERSION     1u

typedef enum {
    OTA_SLOT_A = 0,
    OTA_SLOT_B = 1,
} ota_slot_t;

typedef enum {
    OTA_STATE_IDLE             = 0,
    OTA_STATE_UPGRADE_PENDING  = 1,
    OTA_STATE_COPY_IN_PROGRESS = 2,
    OTA_STATE_CONFIRMED        = 3,
} ota_state_t;

typedef struct {
    uint32_t    magic;
    uint32_t    version;
    ota_slot_t  active_slot;
    ota_state_t state;
    uint32_t    fw_size;
    uint8_t     fw_sha256[32];
    uint32_t    boot_count;
    uint32_t    upgrade_count;
    uint32_t    crc32;
} ota_config_t;

//*** Public API ***//

void     BspFlash_Init(void);

int      BspFlash_EraseSector(uint32_t sector_num);
int      BspFlash_EraseSlot(ota_slot_t slot);

int      BspFlash_Write(uint32_t addr, const uint8_t *p_data, uint32_t len);
void     BspFlash_Read(uint32_t addr, uint8_t *p_data, uint32_t len);

int      BspFlash_ReadConfig(ota_config_t *p_cfg);
int      BspFlash_WriteConfig(const ota_config_t *p_cfg);

uint32_t BspFlash_GetSectorSize(uint32_t sector_num);
uint32_t BspFlash_GetSlotAddress(ota_slot_t slot);
int      BspFlash_IsEmpty(uint32_t addr, uint32_t len);

#endif /* BSP_FLASH_H */
