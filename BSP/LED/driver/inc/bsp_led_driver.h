/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file bsp_led_driver.h
 * 
 * @par dependencies 
 * - stdio.h
 * - stdint.h
 * 
 * @author Jack | R&D Dept. | EternalChip ?????
 *
 * @brief Provides HAL APIs for LED control and operations.
 * 
 * Usage:
 * Call functions directly.
 * 
 * @version V1.0 2024-10-18
 *
 * @note 1 tab == 4 spaces
 * 
 *****************************************************************************/

#ifndef __BSP_LED_DRIVER_H__
#define __BSP_LED_DRIVER_H__

//******************************* Includes **********************************//
#include <stdio.h>
#include <stdint.h>
//******************************* Includes **********************************//


typedef enum
{
    LED_STATUS_OK = 0,
    LED_STATUS_ERROR,
    LED_STATUS_ERROR_TIMEOUT,
    LED_STATUS_ERROR_SOURCE,
    LED_STATUS_ERROR_PARAM,
    LED_STATUS_ERROR_MEMORY,
    LED_STATUS_ERROR_ISR,
    LED_STATUS_ERROR_RESERVED,  
} led_status_t;

typedef enum
{
    LED_BLINK_PROPORTION_0_5 = 0, // 50% duty cycle
    LED_BLINK_PROPORTION_0_25,    // 25% duty cycle
    LED_BLINK_PROPORTION_0_75,    // 75% duty cycle
    LED_BLINK_PROPORTION_MAX
} proportion_t;

typedef struct led_ops
{
    led_status_t (*pf_init)(void);
    led_status_t (*pf_on)(void);
    led_status_t (*pf_off)(void);
    led_status_t (*pf_toggle)(void);
    led_status_t (*pf_blink)(proportion_t proportion, uint32_t count);
} led_ops_t;

typedef struct 
{
    led_status_t (*pf_get_time_ms)(uint32_t *const p_time_ms);
} time_base_t;

typedef struct 
{
    led_status_t (*pf_os_delay_ms)(const uint32_t delay_ms);
} os_delay_t;

typedef struct bsp_led_driver bsp_led_driver_t;

typedef struct bsp_led_driver
{
    uint8_t led_is_inited;

    uint32_t led_cycle_time_ms;
    uint32_t led_blink_count;    
    proportion_t led_blink_proportion;

    led_ops_t *p_led_ops;
    time_base_t time_base;
    os_delay_t os_delay;



} bsp_led_driver_t;

typedef led_status_t (*pf_led_control_func_t)(bsp_led_driver_t *const p_led_driver,
                                              uint32_t led_cycle_time_ms,
                                              uint32_t led_blink_repeat_count,
                                              proportion_t led_proportion);


//******************************* Declaring *********************************//
/**
 * @brief Instantiates the bsp_led_driver_t structure.
 * 
 * Initializes the LED driver instance with provided core, OS, and timebase
 * interfaces.
 * 
 * @param[in] self      : Pointer to the LED driver instance.
 * @param[in] led_ops   : Core LED operations interface.
 * @param[in] os_delay  : OS delay interface (if OS support is enabled).
 * @param[in] time_base : Timebase interface.
 * 
 * @return led_status_t : Operation status
 * 
 */

led_status_t bsp_led_driver_init(bsp_led_driver_t *const self,
                                 led_ops_t *const led_ops,
                                 os_delay_t *const os_delay,
                                 time_base_t *const time_base);


#endif // End of __BSP_LED_DRIVER_H__
