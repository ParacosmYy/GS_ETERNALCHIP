/**
 * @file    bsp_led.h
 * @brief   LED BSP driver - operation abstraction + driver instance
 * @author  GS_Mark
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

    /** @brief  LED 驱动实例前向声明 */
    typedef struct bsp_led_driver bsp_led_driver_t;

    /** @brief  LED 操作抽象层（可替换为非 HAL 实现） */
    typedef struct
    {
        void (*p_On)(bsp_led_driver_t *p_drv);     /**< 点亮 */
        void (*p_Off)(bsp_led_driver_t *p_drv);    /**< 熄灭 */
        void (*p_Toggle)(bsp_led_driver_t *p_drv); /**< 翻转 */
    } led_operations_t;

    /** @brief  LED 硬件配置 */
    typedef struct
    {
        GPIO_TypeDef *p_port;       /**< GPIO 端口 */
        uint16_t      pin;          /**< GPIO 引脚 */
        uint8_t       active_level; /**< 0 = 低有效, 1 = 高有效 */
    } bsp_led_config_t;

    /** @brief  LED 驱动实例 */
    struct bsp_led_driver
    {
        const bsp_led_config_t *p_config;          /**< 硬件配置 */
        const led_operations_t *p_ops;             /**< 操作函数指针 */
        uint32_t                blink_interval_ms; /**< 闪烁周期（0 = 停止） */
        uint32_t                blink_last_tick;   /**< 上次翻转时间戳 */
        uint8_t                 is_blinking;       /**< 1 = 闪烁中 */
    };

    //*** Public API ***//

    /** @brief  初始化 LED 驱动（使用默认 HAL GPIO 操作） */
    void BspLed_Init(bsp_led_driver_t *p_drv, const bsp_led_config_t *p_config);

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

    //*** Default HAL Operations ***//

    /** @brief  预定义的 HAL GPIO 操作实例 */
    extern const led_operations_t bsp_led_hal_ops;

#ifdef __cplusplus
}
#endif

#endif /* BSP_LED_H */
