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
#include "ota_config.h"

//*** Public API ***//

/** @brief  初始化 Flash 驱动（当前为空操作，HAL 已完成初始化） */
void BspFlash_Init(void);

/**
 * @brief  擦除单个 Flash 扇区
 * @param  sector_num  扇区编号 (FLASH_SECTOR_0 ~ FLASH_SECTOR_7)
 * @return 0 成功, -1 扇区无效或 HAL 错误
 */
int BspFlash_EraseSector(uint32_t sector_num);

/**
 * @brief  擦除 OTA 分区对应的所有扇区
 * @param  slot  OTA_SLOT_A 或 OTA_SLOT_B
 * @return 0 成功, -1 分区无效或 HAL 错误
 */
int BspFlash_EraseSlot(ota_slot_t slot);

/**
 * @brief  向 Flash 写入数据（双字节对齐，内部处理末尾补 0xFF）
 * @param  addr    目标地址（必须在 Flash 范围内）
 * @param  p_data  源数据缓冲区
 * @param  len     写入字节数
 * @return 0 成功, -1 参数无效或 HAL 错误
 */
int BspFlash_Write(uint32_t addr, const uint8_t *p_data, uint32_t len);

/**
 * @brief  从 Flash 读取数据（内存映射直接 memcpy）
 * @param  addr   源地址
 * @param  p_data 目标缓冲区
 * @param  len    读取字节数
 */
void BspFlash_Read(uint32_t addr, uint8_t *p_data, uint32_t len);

/**
 * @brief  从 Sector 1 读取 OTA 配置，校验 magic + CRC-32
 * @param  p_cfg  输出配置结构体（调用者分配）
 * @return 0 成功, -1 magic 不匹配, -2 CRC 校验失败
 */
int BspFlash_ReadConfig(ota_config_t *p_cfg);

/**
 * @brief  写入 OTA 配置到 Sector 1（擦除后重写）
 *         自动计算并填充 CRC-32
 * @param  p_cfg  待写入的配置结构体
 * @return 0 成功, -1 失败
 */
int BspFlash_WriteConfig(const ota_config_t *p_cfg);

/**
 * @brief  获取扇区大小（字节）
 * @param  sector_num  扇区编号 (0-7)
 * @return 扇区大小, 无效返回 0
 */
uint32_t BspFlash_GetSectorSize(uint32_t sector_num);

/**
 * @brief  获取 OTA 分区起始地址
 * @param  slot  OTA_SLOT_A 或 OTA_SLOT_B
 * @return Flash 地址, 无效返回 0
 */
uint32_t BspFlash_GetSlotAddress(ota_slot_t slot);

/**
 * @brief  检查 Flash 区域是否全为 0xFF（已擦除）
 * @param  addr  起始地址
 * @param  len   检查字节数
 * @return 1 已擦除, 0 非空
 */
int BspFlash_IsEmpty(uint32_t addr, uint32_t len);

#endif /* BSP_FLASH_H */
