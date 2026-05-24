/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    bsp_wdg.c
 *
 * @par dependencies
 * - bsp_wdg.h
 * - stm32f4xx_hal.h
 *
 * @author GS_Mark
 *
 * @brief Watchdog BSP driver implementation — wraps HAL_IWDG_Refresh
 *
 * Processing flow:
 *   BspWdg_Init() 保存 HAL 句柄。
 *   BspWdg_Feed() 通过句柄刷新 IWDG 计数器。
 *
 * @version V1.0 2025-05-24
 *
 * @note 1 tab == 4 spaces
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "bsp_wdg.h"
#include "stm32f4xx_hal.h"
//******************************** Includes *********************************//

//******************************** Variables ********************************//
static IWDG_HandleTypeDef *s_p_hwdg = NULL;
//******************************** Variables ********************************//

//******************************** Implement ********************************//

void BspWdg_Init(void *p_hwdg)
{
    s_p_hwdg = (IWDG_HandleTypeDef *)p_hwdg;
}

void BspWdg_Feed(void)
{
    if (s_p_hwdg != NULL && s_p_hwdg->Instance != NULL)
    {
        HAL_IWDG_Refresh(s_p_hwdg);
    }
}

//******************************** Implement ********************************//
