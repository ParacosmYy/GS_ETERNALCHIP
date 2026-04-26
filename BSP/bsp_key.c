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
 * BSP_Key_Init 注入 ops → BSP_Key_Scan 轮询读取 PA0 → 下降沿检测 → 消抖 → 返回
 * 
 * @version V2.0 2025-04-26
 *
 * @note 1 tab == 4 spaces
 * 
 *****************************************************************************/

//******************************** Includes *********************************//
#include "bsp_key.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
#define KEY_PORT    GPIOA
#define KEY_PIN     GPIO_PIN_0

//******************************** Variables ********************************//
static bsp_key_time_ops_t const *gp_time_ops = NULL;
static uint8_t g_last_key = 1;   /* 上拉输入，松开为1 */
//******************************** Variables ********************************//

//******************************** Implement ********************************//

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
bsp_key_status_t BSP_Key_Init(const bsp_key_time_ops_t *p_time_ops)
{
    if (p_time_ops == NULL
        || p_time_ops->pf_get_tick_ms == NULL
        || p_time_ops->pf_delay_ms == NULL)
    {
        return KEY_ERROR;
    }

    gp_time_ops = p_time_ops;
    g_last_key  = HAL_GPIO_ReadPin(KEY_PORT, KEY_PIN);

    return KEY_OK;
}

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
bsp_key_status_t BSP_Key_Scan(void)
{
    if (gp_time_ops == NULL)
    {
        return KEY_ERROR;
    }

    uint8_t cur_key = HAL_GPIO_ReadPin(KEY_PORT, KEY_PIN);

    /* 下降沿检测：上次松开 → 本次按下 */
    if (g_last_key == 1 && cur_key == 0)
    {
        gp_time_ops->pf_delay_ms(KEY_DEBOUNCE_MS);
        if (HAL_GPIO_ReadPin(KEY_PORT, KEY_PIN) == 0)
        {
            g_last_key = 0;
            return KEY_PRESSED;
        }
    }

    g_last_key = cur_key;
    return KEY_NOT_PRESSED;
}
