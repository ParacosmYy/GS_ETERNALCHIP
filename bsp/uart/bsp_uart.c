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

void BspUart_Init(UART_HandleTypeDef *p_huart)
{
    s_p_huart = p_huart;
}

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
