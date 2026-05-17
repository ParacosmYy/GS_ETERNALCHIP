/**
 * @file    bsp_flash.h
 * @brief   Internal Flash BSP driver — partition management, Config R/W
 */

#ifndef BSP_FLASH_H
#define BSP_FLASH_H

#include "stm32f4xx_hal.h"
#include "ota_config.h"

//*** Public API ***//

/** @brief  初始化 Flash 驱动 */
void BspFlash_Init(void);

/** @brief  擦除单个 Flash 扇区 */
int BspFlash_EraseSector(uint32_t sector_num);

/** @brief  擦除 OTA 分区对应的所有扇区 */
int BspFlash_EraseSlot(ota_slot_t slot);

/** @brief  向 Flash 写入数据（双字对齐） */
int BspFlash_Write(uint32_t addr, const uint8_t *p_data, uint32_t len);

/** @brief  从 Flash 读取数据（内存映射） */
void BspFlash_Read(uint32_t addr, uint8_t *p_data, uint32_t len);

/** @brief  读取 OTA 配置（校验 magic + CRC-32） */
int BspFlash_ReadConfig(ota_config_t *p_cfg);

/** @brief  写入 OTA 配置（含 CRC + 读回验证） */
int BspFlash_WriteConfig(const ota_config_t *p_cfg);

/** @brief  获取扇区大小 */
uint32_t BspFlash_GetSectorSize(uint32_t sector_num);

/** @brief  获取分区起始地址 */
uint32_t BspFlash_GetSlotAddress(ota_slot_t slot);

/** @brief  检查 Flash 区域是否为空（全 0xFF） */
int BspFlash_IsEmpty(uint32_t addr, uint32_t len);

#endif /* BSP_FLASH_H */
