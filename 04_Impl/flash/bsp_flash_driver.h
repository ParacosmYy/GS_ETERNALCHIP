/**
 * @file    bsp_flash_driver.h
 * @brief   Internal Flash BSP driver — HAL types + platform interface
 * @author  GS_Mark
 *
 * @par dependencies
 * - stm32f4xx_hal.h
 * - plat_flash.h
 *
 * @note    This header provides HAL-specific definitions needed by the
 *          driver implementation. Upper layers should include plat_flash.h
 *          exclusively — this file is for Impl-layer consumption only.
 */

#ifndef BSP_FLASH_DRIVER_H
#define BSP_FLASH_DRIVER_H

#include "stm32f4xx_hal.h"
#include "plat_flash.h"

/* F411 sector count */
#define BSP_FLASH_SECTOR_COUNT  8

/* F411 voltage range for erase operations */
#define BSP_FLASH_VOLTAGE_RANGE  FLASH_VOLTAGE_RANGE_3

#endif /* BSP_FLASH_DRIVER_H */
