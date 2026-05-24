/**
 * @file    bsp_led_driver.h
 * @brief   LED BSP 驱动 — HAL 后端薄封装
 * @author  GS_Mark
 *
 * @par dependencies
 * - stm32f4xx_hal.h
 * - plat_led.h
 *
 * @note    本头文件供需要 HAL 类型的 Impl 层消费者使用。
 *          上层（Service/App）应直接包含 plat_led.h。
 */

#ifndef BSP_LED_DRIVER_H
#define BSP_LED_DRIVER_H

#ifdef __cplusplus
extern "C"
{
#endif

//*** 头文件 ***//
#include "stm32f4xx_hal.h"
#include "plat_led.h"

//*** 可配置宏 ***//

#ifndef BSP_LED_MAX_INSTANCES
#    define BSP_LED_MAX_INSTANCES 4
#endif

#ifdef __cplusplus
}
#endif

#endif /* BSP_LED_DRIVER_H */
