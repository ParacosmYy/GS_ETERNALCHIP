/**
 * @file    bsp_flash_driver.h
 * @brief   内部 Flash BSP 驱动 — HAL 类型 + 平台接口
 * @author  GS_Mark
 *
 * @par dependencies
 * - stm32f4xx_hal.h
 * - plat_flash.h
 *
 * @note    本头文件提供驱动实现所需的 HAL 特定定义。
 *          上层应仅包含 plat_flash.h — 本文件仅供 Impl 层内部使用。
 */

#ifndef BSP_FLASH_DRIVER_H
#define BSP_FLASH_DRIVER_H

#include "stm32f4xx_hal.h"
#include "plat_flash.h"

/* F411 扇区数量 */
#define BSP_FLASH_SECTOR_COUNT  8

/* F411 擦除操作电压范围 */
#define BSP_FLASH_VOLTAGE_RANGE  FLASH_VOLTAGE_RANGE_3

#endif /* BSP_FLASH_DRIVER_H */
