/**
 * @file    bsp_flash.h
 * @brief   Internal Flash BSP driver — partition management, Config R/W
 * @author  GS_Mark
 */

#ifndef BSP_FLASH_H
#define BSP_FLASH_H

#include "stm32f4xx_hal.h"
#include "ota_config.h"

//*** Public API ***//

/** @brief  初始化 Flash 驱动（清除残留错误标志） */
void BspFlash_Init(void);

/** @brief  擦除单个 Flash 扇区 */
int BspFlash_EraseSector(uint32_t sector_num);

/** @brief  擦除 OTA 分区对应的所有扇区 */
int BspFlash_EraseSlot(ota_slot_t slot);

/** @brief  向 Flash 写入数据（4 字节对齐，末尾补 0xFF） */
int BspFlash_Write(uint32_t addr, const uint8_t *p_data, uint32_t len);

/** @brief  从 Flash 读取数据（内存映射直接 memcpy） */
void BspFlash_Read(uint32_t addr, uint8_t *p_data, uint32_t len);

/** @brief  从 Sector 1 读取 OTA 配置，校验 magic + CRC-32 */
int BspFlash_ReadConfig(ota_config_t *p_cfg);

/** @brief  写入 OTA 配置到 Sector 1（擦除后重写，自动填充 CRC-32） */
int BspFlash_WriteConfig(const ota_config_t *p_cfg);

/** @brief  获取扇区大小（字节） */
uint32_t BspFlash_GetSectorSize(uint32_t sector_num);

/** @brief  获取 OTA 分区起始地址 */
uint32_t BspFlash_GetSlotAddress(ota_slot_t slot);

/** @brief  检查 Flash 区域是否全为 0xFF（已擦除） */
int BspFlash_IsEmpty(uint32_t addr, uint32_t len);

#endif /* BSP_FLASH_H */
