/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 * 
 * @file app_uart_task.h
 * 
 * @par dependencies 
 * - cmsis_os2.h
 * 
 * @author GS_Mark
 *
 * @brief UART 应用任务模块，负责串口收发和心跳输出
 * 
 * Usage:
 * App_UART_Init()  - 创建 UART 任务及初始化 DMA 接收
 * 
 * @version V1.0 2025-04-26
 *
 * @note 1 tab == 4 spaces
 * 
 *****************************************************************************/
#ifndef __APP_UART_TASK_H__
#define __APP_UART_TASK_H__

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include "cmsis_os2.h"
//******************************* Includes **********************************//

//******************************* Declaring *********************************//

/**
 * @brief 初始化 UART 任务和 DMA 接收
 * 
 * Steps:
 *  1. 创建 UART 任务
 *  2. 启动 DMA 空闲中断接收
 * 
 * @return osStatus_t : osOK 表示成功
 * 
 * */
osStatus_t App_UART_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_UART_TASK_H__ */
