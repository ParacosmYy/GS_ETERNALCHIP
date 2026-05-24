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

int BspSys_GetRunningBank(ota_slot_t *p_slot)
{
    uint32_t vtor;

    if (p_slot == NULL)
    {
        return -1;
    }

    vtor = SCB->VTOR;
    if (vtor >= FLASH_ADDR_SLOT_B && vtor < (FLASH_ADDR_SLOT_B + SLOT_B_SIZE))
    {
        *p_slot = OTA_SLOT_B;
        return 0;
    }

    if (vtor >= FLASH_ADDR_SLOT_A && vtor < (FLASH_ADDR_SLOT_A + SLOT_A_SIZE))
    {
        *p_slot = OTA_SLOT_A;
        return 0;
    }

    return -1;
}

//******************************** Implement ********************************//
