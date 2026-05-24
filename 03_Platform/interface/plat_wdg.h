/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    plat_wdg.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author  GS_Mark
 *
 * @brief   Platform Watchdog driver interface (OPS pattern)
 *
 * @version V2.0 2025-05-24
 *
 * @note    1 tab == 4 spaces
 *
 *****************************************************************************/
#ifndef PLAT_WDG_H
#define PLAT_WDG_H

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include <stdint.h>
//******************************* Includes **********************************//

//******************************* Types *************************************//

/** @brief Forward declaration of watchdog driver */
typedef struct bsp_wdg_driver bsp_wdg_driver_t;

/**
 * @brief  Watchdog hardware operations (injected at init).
 *
 * @note   Platform layer only — ZERO HAL dependencies.
 * */
typedef struct
{
    void (*pf_refresh)(void *p_handle);
} wdg_hw_operations_t;

/**
 * @brief  Watchdog driver instance.
 *
 * @note   p_handle  : HAL IWDG handle (STM32) or equivalent.
 *         p_hw_ops  : Hardware operations table.
 *         is_inited : 1 after BspWdg_Init(), 0 otherwise.
 * */
struct bsp_wdg_driver
{
    void                       *p_handle;
    const wdg_hw_operations_t  *p_hw_ops;
    uint8_t                     is_inited;
};

//******************************* Declaring **********************************/

/**
 * @brief  初始化 Watchdog BSP 驱动，注入硬件操作表。
 *
 * @param[in] p_drv    : 驱动实例指针。
 * @param[in] p_handle : HAL IWDG 句柄指针（CubeMX 生成）。
 * @param[in] p_ops    : 硬件操作表指针。
 * */
void BspWdg_Init(bsp_wdg_driver_t *p_drv, void *p_handle,
                 const wdg_hw_operations_t *p_ops);

/**
 * @brief  喂看门狗（通过 ops 表刷新计数器）。
 *
 * @param[in] p_drv : 驱动实例指针。
 * */
void BspWdg_Feed(bsp_wdg_driver_t *p_drv);

#ifdef __cplusplus
}
#endif

#endif /* PLAT_WDG_H */
