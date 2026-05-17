/**
 * @file    bsp_uart.c
 * @brief   Simplified UART BSP driver — blocking TX + Printf (bootloader only)
 */

//*** Includes ***//
#include "bsp_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

//*** Private Variables ***//

static UART_HandleTypeDef *s_p_huart = NULL;

//*** Public API ***//

/**
 * @brief  初始化 UART BSP 驱动
 * @param  p_huart  HAL UART 句柄（需已完成 HAL 初始化，如 &huart1）
 */
void BspUart_Init(UART_HandleTypeDef *p_huart)
{
    s_p_huart = p_huart;
}

/**
 * @brief  阻塞式发送数据
 * @param  p_data      发送数据缓冲区
 * @param  len         发送字节数
 * @param  timeout_ms  超时时间（毫秒）
 * @retval 0   成功
 * @retval -1  句柄无效、参数错误或 HAL 超时
 */
int BspUart_SendBlocking(const uint8_t *p_data, uint16_t len, uint32_t timeout_ms)
{
    if (s_p_huart == NULL || p_data == NULL || len == 0)
    {
        return -1;
    }

    if (HAL_UART_Transmit(s_p_huart, p_data, len, timeout_ms) != HAL_OK)
    {
        return -1;
    }
    return 0;
}

/**
 * @brief  格式化打印输出（阻塞式，使用栈缓冲区）
 * @param  fmt  格式化字符串
 * @param  ...  可变参数
 * @return 实际写入字符数，失败返回 -1
 */
int BspUart_Printf(const char *fmt, ...)
{
    char    buf[BSP_UART_PRINTF_BUF_SIZE];
    va_list args;
    int     len;

    va_start(args, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len <= 0)
    {
        return -1;
    }

    if ((uint16_t)len > sizeof(buf) - 1)
    {
        len = (int)(sizeof(buf) - 1);
    }

    if (BspUart_SendBlocking((const uint8_t *)buf, (uint16_t)len, 100) != 0)
    {
        return -1;
    }
    return len;
}
