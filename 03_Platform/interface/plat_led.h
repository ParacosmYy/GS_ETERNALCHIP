/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    plat_led.h
 *
 * @par dependencies
 * - plat_gpio.h
 * - stdint.h
 *
 * @author  GS_Mark
 *
 * @brief   Platform LED interface — no HAL types
 *
 * @version V1.0 2025-05-24
 *
 * @note    1 tab == 4 spaces
 *
 *****************************************************************************/
#ifndef PLAT_LED_H
#define PLAT_LED_H

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include "plat_gpio.h"
#include <stdint.h>
//******************************* Includes **********************************//

//******************************* Types *************************************//
typedef struct bsp_led_driver bsp_led_driver_t;

//******************************* Declaring ********************************//
void BspLed_Init(bsp_led_driver_t *p_drv, const plat_gpio_t *p_config);
void BspLed_On(bsp_led_driver_t *p_drv);
void BspLed_Off(bsp_led_driver_t *p_drv);
void BspLed_Toggle(bsp_led_driver_t *p_drv);
void BspLed_BlinkStart(bsp_led_driver_t *p_drv, uint32_t interval_ms);
void BspLed_BlinkStop(bsp_led_driver_t *p_drv);
void BspLed_TimebaseHook(bsp_led_driver_t *p_drv);

#ifdef __cplusplus
}
#endif

#endif /* PLAT_LED_H */
