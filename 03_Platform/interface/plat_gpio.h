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
 * @brief   平台 GPIO 类型抽象 — 不透明 port/pin
 *
 * 用法：
 *   plat_gpio_t led_gpio = { (void *)GPIOC, 13 };
 *   // 实现层将 port 强转为 GPIO_TypeDef*
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
    void    *port;    /**< GPIO 端口（不透明 — 实现层强转为 GPIO_TypeDef*） */
    uint16_t pin;     /**< GPIO 引脚号 */
    uint8_t  active_level; /**< 0 = 低电平有效，1 = 高电平有效 */
} plat_gpio_t;

#ifdef __cplusplus
}
#endif

#endif /* PLAT_GPIO_H */
