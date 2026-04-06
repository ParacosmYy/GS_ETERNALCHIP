/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file bsp_led_driver.c
 * 
 * @par dependencies 
 * - bsp_led_driver.h
 * 
 * 
 * @author Jack | R&D Dept. | EternalChip Á¢Ð¾Ç¶ÈëÊ½
 * 
 * @brief Provide the HAL APIs of LEDs and corresponding operations.
 * 
 * Processing flow:
 * 
 * call directly.
 * 
 * @version V1.0 2024-10-18
 *
 * @note 1 tab == 4 spaces!
 * 
 *****************************************************************************/

//******************************** Includes *********************************//
#include "bsp_led_driver.h"

//******************************** Includes *********************************//


//******************************** Defines **********************************//

led_status_t led_driver_init(bsp_led_driver_t *const self)
{
    led_status_t ret = LED_STATUS_OK;
#ifdef DEBUG
    DEBUG_OUT("led_driver_init: Initializing LED driver...\n");
#endif
    if(self == NULL)
    {
#ifdef DEBUG
        DEBUG_OUT("led_driver_init: Invalid parameter(s)!\n");
#endif
        return LED_STATUS_ERROR_PARAM;
    }

    self->p_led_ops->pf_off();
    self->p_os_delay->pf_os_delay_ms(100);
    uint32_t time_stamp = 0;
    self->p_time_base->pf_get_time_ms(&time_stamp);

    return ret;

}


led_status_t bsp_led_driver_inst(bsp_led_driver_t *const self,
                                 led_ops_t *const led_ops,
#ifdef OS_SUPPORTING
                                 os_delay_t *const p_os_delay,
#endif
                                 time_base_t *const p_time_base)
{
    led_status_t ret = LED_STATUS_OK;
#ifdef DEBUG
    DEBUG_OUT("bsp_led_driver_inst: Initializing LED driver instance...\n");
#endif
    if (self == NULL || led_ops == NULL || p_time_base == NULL || p_os_delay == NULL)
    {
#ifdef DEBUG
        DEBUG_OUT("bsp_led_driver_inst: Invalid parameter(s)!\n");
#endif
        return LED_STATUS_ERROR_PARAM;
    }

    if(self->led_is_inited == INITED)
    {
#ifdef DEBUG
        DEBUG_OUT("bsp_led_driver_inst: LED driver instance already initialized!\n");
#endif
        return LED_STATUS_ERROR_SOURCE; 
    }

#ifdef DEBUG

    DEBUG_OUT("bsp_led_driver_inst: Initializing LED driver instance...\n");
#endif

    self->p_led_ops = led_ops;
    self->p_time_base = p_time_base;
#ifdef OS_SUPPORTING
    self->p_os_delay = p_os_delay;
#endif

    self->led_cycle_time_ms = 0;
    self->led_blink_proportion = LED_BLINK_PROPORTION_0_5;
    self->led_blink_count = 0;

    ret = led_driver_init(self);

    if(ret != LED_STATUS_OK)

    {
#ifdef DEBUG
        DEBUG_OUT("bsp_led_driver_inst: LED driver instance initialization failed!\n");
#endif
    self->p_led_ops = NULL;
    self->p_time_base = NULL;
#ifdef OS_SUPPORTING
    self->p_os_delay = NULL;
#endif
        return ret;
   }
   self->led_is_inited = INITED;
#ifdef DEBUG
    DEBUG_OUT("bsp_led_driver_inst: LED driver instance initialized successfully!\n");
#endif
    return ret;
}
