/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 * 
 * @file bsp_key.h
 * 
 * @par dependencies 
 * - main.h
 * 
 * @author GS_Mark
 *
 * @brief 按键硬件抽象层，提供带消抖的按键扫描接口
 * 
 * Usage:
 * 1. 定义 bsp_key_time_ops_t 实例，注入延时/时基函数指针
 * 2. 调用 BSP_Key_Init(&time_ops) 初始化
 * 3. 轮询调用 BSP_Key_Scan() 检测按键事件
 * 
 * @version V2.0 2025-04-26
 *
 * @note 1 tab == 4 spaces
 * 
 *****************************************************************************/
#ifndef __BSP_KEY_H__
#define __BSP_KEY_H__

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include "main.h"
//******************************* Includes **********************************//

//******************************* Types *************************************//
/**
 * @brief BSP 按键时间操作接口（由 APP 层注入具体实现）
 * */
typedef struct {
    uint32_t (*pf_get_tick_ms)(void);   /* 获取系统时基（ms） */
    void     (*pf_delay_ms)(uint32_t);  /* 延时函数（ms）      */
} bsp_key_time_ops_t;

/**
 * @brief BSP 按键操作状态
 * */
typedef enum {
    KEY_OK          = 0,    /* 操作成功 */
    KEY_PRESSED     = 1,    /* 检测到有效按下 */
    KEY_NOT_PRESSED = 2,    /* 无按下事件 */
    KEY_ERROR       = 0xFF  /* 通用错误 */
} bsp_key_status_t;

//******************************* Defines ***********************************//
/* 消抖延时(ms) */
#define KEY_DEBOUNCE_MS  20

//******************************* Declaring *********************************//

/**
 * @brief 初始化按键模块（注入时间操作接口）
 * 
 * Steps:
 *  1. 校验函数指针非空
 *  2. 保存 APP 层传入的延时/时基接口
 *  3. 读取当前 GPIO 电平作为初始状态
 * 
 * @param[in] p_time_ops : 时间操作接口（不允许为 NULL）
 * 
 * @return bsp_key_status_t : 操作状态
 * 
 * */
bsp_key_status_t BSP_Key_Init(const bsp_key_time_ops_t *p_time_ops);

/**
 * @brief 扫描按键（带下降沿检测 + 软件消抖）
 * 
 * Steps:
 *  1. 检查是否已初始化
 *  2. 读取 PA0 当前电平（上拉输入，松开=1，按下=0）
 *  3. 检测下降沿：上次松开 → 本次按下
 *  4. 通过 pf_delay_ms 消抖后再次确认
 *  5. 更新内部状态并返回结果
 * 
 * @return bsp_key_status_t : KEY_PRESSED 或 KEY_NOT_PRESSED
 * 
 * */
bsp_key_status_t BSP_Key_Scan(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_KEY_H__ */
