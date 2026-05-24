/**
 * @file    system_adaption.h
 * @brief   系统适配层 — 集中声明所有 HAL/FreeRTOS 操作接口
 * @author  GS_Mark
 *
 * @par dependencies
 * - 所有 plat_*.h 头文件
 *
 * @note    所有 HAL 操作接口实例的唯一头文件。
 *          上层只需包含此头文件，无需了解 HAL 内部细节。
 */

#ifndef SYSTEM_ADAPTION_H
#define SYSTEM_ADAPTION_H

#ifdef __cplusplus
extern "C" {
#endif

//*** 头文件 ***//
#include "plat_led.h"
#include "plat_key.h"
#include "plat_wdg.h"
#include "plat_sys.h"
#include "plat_rtt.h"
#include "plat_flash.h"
#include "plat_uart.h"

//*** 外部 HAL 操作接口实例 ***//

/* LED — 命名保持 bsp_led_hal_ops 以匹配 plat_led.h 的 extern 声明 */
extern const led_operations_t bsp_led_hal_ops;
extern const led_os_operations_t g_led_os_ops;

// 按键
extern const key_hw_operations_t g_key_hal_ops;
extern const key_os_operations_t g_key_os_ops;

// 看门狗
extern const wdg_hw_operations_t g_wdg_hal_ops;

// 系统服务
extern const sys_operations_t g_sys_hal_ops;

// RTT 调试输出
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
