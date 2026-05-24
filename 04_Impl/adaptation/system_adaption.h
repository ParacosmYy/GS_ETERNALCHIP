/**
 * @file    system_adaption.h
 * @brief   System Adaption Layer — centralized HAL/FreeRTOS ops declarations
 * @author  GS_Mark
 *
 * @par dependencies
 * - All plat_*.h headers
 *
 * @note    This is the SINGLE header for all HAL ops instances.
 *          Upper layers include this instead of knowing about HAL internals.
 */

#ifndef SYSTEM_ADAPTION_H
#define SYSTEM_ADAPTION_H

#ifdef __cplusplus
extern "C" {
#endif

//*** Includes ***//
#include "plat_led.h"
#include "plat_key.h"
#include "plat_wdg.h"
#include "plat_sys.h"
#include "plat_rtt.h"
#include "plat_flash.h"
#include "plat_uart.h"

//*** Extern HAL Ops Instances ***//

/* LED — name kept as bsp_led_hal_ops to match plat_led.h extern declaration */
extern const led_operations_t bsp_led_hal_ops;

// Key
extern const key_hw_operations_t g_key_hal_ops;
extern const key_os_operations_t g_key_os_ops;

// WDG
extern const wdg_hw_operations_t g_wdg_hal_ops;

// SYS
extern const sys_operations_t g_sys_hal_ops;

// RTT
extern const rtt_operations_t g_rtt_hal_ops;

// Flash
extern const flash_hw_operations_t g_flash_hal_ops;
extern const flash_os_operations_t g_flash_os_ops;
extern const flash_time_operations_t g_flash_time_ops;
extern const bsp_flash_config_t g_flash_config;

// UART
extern const uart_hw_operations_t g_uart_hal_ops;
extern const uart_os_operations_t g_uart_os_ops;

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_ADAPTION_H */
