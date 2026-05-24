/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    plat_key.h
 *
 * @par dependencies
 * - plat_gpio.h
 * - stdint.h
 *
 * @author  GS_Mark
 *
 * @brief   Platform Key scan interface — no HAL types
 *
 * @version V1.0 2025-05-24
 *
 * @note    1 tab == 4 spaces
 *
 *****************************************************************************/
#ifndef PLAT_KEY_H
#define PLAT_KEY_H

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include "plat_gpio.h"
#include <stdint.h>
//******************************* Includes **********************************//

//******************************* Types *************************************//
typedef enum
{
    BSP_KEY_EVT_NONE = 0,
    BSP_KEY_EVT_PRESSED,
    BSP_KEY_EVT_RELEASED,
    BSP_KEY_EVT_SHORT_PRESS,
    BSP_KEY_EVT_LONG_PRESS,
} bsp_key_event_t;

typedef void (*bsp_key_callback_t)(bsp_key_event_t event, void *p_user_data);

typedef struct
{
    plat_gpio_t         gpio;           /**< GPIO config (port/pin/active_level) */
    uint32_t            debounce_ms;    /**< Debounce duration (default 20ms) */
    uint32_t            long_press_ms;  /**< Long press threshold (default 1000ms) */
    bsp_key_callback_t  callback;       /**< Event callback */
    void               *p_user_data;    /**< Callback user context */
} bsp_key_config_t;

typedef struct
{
    const bsp_key_config_t *p_config;
    uint8_t   state;
    uint8_t   stable_level;
    uint32_t  last_tick;
    uint32_t  press_tick;
    uint8_t   long_press_fired;
} bsp_key_driver_t;

//******************************* Declaring ********************************//
void BspKey_Init(bsp_key_driver_t *p_drv, const bsp_key_config_t *p_config);
void BspKey_Scan(bsp_key_driver_t *p_drv);

#ifdef __cplusplus
}
#endif

#endif /* PLAT_KEY_H */
