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
 * BSP_Key_Scan()  - 扫描按键，返回 1 表示检测到有效按下事件
 * 
 * @version V1.0 2025-04-26
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

//******************************* Defines ***********************************//
/* 按键状态枚举 */
typedef enum {
    KEY_PRESSED  = 1,
    KEY_RELEASED = 0
} bsp_key_state_t;

/* 消抖延时(ms) */
#define KEY_DEBOUNCE_MS  20

//******************************* Declaring *********************************//

/**
 * @brief 扫描按键（带下降沿检测 + 软件消抖）
 * 
 * Steps:
 *  1. 读取 PA0 当前电平
 *  2. 检测下降沿（上次松开 → 本次按下）
 *  3. 延时消抖后再次确认
 *  4. 更新上次状态并返回结果
 * 
 * @param[in] p_last_key : 指向上次按键状态的指针（由调用方维护）
 * 
 * @return bsp_key_state_t : KEY_PRESSED 表示检测到有效按下
 * 
 * */
bsp_key_state_t BSP_Key_Scan(uint8_t * const p_last_key);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_KEY_H__ */
