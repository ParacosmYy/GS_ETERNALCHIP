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
 * @brief  初始化 UART BSP 驱动
 * @param  p_huart  HAL UART 句柄（需已完成 HAL 初始化）
 */
void BspUart_Init(UART_HandleTypeDef *p_huart);

/**
 * @brief  阻塞式发送数据
 * @param  p_data      发送数据缓冲区
 * @param  len         发送字节数
 * @param  timeout_ms  超时时间（毫秒）
 * @retval 0   成功
 * @retval -1  超时或发送失败
 */
int BspUart_SendBlocking(const uint8_t *p_data, uint16_t len, uint32_t timeout_ms);

/**
 * @brief  格式化打印输出（阻塞式，使用栈缓冲区）
 * @param  fmt  格式化字符串
 * @param  ...  可变参数
 * @return 实际写入字符数，失败返回 -1
 */
int BspUart_Printf(const char *fmt, ...);

#endif /* BSP_UART_H */
