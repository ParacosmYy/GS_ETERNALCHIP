/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 * 
 * @file app_stack_monitor.h
 * 
 * @par dependencies 
 * - cmsis_os2.h
 * - task.h (FreeRTOS)
 * 
 * @author GS_Mark
 *
 * @brief 任务栈 High Water Mark 监测模块
 * 
 * Usage:
 * 1. 在本文件中设置 STACK_MONITOR_ENABLE 为 1 启用
 * 2. 在任务初始化阶段调用 App_StackMonitor_Register() 注册待监测任务
 * 3. 在适当时机调用 App_StackMonitor_Print() 打印栈水位
 * 4. Release 版本将 STACK_MONITOR_ENABLE 置 0，完全编译剔除
 * 
 * @version V1.0 2025-04-26
 *
 * @note 1 tab == 4 spaces
 * 
 *****************************************************************************/
#ifndef __APP_STACK_MONITOR_H__
#define __APP_STACK_MONITOR_H__

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include "cmsis_os2.h"
//******************************* Includes **********************************//

//******************************* Defines ***********************************//
/* 编译期总开关：置 1 启用栈监测，置 0 完全剔除，零 RAM / Flash 开销 */
#define STACK_MONITOR_ENABLE        1

/* 最大监测任务数 */
#define STACK_MONITOR_MAX_TASKS     8
//******************************* Defines ***********************************//

//******************************* Types *************************************//
/**
 * @brief 栈监测任务描述
 * */
typedef struct {
    osThreadId_t  handle;        /* 任务句柄 */
    const char   *p_name;        /* 任务名称（字符串字面量） */
    uint16_t      stack_words;   /* 任务栈总大小（字） */
} stack_monitor_task_info_t;

//******************************* Declaring *********************************//

#if STACK_MONITOR_ENABLE

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
                                  uint16_t stack_words);

/**
 * @brief 打印所有已注册任务的栈 High Water Mark
 * 
 * Steps:
 *  1. 遍历已注册任务列表
 *  2. 调用 uxTaskGetStackHighWaterMark() 获取水位
 *  3. printf 格式化输出
 * 
 * */
void App_StackMonitor_Print(void);

#else

/* 禁用时替换为空操作，零开销 */
#define App_StackMonitor_Register(handle, p_name, stack_words)  ((int8_t)0)
#define App_StackMonitor_Print()                                 ((void)0)

#endif /* STACK_MONITOR_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* __APP_STACK_MONITOR_H__ */
