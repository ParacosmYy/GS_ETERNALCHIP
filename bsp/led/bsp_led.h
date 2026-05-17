/**
 * @file    bsp_led.h
 * @brief   LED BSP driver - operation abstraction + driver instance
 * @note    Portable across STM32 families. Override led_operations_t for
 *          non-HAL GPIO implementations (e.g. I2C expander).
 */

#ifndef BSP_LED_H
#define BSP_LED_H

#ifdef __cplusplus
extern "C"
{
#endif

//*** Includes ***//
#include "stm32f4xx_hal.h"
#include <stdint.h>

    //*** Type Definitions ***//

    /** @brief  Forward declaration of driver instance */
    typedef struct bsp_led_driver bsp_led_driver_t;

    /**
     * @brief  LED operation abstraction layer.
     *         Override these function pointers to adapt non-HAL GPIO backends.
     */
    typedef struct
    {
        void (*p_On)(bsp_led_driver_t *p_drv);     /**< Turn LED on     */
        void (*p_Off)(bsp_led_driver_t *p_drv);    /**< Turn LED off    */
        void (*p_Toggle)(bsp_led_driver_t *p_drv); /**< Toggle LED      */
    } led_operations_t;

    /**
     * @brief  LED hardware configuration (immutable, usually const)
     */
    typedef struct
    {
        GPIO_TypeDef *p_port;       /**< GPIO port (GPIOA .. GPIOC)       */
        uint16_t      pin;          /**< GPIO pin  (GPIO_PIN_x)           */
        uint8_t       active_level; /**< 0 = low-active, 1 = high-active */
    } bsp_led_config_t;

    /**
     * @brief  LED driver instance (one per physical LED)
     */
    struct bsp_led_driver
    {
        const bsp_led_config_t *p_config;          /**< Hardware configuration       */
        const led_operations_t *p_ops;             /**< Operation function pointers   */
        uint32_t                blink_interval_ms; /**< Blink period (0 = stopped)    */
        uint32_t                blink_last_tick;   /**< Last toggle timestamp (ms)    */
        uint8_t                 is_blinking;       /**< 1 = blinking in progress      */
    };

    //*** Public API ***//

    /**
     * @brief  初始化 LED 驱动（使用默认 HAL GPIO 操作）
     * @param  p_drv    驱动实例（调用者分配）
     * @param  p_config 硬件配置（需保持有效，通常为 const 全局变量）
     */
    void BspLed_Init(bsp_led_driver_t *p_drv, const bsp_led_config_t *p_config);

    /** @brief  点亮 LED */
    void BspLed_On(bsp_led_driver_t *p_drv);

    /** @brief  熄灭 LED */
    void BspLed_Off(bsp_led_driver_t *p_drv);

    /** @brief  翻转 LED 状态 */
    void BspLed_Toggle(bsp_led_driver_t *p_drv);

    /**
     * @brief  启动非阻塞闪烁
     * @param  p_drv        驱动实例
     * @param  interval_ms  翻转周期（毫秒）
     */
    void BspLed_BlinkStart(bsp_led_driver_t *p_drv, uint32_t interval_ms);

    /** @brief  停止闪烁并熄灭 LED */
    void BspLed_BlinkStop(bsp_led_driver_t *p_drv);

    /**
     * @brief  时基钩子函数（周期性调用，如 SysTick 或定时器中断）
     *         驱动闪烁状态机，检查是否到达翻转时间。
     * @param  p_drv  驱动实例
     */
    void BspLed_TimebaseHook(bsp_led_driver_t *p_drv);

    //*** Default HAL Operations ***//

    /** @brief  Pre-defined HAL operations instance for STM32 GPIO */
    extern const led_operations_t bsp_led_hal_ops;

#ifdef __cplusplus
}
#endif

#endif /* BSP_LED_H */
