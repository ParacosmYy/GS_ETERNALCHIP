/**
 * @file    bsp_uart_driver.h
 * @brief   UART BSP 驱动 — DMA + IDLE 接收，Printf 支持
 * @author  GS_Mark
 *
 * @note    所有类型（bsp_uart_driver_t、bsp_uart_config_t 等）和 API
 *          声明位于 plat_uart.h（03_Platform/interface）。
 *          本头文件仅添加实现所需的 HAL 头文件和可配置宏。
 */

#ifndef BSP_UART_DRIVER_H
#define BSP_UART_DRIVER_H

#ifdef __cplusplus
extern "C"
{
#endif

//*** 头文件 ***//
#include "stm32f4xx_hal.h"
#include "plat_uart.h"

    //*** 可配置宏 ***//

#ifndef BSP_UART_MAX_INSTANCES
#    define BSP_UART_MAX_INSTANCES 2
#endif

#ifndef BSP_UART_PRINTF_BUF_SIZE
#    define BSP_UART_PRINTF_BUF_SIZE 128
#endif

#ifdef __cplusplus
}
#endif

#endif /* BSP_UART_DRIVER_H */
