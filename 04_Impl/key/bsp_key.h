/**
 * @file    bsp_key.h
 * @brief   Key BSP driver — debounce state machine + long/short press detection
 * @author  GS_Mark
 *
 * @note    Types and API declarations are in plat_key.h (03_Platform/interface).
 *          This header exposes the FSM states for Impl-layer consumers.
 */

#ifndef BSP_KEY_H
#define BSP_KEY_H

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

#ifdef __cplusplus
}
#endif

#endif /* BSP_KEY_H */
