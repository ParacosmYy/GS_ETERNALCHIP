/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 * 
 * @file app_uart_task.c
 * 
 * @par dependencies 
 * - app_uart_task.h
 * - app_key_task.h
 * - app_stack_monitor.h
 * - usart.h
 * 
 * @author GS_Mark
 *
 * @brief UART 应用任务实现，支持 echo 回显、心跳输出、按键事件响应
 * 
 * Processing flow:
 * DMA空闲中断接收 → 回调置标志 → 任务内 echo 回显
 * 按键队列消息 → 发送 "LED Toggle" 字符串
 * 定时1s → 发送 "heartbeat" 字符串（可编译期关闭）
 * 
 * @version V1.1 2025-04-26
 *
 * @note 1 tab == 4 spaces
 * 
 *****************************************************************************/

//******************************** Includes *********************************//
#include "FreeRTOS.h"            /* 必须第一个包含 */
#include "task.h"                 /* TaskHandle_t */

#include "app_uart_task.h"       /* cmsis_os2.h */
#include "app_key_task.h"        /* App_Key_GetTaskHandle */
#include "app_stack_monitor.h"   /* App_StackMonitor_Register / Print */
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

/* 调试开关：置 0 关闭心跳，置 1 开启心跳 */
#define UART_DEBUG_ENABLE_HEARTBEAT  0

/* 栈水位检测：启动后延时打印（毫秒） */
#define STACK_CHECK_DELAY_MS         5000
//******************************** Defines **********************************//

//******************************** Variables ********************************//
static uint8_t g_uart_rx_buf[UART_RX_BUF_SIZE];
static volatile uint16_t g_uart_rx_len  = 0;
static volatile uint8_t  g_uart_rx_flag = 0;

static osThreadId_t g_uart_task_handle = NULL;
//******************************** Variables ********************************//

//******************************** Declaring ********************************//
static void StartUARTTask(void *argument);
//******************************** Declaring ********************************//

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
 *  1. 注册栈监测任务（STACK_MONITOR_ENABLE 控制编译期开关）
 *  2. 启动 DMA 空闲中断接收
 *  3. 主循环中：
 *     a. 定时打印栈 High Water Mark（仅一次，STACK_MONITOR_ENABLE 控制）
 *     b. 检查按键队列消息，有事件则发送 "LED Toggle"
 *     c. 检查 DMA 接收标志，有数据则 echo 回显
 *     d. 定时发送心跳字符串（UART_DEBUG_ENABLE_HEARTBEAT 控制）
 * 
 * @param[in] argument : 未使用
 * 
 * */
static void StartUARTTask(void *argument)
{
    (void)argument;

#if UART_DEBUG_ENABLE_HEARTBEAT
    uint32_t heartbeat_tick = osKernelGetTickCount();
#endif

    QueueHandle_t key_queue = App_Key_GetQueue();

#if STACK_MONITOR_ENABLE
    /* 注册栈监测任务 */
    extern osThreadId_t defaultTaskHandle;  /* freertos.c 中定义 */
    (void)App_StackMonitor_Register(defaultTaskHandle,        "defaultTask", 128);
    (void)App_StackMonitor_Register(App_Key_GetTaskHandle(),  "keyTask",     128);
    (void)App_StackMonitor_Register(g_uart_task_handle,       "uartTask",    256);

    uint32_t init_tick     = osKernelGetTickCount();
    uint8_t  stack_printed = 0;
#endif

    /* 启动 DMA 接收 */
    (void)HAL_UARTEx_ReceiveToIdle_DMA(&huart1, g_uart_rx_buf, UART_RX_BUF_SIZE);

    for (;;)
    {
        /* ---- 栈 High Water Mark 打印（仅一次，启动 5s 后） ---- */
#if STACK_MONITOR_ENABLE
        if (!stack_printed
            && (osKernelGetTickCount() - init_tick) >= STACK_CHECK_DELAY_MS)
        {
            stack_printed = 1;
            App_StackMonitor_Print();
        }
#endif

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
#if UART_DEBUG_ENABLE_HEARTBEAT
        if ((osKernelGetTickCount() - heartbeat_tick) >= HEARTBEAT_INTERVAL_MS)
        {
            heartbeat_tick = osKernelGetTickCount();
            printf("heartbeat\r\n");
        }
#endif

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

/**
 * @brief 获取 UART 任务句柄
 * 
 * @return osThreadId_t : UART 任务句柄
 * 
 * */
osThreadId_t App_UART_GetTaskHandle(void)
{
    return g_uart_task_handle;
}
