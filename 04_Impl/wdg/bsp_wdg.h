/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    bsp_wdg.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author GS_Mark
 *
 * @brief Watchdog BSP driver — abstracts HAL IWDG feed operation
 *
 * Usage:
 *   BspWdg_Init(&hiwdg);   // once at startup
 *   BspWdg_Feed();          // call periodically
 *
 * @version V1.0 2025-05-24
 *
 * @note 1 tab == 4 spaces
 *
 *****************************************************************************/
#ifndef BSP_WDG_H
#define BSP_WDG_H

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include <stdint.h>
//******************************* Includes **********************************//

//******************************* Declaring ********************************//

/**
 * @brief  初始化 Watchdog BSP 驱动，保存 HAL IWDG 句柄。
 *
 * @param[in] p_hwdg : HAL IWDG 句柄指针（CubeMX 生成）。
 * */
void BspWdg_Init(void *p_hwdg);

/**
 * @brief  喂看门狗（刷新 IWDG 计数器）。
 * */
void BspWdg_Feed(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_WDG_H */
