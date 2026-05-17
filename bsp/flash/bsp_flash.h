/**
 * @file    bsp_flash.h
 * @brief   Internal Flash BSP driver — partition management, Config R/W
 *
 * Partition layout (STM32F411CEUx, 512 KB):
 *   Sector 0 (16KB)  0x08000000  Bootloader
 *   Sector 1 (16KB)  0x08004000  OTA Config
 *   Sector 2-5(224KB)0x08008000  Slot A (active app)
 *   Sector 6-7(256KB)0x08040000  Slot B (download area)
 */

#ifndef BSP_FLASH_H
#define BSP_FLASH_H

#include "stm32f4xx_hal.h"
#include "ota_config.h"

//*** Public API ***//

void BspFlash_Init(void);

int BspFlash_EraseSector(uint32_t sector_num);

int BspFlash_EraseSlot(ota_slot_t slot);

int BspFlash_Write(uint32_t addr, const uint8_t *p_data, uint32_t len);

void BspFlash_Read(uint32_t addr, uint8_t *p_data, uint32_t len);

int BspFlash_ReadConfig(ota_config_t *p_cfg);

int BspFlash_WriteConfig(const ota_config_t *p_cfg);

uint32_t BspFlash_GetSectorSize(uint32_t sector_num);

uint32_t BspFlash_GetSlotAddress(ota_slot_t slot);

int BspFlash_IsEmpty(uint32_t addr, uint32_t len);

#endif /* BSP_FLASH_H */
