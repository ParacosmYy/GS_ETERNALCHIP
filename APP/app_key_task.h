/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 * 
 * @file app_key_task.h
 * 
 * @par dependencies 
 * - cmsis_os2.h
 * 
 * @author GS_Mark
 *
 * @brief 按键应用任务模块，负责按键扫描并通过消息队列通知其他任务
 * 
 * Usage:
 * App_Key_Init()  - 创建按键任务及消息队列（在 MX_FREERTOS_Init 中调用）
 * App_Key_GetQueue() - 获取按键消息队列句柄（供其他任务接收）
 * 
 * @version V1.0 2025-04-26
 *
 * @note 1 tab == 4 spaces
 * 
 *****************************************************************************/
#ifndef __APP_KEY_TASK_H__
#define __APP_KEY_TASK_H__

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include "cmsis_os2.h"
//******************************* Includes **********************************//

//******************************* Defines ***********************************//
/* 按键事件类型 */
typedef enum {
    KEY_EVENT_NONE    = 0,
    KEY_EVENT_PRESSED = 1
} app_key_event_t;

//******************************* Declaring *********************************//

/**
 * @brief 初始化按键任务和消息队列
 * 
 * Steps:
 *  1. 创建按键事件消息队列
 *  2. 创建按键扫描任务
 * 
 * @return osStatus_t : osOK 表示成功
 * 
 * */
osStatus_t App_Key_Init(void);

/**
 * @brief 获取按键消息队列句柄
 * 
 * @return osMessageQueueId_t : 队列句柄，供其他任务接收按键事件
 * 
 * */
osMessageQueueId_t App_Key_GetQueue(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_KEY_TASK_H__ */
