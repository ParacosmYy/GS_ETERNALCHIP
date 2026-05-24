/**
 * @file    bsp_flash_handler.h
 * @brief   Flash Handler — 分区级门面，封装 Flash Driver
 * @author  GS_Mark
 *
 * @par dependencies
 * - plat_flash.h
 *
 * @note    Handler 持有一个静态 Driver 实例，对外暴露与上层
 *          已有调用一致的 API，调用方无需修改。
 */

#ifndef BSP_FLASH_HANDLER_H
#define BSP_FLASH_HANDLER_H

#include "plat_flash.h"

/** @brief  Flash Handler — 持有一个驱动实例 */
typedef struct
{
    bsp_flash_driver_t *p_driver;
    uint8_t             is_inited;
} bsp_flash_handler_t;

/** @brief  初始化 Handler，挂载已配置的驱动实例 */
void BspFlashHandler_Init(bsp_flash_handler_t *p_handler,
                          bsp_flash_driver_t *p_driver);

#endif /* BSP_FLASH_HANDLER_H */
