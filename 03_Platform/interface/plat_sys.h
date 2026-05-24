/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    plat_sys.h
 *
 * @par dependencies
 * - stdint.h
 * - ota_config.h
 *
 * @author  GS_Mark
 *
 * @brief   平台系统工具接口（OPS 模式）
 *
 * @version V2.0 2025-05-24
 *
 * @note    1 tab == 4 spaces
 *
 *****************************************************************************/
#ifndef PLAT_SYS_H
#define PLAT_SYS_H

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include <stdint.h>
#include "ota_config.h"
//******************************* Includes **********************************//

//******************************* Types *************************************//

/** @brief 系统驱动前向声明 */
typedef struct bsp_sys_driver bsp_sys_driver_t;

/**
 * @brief  系统硬件操作（初始化时注入）。
 *
 * @note   仅平台层 — 零 HAL 依赖。
 * */
typedef struct
{
    uint32_t (*pf_get_tick)(void);
    void     (*pf_reboot)(void);
    int      (*pf_get_running_bank)(ota_slot_t *p_slot);
} sys_operations_t;

/**
 * @brief  系统驱动实例。
 *
 * @note   p_ops     : 硬件操作表。
 *         is_inited : BspSys_Init() 后为 1，否则为 0。
 * */
struct bsp_sys_driver
{
    const sys_operations_t *p_ops;
    uint8_t                 is_inited;
};

//******************************* Declaring **********************************/

/**
 * @brief  初始化 System BSP 驱动，注入硬件操作表。
 *
 * @param[in] p_drv : 驱动实例指针。
 * @param[in] p_ops : 硬件操作表指针。
 * */
void BspSys_Init(bsp_sys_driver_t *p_drv, const sys_operations_t *p_ops);

/**
 * @brief  获取系统 tick 计数（毫秒）。
 *
 * @param[in] p_drv : 驱动实例指针。
 *
 * @return  当前 tick 值。
 * */
uint32_t BspSys_GetTick(const bsp_sys_driver_t *p_drv);

/**
 * @brief  软件复位 MCU。
 *
 * @param[in] p_drv : 驱动实例指针。
 * */
void BspSys_Reboot(const bsp_sys_driver_t *p_drv);

/**
 * @brief  检测当前运行的 OTA Bank。
 *
 * @param[in]  p_drv  : 驱动实例指针。
 * @param[out] p_slot : 检测到的运行 Bank。
 *
 * @return   0 : 检测成功。
 * @return  -1 : 检测失败。
 * */
int BspSys_GetRunningBank(const bsp_sys_driver_t *p_drv, ota_slot_t *p_slot);

#ifdef __cplusplus
}
#endif

#endif /* PLAT_SYS_H */
