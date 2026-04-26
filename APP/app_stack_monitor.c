/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 * 
 * @file app_stack_monitor.c
 * 
 * @par dependencies 
 * - app_stack_monitor.h
 * 
 * @author GS_Mark
 *
 * @brief 任务栈 High Water Mark 监测模块实现
 * 
 * @version V1.0 2025-04-26
 *
 * @note 1 tab == 4 spaces
 * 
 *****************************************************************************/

//******************************** Includes *********************************//
#include "FreeRTOS.h"                /* 必须第一个包含 */
#include "task.h"                     /* uxTaskGetStackHighWaterMark */

#include "app_stack_monitor.h"

#include <stdio.h>
//******************************** Includes *********************************//

#if STACK_MONITOR_ENABLE

//******************************** Variables ********************************//
static stack_monitor_task_info_t g_monitored_tasks[STACK_MONITOR_MAX_TASKS];
static uint8_t g_monitored_count = 0;
//******************************** Variables ********************************//

//******************************** Implement ********************************//

/**
 * @brief 注册一个待监测的任务
 * 
 * Steps:
 *  1. 检查是否超出最大监测数量
 *  2. 保存任务句柄、名称、栈大小
 * 
 * @param[in] handle      : 任务句柄
 * @param[in] p_name      : 任务名称字符串
 * @param[in] stack_words : 任务栈总大小（字）
 * 
 * @return int8_t : 注册成功返回索引，失败返回 -1
 * 
 * */
int8_t App_StackMonitor_Register(osThreadId_t handle, const char *p_name,
                                  uint16_t stack_words)
{
    if (g_monitored_count >= STACK_MONITOR_MAX_TASKS)
    {
        return -1;
    }

    g_monitored_tasks[g_monitored_count].handle      = handle;
    g_monitored_tasks[g_monitored_count].p_name       = p_name;
    g_monitored_tasks[g_monitored_count].stack_words  = stack_words;

    return (int8_t)g_monitored_count++;
}

/**
 * @brief 打印所有已注册任务的栈 High Water Mark
 * 
 * Steps:
 *  1. 遍历已注册任务列表
 *  2. 调用 uxTaskGetStackHighWaterMark() 获取水位
 *  3. 计算已用字节数 = (总字数 - hwm) × 4
 *  4. printf 格式化输出
 * 
 * @note hwm = 历史剩余最小空间（字），used = (总字数 - hwm) × 4 字节
 * 
 * */
void App_StackMonitor_Print(void)
{
    UBaseType_t hwm;
    uint16_t    stack_words;

    printf("=== Stack High Water Mark ===\r\n");

    for (uint8_t i = 0; i < g_monitored_count; i++)
    {
        stack_words = g_monitored_tasks[i].stack_words;
        hwm = uxTaskGetStackHighWaterMark(
                  (TaskHandle_t)g_monitored_tasks[i].handle);

        printf("%-11s : total=%u, hwm=%u words, used=%u bytes\r\n",
               g_monitored_tasks[i].p_name,
               (unsigned int)(stack_words * 4),
               (unsigned int)hwm,
               (unsigned int)((stack_words - hwm) * 4));
    }
}

#endif /* STACK_MONITOR_ENABLE */
