/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 * 
 * @file bsp_key.c
 * 
 * @par dependencies 
 * - bsp_key.h
 * 
 * @author GS_Mark
 *
 * @brief 按键硬件抽象层实现，基于 HAL GPIO 读取 PA0
 * 
 * Processing flow:
 * 轮询读取 PA0 电平 → 下降沿检测 → 软件消抖 → 返回按键事件
 * 
 * @version V1.0 2025-04-26
 *
 * @note 1 tab == 4 spaces
 * 
 *****************************************************************************/

//******************************** Includes *********************************//
#include "bsp_key.h"
#include "cmsis_os.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
#define KEY_PORT    GPIOA
#define KEY_PIN     GPIO_PIN_0

//******************************** Implement ********************************//

/**
 * @brief 扫描按键（带下降沿检测 + 软件消抖）
 * 
 * Steps:
 *  1. 读取 PA0 当前电平（上拉输入，松开=1，按下=0）
 *  2. 检测下降沿：上次为松开(1)且本次为按下(0)
 *  3. 延时 KEY_DEBOUNCE_MS 后再次确认仍为低电平
 *  4. 更新 p_last_key 并返回结果
 * 
 * @param[in] p_last_key : 指向上次按键状态的指针（由调用方维护）
 * 
 * @return bsp_key_state_t : KEY_PRESSED 表示检测到有效按下
 * 
 * */
bsp_key_state_t BSP_Key_Scan(uint8_t * const p_last_key)
{
    uint8_t cur_key = HAL_GPIO_ReadPin(KEY_PORT, KEY_PIN);

    /* 下降沿检测：上次松开 → 本次按下 */
    if (*p_last_key == 1 && cur_key == 0)
    {
        osDelay(KEY_DEBOUNCE_MS);
        if (HAL_GPIO_ReadPin(KEY_PORT, KEY_PIN) == 0)
        {
            *p_last_key = 0;
            return KEY_PRESSED;
        }
    }

    *p_last_key = cur_key;
    return KEY_RELEASED;
}
