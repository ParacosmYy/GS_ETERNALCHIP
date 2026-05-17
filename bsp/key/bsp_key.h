/**
 * @file    bsp_key.h
 * @brief   Key BSP driver — debounce state machine + long/short press detection
 * @note    Call BspKey_Scan() periodically (recommended 5-10 ms) from a timer
 *          or RTOS task. The driver is bare-metal friendly (no RTOS dependency).
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

    /** @brief  Key event types reported via callback */
    typedef enum
    {
        BSP_KEY_EVT_NONE = 0,
        BSP_KEY_EVT_PRESSED,     /**< Key pressed (debounce confirmed)          */
        BSP_KEY_EVT_RELEASED,    /**< Key released (debounce confirmed)         */
        BSP_KEY_EVT_SHORT_PRESS, /**< Short press completed (release detected)  */
        BSP_KEY_EVT_LONG_PRESS,  /**< Long press detected (hold timeout)        */
    } bsp_key_event_t;

    /** @brief  Key event callback function type */
    typedef void (*bsp_key_callback_t)(bsp_key_event_t event, void *p_user_data);

    /**
     * @brief  Key hardware configuration (immutable, usually const)
     */
    typedef struct
    {
        GPIO_TypeDef      *p_port;        /**< GPIO port                    */
        uint16_t           pin;           /**< GPIO pin                     */
        uint8_t            active_level;  /**< 0 = low-active, 1 = high    */
        uint32_t           debounce_ms;   /**< Debounce duration (default 20)   */
        uint32_t           long_press_ms; /**< Long press threshold (default 1000) */
        bsp_key_callback_t callback;      /**< Event callback               */
        void              *p_user_data;   /**< User context passed to cb    */
    } bsp_key_config_t;

    /** @brief  Internal debounce states */
    enum
    {
        BSP_KEY_STATE_IDLE       = 0, /**< Waiting for level change     */
        BSP_KEY_STATE_DEBOUNCING = 1, /**< Debounce timer running       */
        BSP_KEY_STATE_STABLE     = 2, /**< Stable state, tracking press */
    };

    /**
     * @brief  Key driver instance (one per physical key)
     */
    typedef struct
    {
        const bsp_key_config_t *p_config;         /**< Configuration reference     */
        uint8_t                 state;            /**< Current FSM state           */
        uint8_t                 stable_level;     /**< Current stable pin level    */
        uint32_t                last_tick;        /**< Timestamp for debounce      */
        uint32_t                press_tick;       /**< Timestamp when pressed      */
        uint8_t                 long_press_fired; /**< 1 = long press already sent */
    } bsp_key_driver_t;

    //*** Public API ***//

    /**
     * @brief  初始化按键驱动实例
     * @param  p_drv    驱动实例（调用者分配）
     * @param  p_config 硬件配置（需保持有效，通常为 const 全局变量）
     */
    void BspKey_Init(bsp_key_driver_t *p_drv, const bsp_key_config_t *p_config);

    /**
     * @brief  扫描按键状态（建议 5-10 ms 周期调用）
     *         运行消抖状态机，检测到事件时触发回调。
     * @param  p_drv  驱动实例
     */
    void BspKey_Scan(bsp_key_driver_t *p_drv);

#ifdef __cplusplus
}
#endif

#endif /* BSP_KEY_H */
