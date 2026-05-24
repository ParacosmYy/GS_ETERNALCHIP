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
 * @brief   平台看门狗驱动接口（OPS 模式）
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

/** @brief 看门狗驱动前向声明 */
typedef struct bsp_wdg_driver bsp_wdg_driver_t;

/**
 * @brief  看门狗硬件操作（初始化时注入）。
 *
 * @note   仅平台层 — 零 HAL 依赖。
 * */
typedef struct
{
    void (*pf_refresh)(void *p_handle);
} wdg_hw_operations_t;

/**
 * @brief  看门狗驱动实例。
 *
 * @note   p_handle  : HAL IWDG 句柄（STM32）或等效类型。
 *         p_hw_ops  : 硬件操作表。
 *         is_inited : BspWdg_Init() 后为 1，否则为 0。
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
