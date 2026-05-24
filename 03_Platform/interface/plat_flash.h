/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    plat_flash.h
 *
 * @par dependencies
 * - stdint.h
 * - stddef.h
 * - ota_config.h (Shared/)
 *
 * @author  GS_Mark
 *
 * @brief   Platform Flash interface — OPS types + Driver API + Handler API
 *
 *          Architecture:
 *            Handler (facade, backward-compatible API)
 *              |-> Driver (sector-level, ops-injected)
 *                    +-> HW ops  (unlock/lock/erase/program)
 *                    +-> OS ops  (critical enter/exit)
 *                    +-> Time ops (get_tick)
 *
 *          Upper layers include ONLY this header. They call Handler API
 *          functions directly — signatures are identical to the original
 *          procedural API.
 *
 * @version V2.0 2025-05-24
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
#include <stddef.h>
//******************************* Includes **********************************//

//******************************* Forward Declarations **********************//
typedef struct bsp_flash_driver bsp_flash_driver_t;

//******************************* OPS Types *********************************//

/** @brief  Flash hardware operations — implemented by system_adaption or HAL glue */
typedef struct
{
    int  (*pf_unlock)(void);
    int  (*pf_lock)(void);
    void (*pf_clear_errors)(void);
    int  (*pf_erase_sector)(uint32_t sector, uint32_t voltage_range);
    int  (*pf_program_word)(uint32_t addr, uint32_t data);
} flash_hw_operations_t;

/** @brief  Flash OS operations — critical section management */
typedef struct
{
    void (*pf_critical_enter)(void);
    void (*pf_critical_exit)(void);
} flash_os_operations_t;

/** @brief  Flash time base operations */
typedef struct
{
    uint32_t (*pf_get_tick)(void);
} flash_time_operations_t;

/** @brief  Flash driver configuration — chip layout (no HAL types) */
typedef struct
{
    uint32_t boot_addr;
    uint32_t config_addr;
    uint32_t slot_a_addr;
    uint32_t slot_b_addr;
    uint32_t slot_a_size;
    uint32_t slot_b_size;
} bsp_flash_config_t;

/** @brief  Flash driver instance — holds config + ops pointers */
struct bsp_flash_driver
{
    const bsp_flash_config_t      *p_config;
    const flash_hw_operations_t   *p_hw_ops;
    const flash_os_operations_t   *p_os_ops;
    const flash_time_operations_t *p_time_ops;
    uint8_t                        is_inited;
};

//******************************* Driver API ********************************//
/* Sector-level operations — internal, only used by Handler */

/** @brief  Initialize driver instance with config and ops */
void     BspFlash_DriverInit(bsp_flash_driver_t *p_drv,
                             const bsp_flash_config_t *p_config,
                             const flash_hw_operations_t *p_hw_ops,
                             const flash_os_operations_t *p_os_ops,
                             const flash_time_operations_t *p_time_ops);

/** @brief  Erase a single Flash sector */
int      BspFlash_DriverEraseSector(bsp_flash_driver_t *p_drv, uint32_t sector);

/** @brief  Write data to Flash at byte level (4-byte aligned internally) */
int      BspFlash_DriverWrite(bsp_flash_driver_t *p_drv,
                              uint32_t addr, const uint8_t *p_data, uint32_t len);

/** @brief  Read data from Flash (memory-mapped memcpy) */
void     BspFlash_DriverRead(bsp_flash_driver_t *p_drv,
                             uint32_t addr, uint8_t *p_data, uint32_t len);

/** @brief  Get sector size in bytes for a given sector number */
uint32_t BspFlash_DriverGetSectorSize(const bsp_flash_driver_t *p_drv,
                                      uint32_t sector);

//******************************* Handler API *******************************//
/* Partition-level — SAME signatures as old procedural API for backward compat */

/** @brief  Initialize Flash driver (clear residual error flags) */
void     BspFlash_Init(void);

/** @brief  Erase all sectors belonging to an OTA slot */
int      BspFlash_EraseSlot(ota_slot_t slot);

/** @brief  Write data to Flash (4-byte aligned, tail padded with 0xFF) */
int      BspFlash_Write(uint32_t addr, const uint8_t *p_data, uint32_t len);

/** @brief  Read data from Flash (memory-mapped direct read) */
void     BspFlash_Read(uint32_t addr, uint8_t *p_data, uint32_t len);

/** @brief  Read OTA config from Sector 1, verify magic + CRC-32 */
int      BspFlash_ReadConfig(ota_config_t *p_cfg);

/** @brief  Write OTA config to Sector 1 (read-modify-write with CRC) */
int      BspFlash_WriteConfig(const ota_config_t *p_cfg);

/** @brief  Get sector size in bytes */
uint32_t BspFlash_GetSectorSize(uint32_t sector_num);

/** @brief  Get OTA slot start address */
uint32_t BspFlash_GetSlotAddress(ota_slot_t slot);

/** @brief  Check if a Flash region is all 0xFF (erased) */
int      BspFlash_IsEmpty(uint32_t addr, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* PLAT_FLASH_H */
