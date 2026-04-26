/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 * 
 * @file app_uart_task.c
 * 
 * @par dependencies 
 * - app_uart_task.h
 * - app_key_task.h
 * - usart.h
 * 
 * @author GS_Mark
 *
 * @brief UART 应用任务实现，支持 echo 回显、心跳输出、按键事件响应
 * 
 * Processing flow:
 * DMA空闲中断接收 → 回调置标志 → 任务内 echo 回显
 * 按键队列消息 → 发送 "LED Toggle" 字符串
 * 定时1s → 发送 "heartbeat" 字符串
 * 
 * @version V1.0 2025-04-26
 *
 * @note 1 tab == 4 spaces
 * 
 *****************************************************************************/

//******************************** Includes *********************************//
#include "app_uart_task.h"
#include "app_key_task.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
//******************************** Includes *********************************//

//******************************** Defines **********************************//
#define UART_TASK_STACK_SIZE   256 * 4   /* 单位: 字，UART需要较大栈 */
#define UART_TASK_PRIORITY     osPriorityNormal
#define UART_RX_BUF_SIZE       64
#define UART_TASK_PERIOD_MS    10
#define HEARTBEAT_INTERVAL_MS  1000

//******************************** Variables ********************************//
static uint8_t g_uart_rx_buf[UART_RX_BUF_SIZE];
static volatile uint16_t g_uart_rx_len  = 0;
static volatile uint8_t  g_uart_rx_flag = 0;

static osThreadId_t g_uart_task_handle = NULL;

//******************************** Declaring ********************************//
static void StartUARTTask(void *argument);

//******************************** Implement ********************************//

/**
 * @brief 初始化 UART 任务和 DMA 接收
 * 
 * Steps:
 *  1. 创建 UART 任务
 *  2. 启动 HAL_UARTEx_ReceiveToIdle_DMA 接收
 * 
 * @return osStatus_t : osOK 表示成功
 * 
 * */
osStatus_t App_UART_Init(void)
{
    const osThreadAttr_t uart_task_attr = {
        .name       = "uartTask",
        .stack_size = UART_TASK_STACK_SIZE,
        .priority   = (osPriority_t) UART_TASK_PRIORITY,
    };

    g_uart_task_handle = osThreadNew(StartUARTTask, NULL, &uart_task_attr);
    if (g_uart_task_handle == NULL)
    {
        return osError;
    }

    return osOK;
}

/**
 * @brief UART 任务函数
 * 
 * Steps:
 *  1. 启动 DMA 空闲中断接收
 *  2. 主循环中：
 *     a. 检查按键队列消息，有事件则发送 "LED Toggle"
 *     b. 检查 DMA 接收标志，有数据则 echo 回显
 *     c. 定时发送心跳字符串
 * 
 * @param[in] argument : 未使用
 * 
 * */
static void StartUARTTask(void *argument)
{
    (void)argument;
    uint32_t heartbeat_tick = osKernelGetTickCount();

    QueueHandle_t key_queue = App_Key_GetQueue();

    /* 启动 DMA 接收 */
    (void)HAL_UARTEx_ReceiveToIdle_DMA(&huart1, g_uart_rx_buf, UART_RX_BUF_SIZE);

    for (;;)
    {
        /* ---- 按键事件响应 ---- */
        app_key_event_t evt;
        if (xQueueReceive(key_queue, &evt, 0) == pdPASS)
        {
            printf("LED Toggle\r\n");
        }

        /* ---- UART echo 回显 ---- */
        if (g_uart_rx_flag)
        {
            (void)HAL_UART_Transmit(&huart1, g_uart_rx_buf, g_uart_rx_len, 100);
            g_uart_rx_flag = 0;
            g_uart_rx_len  = 0;
        }

        /* ---- 心跳 ---- */
        if ((osKernelGetTickCount() - heartbeat_tick) >= HEARTBEAT_INTERVAL_MS)
        {
            heartbeat_tick = osKernelGetTickCount();
            printf("heartbeat\r\n");
        }

        osDelay(UART_TASK_PERIOD_MS);
    }
}

/**
 * @brief UART 空闲中断 + DMA 接收完成回调
 * 
 * Steps:
 *  1. 检查是否为 USART1 实例
 *  2. 记录接收数据长度并置标志
 *  3. 重新启动 DMA 接收
 * 
 * @param[in] huart : UART 句柄指针
 * @param[in] size  : 接收到的数据长度
 * 
 * */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
    if (huart->Instance == USART1)
    {
        g_uart_rx_len  = size;
        g_uart_rx_flag = 1;
        (void)HAL_UARTEx_ReceiveToIdle_DMA(&huart1, g_uart_rx_buf, UART_RX_BUF_SIZE);
    }
}
