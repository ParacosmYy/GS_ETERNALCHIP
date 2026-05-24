/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    bsp_wdg_driver.c
 *
 * @par dependencies
 * - bsp_wdg_driver.h
 * - stm32f4xx_hal.h
 *
 * @author  GS_Mark
 *
 * @brief   Watchdog BSP driver implementation — HAL IWDG via OPS table
 *
 * Processing flow:
 *   BspWdg_Init() stores handle + ops table into driver instance.
 *   BspWdg_Feed() delegates to pf_refresh via ops table.
 *
 * @version V2.0 2025-05-24
 *
 * @note    1 tab == 4 spaces
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "bsp_wdg_driver.h"
//******************************** Includes *********************************//

//******************************** Implement ********************************//

/**
 * @brief  初始化 Watchdog BSP 驱动，注入硬件操作表。
 *
 * Steps:
 *  1. Validate parameters.
 *  2. Store handle and ops table.
 *  3. Mark as initialized.
 *
 * @param[in] p_drv    : 驱动实例指针。
 * @param[in] p_handle : HAL IWDG 句柄指针（CubeMX 生成）。
 * @param[in] p_ops    : 硬件操作表指针。
 * */
void BspWdg_Init(bsp_wdg_driver_t *p_drv, void *p_handle,
                 const wdg_hw_operations_t *p_ops)
{
    if (p_drv == NULL || p_ops == NULL)
    {
        return;
    }

    p_drv->p_handle = p_handle;
    p_drv->p_hw_ops = p_ops;
    p_drv->is_inited = 1u;
}

/**
 * @brief  喂看门狗（通过 ops 表刷新计数器）。
 *
 * Steps:
 *  1. Validate driver and ops.
 *  2. Call pf_refresh with stored handle.
 *
 * @param[in] p_drv : 驱动实例指针。
 * */
void BspWdg_Feed(bsp_wdg_driver_t *p_drv)
{
    if (p_drv == NULL || p_drv->p_hw_ops == NULL)
    {
        return;
    }

    if (p_drv->p_hw_ops->pf_refresh != NULL)
    {
        p_drv->p_hw_ops->pf_refresh(p_drv->p_handle);
    }
}

//******************************** Implement ********************************//
