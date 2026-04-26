/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 * 
 * @file bsp_led.c
 * 
 * @par dependencies 
 * - bsp_led.h
 * 
 * @author GS_Mark
 *
 * @brief LED 硬件抽象层实现，基于 HAL GPIO 控制 PC13
 * 
 * @version V1.0 2025-04-26
 *
 * @note 1 tab == 4 spaces
 * 
 *****************************************************************************/

//******************************** Includes *********************************//
#include "bsp_led.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
#define LED_PORT    GPIOC
#define LED_PIN     GPIO_PIN_13

//******************************** Implement ********************************//

/**
 * @brief 点亮 LED（PC13 低电平点亮）
 * 
 * Steps:
 *  1. 调用 HAL_GPIO_WritePin 输出低电平
 * 
 * @return bsp_led_status_t : 操作状态
 * 
 * */
bsp_led_status_t BSP_LED_On(void)
{
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
    return LED_OK;
}

/**
 * @brief 熄灭 LED（PC13 高电平熄灭）
 * 
 * Steps:
 *  1. 调用 HAL_GPIO_WritePin 输出高电平
 * 
 * @return bsp_led_status_t : 操作状态
 * 
 * */
bsp_led_status_t BSP_LED_Off(void)
{
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
    return LED_OK;
}

/**
 * @brief 翻转 LED 状态
 * 
 * Steps:
 *  1. 调用 HAL_GPIO_TogglePin 翻转 PC13 电平
 * 
 * @return bsp_led_status_t : 操作状态
 * 
 * */
bsp_led_status_t BSP_LED_Toggle(void)
{
    HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
    return LED_OK;
}
