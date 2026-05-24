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
#include <stdint.h>
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

#ifdef __cplusplus
}
#endif

#endif /* BSP_SYS_H */
