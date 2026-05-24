/**
 * @file    bsp_flash_handler.h
 * @brief   Flash Handler — partition-level facade over Flash Driver
 * @author  GS_Mark
 *
 * @par dependencies
 * - plat_flash.h
 *
 * @note    The Handler owns a static Driver instance and exposes the same
 *          API that upper layers already call. No caller changes needed.
 */

#ifndef BSP_FLASH_HANDLER_H
#define BSP_FLASH_HANDLER_H

#include "plat_flash.h"

/** @brief  Flash Handler — owns one driver instance */
typedef struct
{
    bsp_flash_driver_t *p_driver;
    uint8_t             is_inited;
} bsp_flash_handler_t;

/** @brief  Initialize handler with a pre-configured driver instance */
void BspFlashHandler_Init(bsp_flash_handler_t *p_handler,
                          bsp_flash_driver_t *p_driver);

#endif /* BSP_FLASH_HANDLER_H */
