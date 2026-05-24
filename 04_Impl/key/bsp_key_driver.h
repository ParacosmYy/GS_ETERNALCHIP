/**
 * @file    bsp_key_driver.h
 * @brief   Key BSP driver — debounce state machine + long/short press detection
 * @author  GS_Mark
 *
 * @note    Types and API declarations are in plat_key.h (03_Platform/interface).
 *          This header exposes the FSM states for Impl-layer consumers.
 */

#ifndef BSP_KEY_DRIVER_H
#define BSP_KEY_DRIVER_H

#ifdef __cplusplus
extern "C"
{
#endif

//*** Includes ***//
#include "stm32f4xx_hal.h"
#include "plat_key.h"

    //*** FSM States (Impl-layer only) ***//

    enum
    {
        BSP_KEY_STATE_IDLE       = 0, /**< Waiting for level change */
        BSP_KEY_STATE_DEBOUNCING = 1, /**< Debounce timer running */
        BSP_KEY_STATE_STABLE     = 2, /**< Stable state, tracking press */
    };

    //*** Default HAL OPS Implementation ***//

    /** @brief  Default HAL implementation of pf_read_pin */
    uint8_t Key_Hal_ReadPin(const bsp_key_config_t *p_cfg);

    /** @brief  Default HAL implementation of pf_get_tick */
    uint32_t Key_Hal_GetTick(void);

    /** @brief  Default HAL ops instances (defined in bsp_key_driver.c) */
    extern const key_hw_operations_t g_key_hal_ops;
    extern const key_os_operations_t g_key_os_ops;

#ifdef __cplusplus
}
#endif

#endif /* BSP_KEY_DRIVER_H */
