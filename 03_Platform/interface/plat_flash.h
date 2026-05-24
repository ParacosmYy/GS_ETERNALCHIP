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
 * @brief   平台 Flash 接口 — OPS 类型 + Driver API + Handler API
 *
 *          架构：
 *            Handler（门面，向后兼容 API）
 *              |-> Driver（扇区级，ops 注入）
 *                    +-> HW ops（解锁/锁定/擦除/编程）
 *                    +-> OS ops（临界区进入/退出）
 *                    +-> Time ops（获取 tick）
 *
 *          上层只包含本头文件，直接调用 Handler API ——
 *          签名与原始过程式 API 完全一致。
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
#include "plat_tick.h"
#include <stdint.h>
#include <stddef.h>
//******************************* Includes **********************************//

//******************************* Forward Declarations **********************//
typedef struct bsp_flash_driver bsp_flash_driver_t;

//******************************* OPS Types *********************************//

/** @brief  Flash 硬件操作 — 由 system_adaption 或 HAL 胶水层实现 */
typedef struct
{
    int  (*pf_unlock)(void);
    int  (*pf_lock)(void);
    void (*pf_clear_errors)(void);
    int  (*pf_erase_sector)(uint32_t sector, uint32_t voltage_range);
    int  (*pf_program_word)(uint32_t addr, uint32_t data);
} flash_hw_operations_t;

/** @brief  Flash OS 操作 — 临界区管理 */
typedef struct
{
    void (*pf_critical_enter)(void);
    void (*pf_critical_exit)(void);
} flash_os_operations_t;

/** @brief  Flash 驱动配置 — 芯片布局（无 HAL 类型） */
typedef struct
{
    uint32_t boot_addr;
    uint32_t config_addr;
    uint32_t slot_a_addr;
    uint32_t slot_b_addr;
    uint32_t slot_a_size;
    uint32_t slot_b_size;
} bsp_flash_config_t;

/** @brief  Flash 驱动实例 — 持有配置和 ops 指针 */
struct bsp_flash_driver
{
    const bsp_flash_config_t      *p_config;
    const flash_hw_operations_t   *p_hw_ops;
    const flash_os_operations_t   *p_os_ops;
    const tick_operations_t       *p_time_ops;
    uint8_t                        is_inited;
};

//******************************* Driver API ********************************//
/* 扇区级操作 — 内部使用，仅 Handler 调用 */

/** @brief  使用配置和 ops 初始化驱动实例 */
void     BspFlash_DriverInit(bsp_flash_driver_t *p_drv,
                             const bsp_flash_config_t *p_config,
                             const flash_hw_operations_t *p_hw_ops,
                             const flash_os_operations_t *p_os_ops,
                             const tick_operations_t *p_time_ops);

/** @brief  擦除单个 Flash 扇区 */
int      BspFlash_DriverEraseSector(bsp_flash_driver_t *p_drv, uint32_t sector);

/** @brief  向 Flash 写入数据（内部按 4 字节对齐） */
int      BspFlash_DriverWrite(bsp_flash_driver_t *p_drv,
                              uint32_t addr, const uint8_t *p_data, uint32_t len);

/** @brief  从 Flash 读取数据（内存映射 memcpy） */
void     BspFlash_DriverRead(bsp_flash_driver_t *p_drv,
                             uint32_t addr, uint8_t *p_data, uint32_t len);

/** @brief  获取指定扇区号的大小（字节） */
uint32_t BspFlash_DriverGetSectorSize(const bsp_flash_driver_t *p_drv,
                                      uint32_t sector);

//******************************* Handler API *******************************//
/* 分区级 — 签名与旧版过程式 API 完全一致，向后兼容 */

/** @brief  初始化 Flash 驱动（清除残留错误标志） */
void     BspFlash_Init(void);

/** @brief  擦除 OTA 槽位对应的所有扇区 */
int      BspFlash_EraseSlot(ota_slot_t slot);

/** @brief  向 Flash 写入数据（4 字节对齐，尾部填充 0xFF） */
int      BspFlash_Write(uint32_t addr, const uint8_t *p_data, uint32_t len);

/** @brief  从 Flash 读取数据（内存映射直接读取） */
void     BspFlash_Read(uint32_t addr, uint8_t *p_data, uint32_t len);

/** @brief  从扇区 1 读取 OTA 配置，校验 magic + CRC-32 */
int      BspFlash_ReadConfig(ota_config_t *p_cfg);

/** @brief  向扇区 1 写入 OTA 配置（读-改-写 + CRC） */
int      BspFlash_WriteConfig(const ota_config_t *p_cfg);

/** @brief  清除 OTA 追踪区为 0xFF，保留 Config 和 Crash Dump */
int      BspFlash_ClearTrace(void);

/** @brief  获取扇区大小（字节） */
uint32_t BspFlash_GetSectorSize(uint32_t sector_num);

/** @brief  获取 OTA 槽位起始地址 */
uint32_t BspFlash_GetSlotAddress(ota_slot_t slot);

/** @brief  检查 Flash 区域是否全为 0xFF（已擦除） */
int      BspFlash_IsEmpty(uint32_t addr, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* PLAT_FLASH_H */
