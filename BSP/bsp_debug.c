/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 * 
 * @file bsp_debug.c
 * 
 * @par dependencies 
 * - bsp_debug.h
 * - usart.h
 * - FreeRTOS.h
 * - task.h
 * - stdio.h
 * 
 * @author GS_Mark
 *
 * @brief 调试输出实现，fputc 重定向到 UART + 调度器挂起保护
 * 
 * Processing flow:
 * printf() → fputc() → vTaskSuspendAll → HAL_UART_Transmit → xTaskResumeAll
 * 
 * @version V1.0 2025-04-26
 *
 * @note 1 tab == 4 spaces
 * 
 *****************************************************************************/

//******************************** Includes *********************************//
#include "bsp_debug.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
//******************************** Includes *********************************//

//******************************** Implement ********************************//

/**
 * @brief fputc 重定向，将 printf 输出逐字符发送到 UART
 * 
 * Steps:
 *  1. 挂起调度器（单字符约87µs@115200，临界区极短）
 *  2. 调用 HAL_UART_Transmit 轮询发送单个字符
 *  3. 恢复调度器
 * 
 * @param[in] ch : 待发送的字符
 * @param[in] f  : 文件指针（未使用）
 * 
 * @return int : 发送的字符
 * 
 * */
int fputc(int ch, FILE *f)
{
    (void)f;

    vTaskSuspendAll();
    (void)HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
    xTaskResumeAll();

    return ch;
}
