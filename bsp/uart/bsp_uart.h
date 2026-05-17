/**
 * @file    bsp_uart.h
 * @brief   Simplified UART BSP — blocking TX + Printf (bootloader only)
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

/** @brief  初始化 UART BSP 驱动 */
void BspUart_Init(UART_HandleTypeDef *p_huart);

/** @brief  阻塞式发送数据 */
int BspUart_SendBlocking(const uint8_t *p_data, uint16_t len, uint32_t timeout_ms);

/** @brief  格式化打印输出（阻塞式） */
int BspUart_Printf(const char *fmt, ...);

#endif /* BSP_UART_H */
