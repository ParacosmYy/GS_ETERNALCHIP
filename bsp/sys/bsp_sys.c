/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    bsp_sys.c
 *
 * @par dependencies
 * - bsp_sys.h
 * - stm32f4xx_hal.h
 *
 * @author GS_Mark
 *
 * @brief System BSP utilities implementation — wraps HAL_GetTick / NVIC_SystemReset
 *
 * @version V1.0 2025-05-24
 *
 * @note 1 tab == 4 spaces
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "bsp_sys.h"
#include "stm32f4xx_hal.h"
//******************************** Includes *********************************//

//******************************** Implement ********************************//

uint32_t BspSys_GetTick(void)
{
    return HAL_GetTick();
}

void BspSys_Reboot(void)
{
    NVIC_SystemReset();
}

//******************************** Implement ********************************//
