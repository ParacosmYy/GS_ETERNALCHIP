/**
 * @file    bsp_key.h
 * @brief   Key BSP driver — debounce state machine + long/short press detection
 */

#ifndef BSP_KEY_H
#define BSP_KEY_H

#ifdef __cplusplus
extern "C"
{
#endif

//*** Includes ***//
#include "stm32f4xx_hal.h"
#include <stdint.h>

    //*** Type Definitions ***//

    /** @brief  按键事件类型 */
    typedef enum
    {
        BSP_KEY_EVT_NONE = 0,
        BSP_KEY_EVT_PRESSED,     /**< 按下（消抖确认） */
        BSP_KEY_EVT_RELEASED,    /**< 释放（消抖确认） */
        BSP_KEY_EVT_SHORT_PRESS, /**< 短按完成（释放检测） */
        BSP_KEY_EVT_LONG_PRESS,  /**< 长按触发（保持超时） */
    } bsp_key_event_t;

    /** @brief  按键事件回调函数类型 */
    typedef void (*bsp_key_callback_t)(bsp_key_event_t event, void *p_user_data);

    /** @brief  按键硬件配置 */
    typedef struct
    {
        GPIO_TypeDef      *p_port;        /**< GPIO 端口 */
        uint16_t           pin;           /**< GPIO 引脚 */
        uint8_t            active_level;  /**< 0 = 低有效, 1 = 高有效 */
        uint32_t           debounce_ms;   /**< 消抖时长（默认 20ms） */
        uint32_t           long_press_ms; /**< 长按阈值（默认 1000ms） */
        bsp_key_callback_t callback;      /**< 事件回调 */
        void              *p_user_data;   /**< 回调用户上下文 */
    } bsp_key_config_t;

    /** @brief  内部消抖状态 */
    enum
    {
        BSP_KEY_STATE_IDLE       = 0, /**< 等待电平变化 */
        BSP_KEY_STATE_DEBOUNCING = 1, /**< 消抖计时中 */
        BSP_KEY_STATE_STABLE     = 2, /**< 稳定状态，跟踪按压 */
    };

    /** @brief  按键驱动实例 */
    typedef struct
    {
        const bsp_key_config_t *p_config;         /**< 配置引用 */
        uint8_t                 state;            /**< 当前 FSM 状态 */
        uint8_t                 stable_level;     /**< 当前稳定电平 */
        uint32_t                last_tick;        /**< 消抖时间戳 */
        uint32_t                press_tick;       /**< 按下时间戳 */
        uint8_t                 long_press_fired; /**< 1 = 长按已触发 */
    } bsp_key_driver_t;

    //*** Public API ***//

    /** @brief  初始化按键驱动实例 */
    void BspKey_Init(bsp_key_driver_t *p_drv, const bsp_key_config_t *p_config);

    /** @brief  扫描按键状态（建议 5-10ms 周期调用） */
    void BspKey_Scan(bsp_key_driver_t *p_drv);

#ifdef __cplusplus
}
#endif

#endif /* BSP_KEY_H */
