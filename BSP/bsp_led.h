/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 * 
 * @file bsp_led.h
 * 
 * @par dependencies 
 * - main.h
 * 
 * @author GS_Mark
 *
 * @brief LED 硬件抽象层，提供 LED 开/关/翻转接口
 * 
 * Usage:
 * BSP_LED_Init()      - 初始化 LED（CubeMX 已完成 GPIO 配置，可选调用）
 * BSP_LED_On()        - 点亮 LED
 * BSP_LED_Off()       - 熄灭 LED
 * BSP_LED_Toggle()    - 翻转 LED 状态
 * 
 * @version V1.0 2025-04-26
 *
 * @note 1 tab == 4 spaces
 * 
 *****************************************************************************/
#ifndef __BSP_LED_H__
#define __BSP_LED_H__

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include "main.h"
//******************************* Includes **********************************//

//******************************* Defines ***********************************//
/* LED 状态枚举 */
typedef enum {
    LED_OK   = 0,
    LED_ERROR = 1
} bsp_led_status_t;

//******************************* Declaring *********************************//

/**
 * @brief 点亮 LED（PC13 低电平点亮）
 * 
 * @return bsp_led_status_t : 操作状态
 * */
bsp_led_status_t BSP_LED_On(void);

/**
 * @brief 熄灭 LED（PC13 高电平熄灭）
 * 
 * @return bsp_led_status_t : 操作状态
 * */
bsp_led_status_t BSP_LED_Off(void);

/**
 * @brief 翻转 LED 状态
 * 
 * @return bsp_led_status_t : 操作状态
 * */
bsp_led_status_t BSP_LED_Toggle(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_LED_H__ */
