/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    bsp_sys.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author GS_Mark
 *
 * @brief System BSP utilities — tick, reboot (abstracts HAL/CMSIS calls)
 *
 * Usage:
 *   uint32_t now = BspSys_GetTick();
 *   BspSys_Reboot();
 *
 * @version V1.0 2025-05-24
 *
 * @note 1 tab == 4 spaces
 *
 *****************************************************************************/
#ifndef BSP_SYS_H
#define BSP_SYS_H

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include "plat_sys.h"
//******************************* Includes **********************************//

//******************************* Declaring ********************************//

/**
 * @brief  获取系统 tick 计数（毫秒）。
 *
 * @return  当前 tick 值。
 * */
uint32_t BspSys_GetTick(void);

/**
 * @brief  软件复位 MCU（NVIC System Reset）。
 * */
void BspSys_Reboot(void);

/**
 * @brief  检测当前运行的 OTA Bank（通过 SCB->VTOR）。
 *
 * @param[out] p_slot : 检测到的运行 Bank。
 *
 * @return   0 : 检测成功。
 * @return  -1 : VTOR 不指向已知的 OTA Bank。
 * */
int BspSys_GetRunningBank(ota_slot_t *p_slot);

#ifdef __cplusplus
}
#endif

#endif /* BSP_SYS_H */
