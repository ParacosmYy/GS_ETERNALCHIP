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
 * @brief   平台 LED 接口 — 零 HAL 类型
 *
 * @version V2.0 2025-05-24
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

/** @brief  LED 驱动实例前向声明 */
typedef struct bsp_led_driver bsp_led_driver_t;

/** @brief  LED 操作抽象层（可替换为非 HAL 实现） */
typedef struct
{
    void (*p_On)(bsp_led_driver_t *p_drv);     /**< 点亮 */
    void (*p_Off)(bsp_led_driver_t *p_drv);    /**< 熄灭 */
    void (*p_Toggle)(bsp_led_driver_t *p_drv); /**< 翻转 */
} led_operations_t;

/** @brief  LED OS 操作抽象层（时基服务） */
typedef struct
{
    uint32_t (*pf_get_tick)(void); /**< 获取系统 tick（毫秒） */
} led_os_operations_t;

/** @brief  LED 硬件配置（平台无关） */
typedef struct
{
    plat_gpio_t gpio; /**< GPIO 配置（port/pin/active_level） */
} bsp_led_config_t;

/** @brief  LED 驱动实例 */
struct bsp_led_driver
{
    const bsp_led_config_t    *p_config;          /**< 硬件配置 */
    const led_operations_t    *p_ops;             /**< 操作函数指针 */
    const led_os_operations_t *p_os_ops;          /**< OS 操作函数指针 */
    uint32_t                   blink_interval_ms; /**< 闪烁周期（0 = 停止） */
    uint32_t                   blink_last_tick;   /**< 上次翻转时间戳 */
    uint8_t                    is_blinking;       /**< 1 = 闪烁中 */
};

//******************************* Declaring ********************************//

/** @brief  初始化 LED 驱动（使用默认 HAL GPIO 操作 + OS 时基操作） */
void BspLed_Init(bsp_led_driver_t *p_drv, const bsp_led_config_t *p_config,
                 const led_os_operations_t *p_os_ops);

/** @brief  点亮 LED */
void BspLed_On(bsp_led_driver_t *p_drv);

/** @brief  熄灭 LED */
void BspLed_Off(bsp_led_driver_t *p_drv);

/** @brief  翻转 LED 状态 */
void BspLed_Toggle(bsp_led_driver_t *p_drv);

/** @brief  启动非阻塞闪烁 */
void BspLed_BlinkStart(bsp_led_driver_t *p_drv, uint32_t interval_ms);

/** @brief  停止闪烁并熄灭 LED */
void BspLed_BlinkStop(bsp_led_driver_t *p_drv);

/** @brief  时基钩子（周期调用驱动闪烁） */
void BspLed_TimebaseHook(bsp_led_driver_t *p_drv);

//******************************* Extern ***********************************//

/** @brief  预定义的 HAL GPIO 操作实例 */
extern const led_operations_t bsp_led_hal_ops;

/** @brief  预定义的 HAL OS 操作实例（时基） */
extern const led_os_operations_t g_led_os_ops;

#ifdef __cplusplus
}
#endif

#endif /* PLAT_LED_H */
