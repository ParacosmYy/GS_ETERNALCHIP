/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    plat_gpio.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author  GS_Mark
 *
 * @brief   Platform GPIO type abstraction — opaque port/pin
 *
 * Usage:
 *   plat_gpio_t led_gpio = { (void *)GPIOC, 13 };
 *   // Impl layer casts port to GPIO_TypeDef*
 *
 * @version V1.0 2025-05-24
 *
 * @note    1 tab == 4 spaces
 *
 *****************************************************************************/
#ifndef PLAT_GPIO_H
#define PLAT_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include <stdint.h>
//******************************* Includes **********************************//

//******************************* Types *************************************//
typedef struct
{
    void    *port;    /**< GPIO port (opaque — Impl casts to GPIO_TypeDef*) */
    uint16_t pin;     /**< GPIO pin number */
    uint8_t  active_level; /**< 0 = active low, 1 = active high */
} plat_gpio_t;

#ifdef __cplusplus
}
#endif

#endif /* PLAT_GPIO_H */
