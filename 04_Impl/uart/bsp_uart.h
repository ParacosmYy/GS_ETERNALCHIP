/**
 * @file    bsp_uart.h
 * @brief   UART BSP driver — DMA + IDLE reception, Printf support
 * @author  GS_Mark
 *
 * @note    All types (bsp_uart_driver_t, bsp_uart_config_t, etc.) and API
 *          declarations are in plat_uart.h (03_Platform/interface).
 *          This header only adds HAL includes and configurable macros
 *          needed by the implementation (bsp_uart.c).
 */

#ifndef BSP_UART_H
#define BSP_UART_H

#ifdef __cplusplus
extern "C"
{
#endif

//*** Includes ***//
#include "stm32f4xx_hal.h"
#include "plat_uart.h"

    //*** Configurable Macros ***//

#ifndef BSP_UART_MAX_INSTANCES
#    define BSP_UART_MAX_INSTANCES 2
#endif

#ifndef BSP_UART_PRINTF_BUF_SIZE
#    define BSP_UART_PRINTF_BUF_SIZE 128
#endif

#ifdef __cplusplus
}
#endif

#endif /* BSP_UART_H */
