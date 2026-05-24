/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    bsp_sys_driver.c
 *
 * @par dependencies
 * - bsp_sys_driver.h
 * - stm32f4xx_hal.h
 *
 * @author  GS_Mark
 *
 * @brief   System BSP driver implementation — HAL/CMSIS via OPS table
 *
 * Processing flow:
 *   BspSys_Init() stores ops table into driver instance.
 *   All public functions delegate to ops table function pointers.
 *
 * @version V2.0 2025-05-24
 *
 * @note    1 tab == 4 spaces
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "bsp_sys_driver.h"
//******************************** Includes *********************************//

//******************************** Implement ********************************//

/**
 * @brief  初始化 System BSP 驱动，注入硬件操作表。
 *
 * Steps:
 *  1. Validate parameters.
 *  2. Store ops table.
 *  3. Mark as initialized.
 *
 * @param[in] p_drv : 驱动实例指针。
 * @param[in] p_ops : 硬件操作表指针。
 * */
void BspSys_Init(bsp_sys_driver_t *p_drv, const sys_operations_t *p_ops)
{
    if (p_drv == NULL || p_ops == NULL)
    {
        return;
    }

    p_drv->p_ops     = p_ops;
    p_drv->is_inited = 1u;
}

/**
 * @brief  获取系统 tick 计数（毫秒）。
 *
 * Steps:
 *  1. Validate driver and ops.
 *  2. Delegate to pf_get_tick.
 *
 * @param[in] p_drv : 驱动实例指针。
 *
 * @return  当前 tick 值。
 * */
uint32_t BspSys_GetTick(const bsp_sys_driver_t *p_drv)
{
    if (p_drv == NULL || p_drv->p_ops == NULL)
    {
        return 0u;
    }

    if (p_drv->p_ops->pf_get_tick != NULL)
    {
        return p_drv->p_ops->pf_get_tick();
    }

    return 0u;
}

/**
 * @brief  软件复位 MCU。
 *
 * Steps:
 *  1. Validate driver and ops.
 *  2. Delegate to pf_reboot.
 *
 * @param[in] p_drv : 驱动实例指针。
 * */
void BspSys_Reboot(const bsp_sys_driver_t *p_drv)
{
    if (p_drv == NULL || p_drv->p_ops == NULL)
    {
        return;
    }

    if (p_drv->p_ops->pf_reboot != NULL)
    {
        p_drv->p_ops->pf_reboot();
    }
}

/**
 * @brief  检测当前运行的 OTA Bank。
 *
 * Steps:
 *  1. Validate driver and ops.
 *  2. Delegate to pf_get_running_bank.
 *
 * @param[in]  p_drv  : 驱动实例指针。
 * @param[out] p_slot : 检测到的运行 Bank。
 *
 * @return   0 : 检测成功。
 * @return  -1 : 检测失败。
 * */
int BspSys_GetRunningBank(const bsp_sys_driver_t *p_drv, ota_slot_t *p_slot)
{
    if (p_drv == NULL || p_drv->p_ops == NULL)
    {
        return -1;
    }

    if (p_drv->p_ops->pf_get_running_bank != NULL)
    {
        return p_drv->p_ops->pf_get_running_bank(p_slot);
    }

    return -1;
}

//******************************** Implement ********************************//
