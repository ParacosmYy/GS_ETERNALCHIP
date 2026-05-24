/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    plat_flash.h
 *
 * @par dependencies
 * - stdint.h
 * - ota_config.h (Shared/)
 *
 * @author  GS_Mark
 *
 * @brief   Platform Flash interface — no HAL types in header
 *
 * @version V1.0 2025-05-24
 *
 * @note    1 tab == 4 spaces
 *
 *****************************************************************************/
#ifndef PLAT_FLASH_H
#define PLAT_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include "ota_config.h"
#include <stdint.h>
//******************************* Includes **********************************//

//******************************* Declaring ********************************//
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

#ifdef __cplusplus
}
#endif

#endif /* PLAT_FLASH_H */
