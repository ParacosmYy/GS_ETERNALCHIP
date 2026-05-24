/**
 * @file    bsp_led_driver.h
 * @brief   LED BSP driver — HAL backend thin wrapper
 * @author  GS_Mark
 *
 * @par dependencies
 * - stm32f4xx_hal.h
 * - plat_led.h
 *
 * @note    This header is for Impl-layer consumers that need HAL types.
 *          Upper layers (Service/App) should include plat_led.h directly.
 */

#ifndef BSP_LED_DRIVER_H
#define BSP_LED_DRIVER_H

#ifdef __cplusplus
extern "C"
{
#endif

//*** Includes ***//
#include "stm32f4xx_hal.h"
#include "plat_led.h"

#ifdef __cplusplus
}
#endif

#endif /* BSP_LED_DRIVER_H */
