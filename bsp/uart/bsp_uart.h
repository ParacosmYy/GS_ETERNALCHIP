/**
 * @file    bsp_uart.h
 * @brief   Simplified UART BSP — blocking TX and Printf (bootloader only)
 */

#ifndef BSP_UART_H
#define BSP_UART_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

//*** Configurable Macros ***//

#ifndef BSP_UART_PRINTF_BUF_SIZE
#    define BSP_UART_PRINTF_BUF_SIZE 128
#endif

//*** Public API ***//

/**
 * @brief  Initialize UART BSP driver
 * @param  p_huart  HAL UART handle (must already be initialized)
 */
void BspUart_Init(UART_HandleTypeDef *p_huart);

/**
 * @brief  Blocking transmit with timeout
 * @return 0 success, -1 error
 */
int BspUart_SendBlocking(const uint8_t *p_data, uint16_t len, uint32_t timeout_ms);

/**
 * @brief  Printf-style formatted output (blocking)
 * @return chars written, or -1 on error
 */
int BspUart_Printf(const char *fmt, ...);

#endif /* BSP_UART_H */
