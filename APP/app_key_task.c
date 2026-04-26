/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 * 
 * @file app_key_task.c
 * 
 * @par dependencies 
 * - app_key_task.h
 * - bsp_key.h
 * - bsp_led.h
 * 
 * @author GS_Mark
 *
 * @brief 按键应用任务实现，轮询扫描按键并发送事件到消息队列
 * 
 * Processing flow:
 * BSP_Key_Scan 检测按下 → BSP_LED_Toggle 翻转 LED → 队列发送事件
 * 
 * @version V1.0 2025-04-26
 *
 * @note 1 tab == 4 spaces
 * 
 *****************************************************************************/

//******************************** Includes *********************************//
#include "app_key_task.h"
#include "bsp_key.h"
#include "bsp_led.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
#define KEY_TASK_STACK_SIZE  128 * 4   /* 单位: 字 */
#define KEY_TASK_PRIORITY    osPriorityNormal
#define KEY_TASK_PERIOD_MS   10
#define KEY_QUEUE_SIZE       4

//******************************** Variables ********************************//
static osMessageQueueId_t g_key_queue = NULL;
static osThreadId_t       g_key_task_handle = NULL;

//******************************** Declaring ********************************//
static void StartKeyTask(void *argument);

//******************************** Implement ********************************//

/**
 * @brief 初始化按键任务和消息队列
 * 
 * Steps:
 *  1. 创建按键事件消息队列（容量4，每项 sizeof(app_key_event_t)）
 *  2. 创建按键扫描任务
 * 
 * @return osStatus_t : osOK 表示成功
 * 
 * */
osStatus_t App_Key_Init(void)
{
    g_key_queue = osMessageQueueNew(KEY_QUEUE_SIZE, sizeof(app_key_event_t), NULL);
    if (g_key_queue == NULL)
    {
        return osError;
    }

    const osThreadAttr_t key_task_attr = {
        .name       = "keyTask",
        .stack_size = KEY_TASK_STACK_SIZE,
        .priority   = (osPriority_t) KEY_TASK_PRIORITY,
    };

    g_key_task_handle = osThreadNew(StartKeyTask, NULL, &key_task_attr);
    if (g_key_task_handle == NULL)
    {
        return osError;
    }

    return osOK;
}

/**
 * @brief 获取按键消息队列句柄
 * 
 * @return osMessageQueueId_t : 队列句柄
 * 
 * */
osMessageQueueId_t App_Key_GetQueue(void)
{
    return g_key_queue;
}

/**
 * @brief 按键扫描任务函数
 * 
 * Steps:
 *  1. 维护 last_key 状态（上拉输入，松开=1）
 *  2. 每 10ms 调用 BSP_Key_Scan 检测按键
 *  3. 检测到有效按下时翻转 LED 并向队列发送事件
 * 
 * @param[in] argument : 未使用
 * 
 * */
static void StartKeyTask(void *argument)
{
    (void)argument;
    uint8_t last_key = 1;  /* 上拉输入，松开为1 */

    for (;;)
    {
        if (BSP_Key_Scan(&last_key) == KEY_PRESSED)
        {
            (void)BSP_LED_Toggle();

            app_key_event_t evt = KEY_EVENT_PRESSED;
            (void)osMessageQueuePut(g_key_queue, &evt, 0, 0);
        }

        osDelay(KEY_TASK_PERIOD_MS);
    }
}
