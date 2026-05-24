/**
 * @file    bsp_key.c
 * @brief   Key BSP driver implementation — debounce FSM + long/short press
 * @author  GS_Mark
 *
 * @par dependencies
 * - bsp_key.h
 */

//*** Includes ***//
#include "bsp_key.h"
#include <string.h>

//*** HAL Cast Helper ***//
#define HAL_GPIO_PORT(gpio)  ((GPIO_TypeDef *)((gpio).port))

//*** Private Helpers ***//

/**
 * @brief  读取引脚原始电平并转换为逻辑电平（0/1）。
 *         active_level=0（低有效）: pin RESET → logical 1（按下）。
 *         active_level=1（高有效）: pin SET   → logical 1（按下）。
 *
 * @param[in] p_cfg : 按键硬件配置指针。
 *
 * @return  1 : 按下。
 * @return  0 : 释放。
 * */
static uint8_t Key_ReadRaw(const bsp_key_config_t *p_cfg)
{
    GPIO_PinState pin;

    pin = HAL_GPIO_ReadPin(HAL_GPIO_PORT(p_cfg->gpio), p_cfg->gpio.pin);

    if (p_cfg->gpio.active_level == 0)
    {
        if (pin == GPIO_PIN_RESET)
        {
            return 1;
        }
        return 0;
    }

    if (pin == GPIO_PIN_SET)
    {
        return 1;
    }
    return 0;
}

/**
 * @brief  调用用户注册的回调函数。
 *
 * @param[in] p_drv : 按键驱动实例指针。
 * @param[in] evt   : 按键事件类型。
 * */
static void Key_Notify(const bsp_key_driver_t *p_drv, bsp_key_event_t evt)
{
    if (p_drv->p_config->callback != NULL)
    {
        p_drv->p_config->callback(evt, p_drv->p_config->p_user_data);
    }
}

//*** Public API ***//

/**
 * @brief  初始化按键驱动实例。
 *
 * Steps:
 *  1. 将驱动结构体清零。
 *  2. 绑定硬件配置。
 *  3. 初始 stable_level 设为 0（未按下）。
 *
 * @param[out] p_drv    : 按键驱动实例指针。
 * @param[in]  p_config : 按键硬件配置（GPIO 端口、引脚、有效电平、消抖时间、长按时间、回调）。
 * */
void BspKey_Init(bsp_key_driver_t *p_drv, const bsp_key_config_t *p_config)
{
    memset(p_drv, 0, sizeof(*p_drv));
    p_drv->p_config = p_config;

    /* 初始化为当前未按下状态，首次扫描时正常检测 */
    p_drv->stable_level = 0;
}

/**
 * @brief  按键扫描处理（需在主循环中周期调用）。
 *
 *         内部实现三态 FSM：IDLE -> DEBOUNCING -> STABLE
 *         - IDLE       ：等待电平变化。
 *         - DEBOUNCING ：消抖计时，确认电平变化有效后触发 PRESSED / RELEASED 事件。
 *         - STABLE     ：稳定状态，持续检测长按超时。
 *
 * @note   消抖和长按判定均依赖此函数的调用频率，建议每 1~10 ms 调用一次。
 *
 * @param[in] p_drv : 按键驱动实例指针。
 * */
void BspKey_Scan(bsp_key_driver_t *p_drv)
{
    const bsp_key_config_t *p_cfg;
    uint8_t                 raw;
    uint32_t                now;

    p_cfg = p_drv->p_config;
    raw   = Key_ReadRaw(p_cfg);
    now   = HAL_GetTick();

    switch (p_drv->state)
    {
        /*--- 等待任意电平变化 ---*/
        case BSP_KEY_STATE_IDLE:
            if (raw != p_drv->stable_level)
            {
                p_drv->state     = BSP_KEY_STATE_DEBOUNCING;
                p_drv->last_tick = now;
            }
            break;

        /*--- 消抖计时中 ---*/
        case BSP_KEY_STATE_DEBOUNCING:
            if (raw != p_drv->stable_level)
            {
                /* 电平持续不同 — 等待消抖窗口结束 */
                if ((now - p_drv->last_tick) >= p_cfg->debounce_ms)
                {
                    /* 消抖确认 — 接受新电平 */
                    p_drv->stable_level = raw;

                    if (raw == 1)
                    {
                        /* 按下事件 */
                        p_drv->press_tick       = now;
                        p_drv->long_press_fired = 0;
                        Key_Notify(p_drv, BSP_KEY_EVT_PRESSED);
                    }
                    else
                    {
                        /* 释放事件 + 短按判定（长按未触发时） */
                        Key_Notify(p_drv, BSP_KEY_EVT_RELEASED);

                        if (!p_drv->long_press_fired)
                        {
                            Key_Notify(p_drv, BSP_KEY_EVT_SHORT_PRESS);
                        }
                    }
                    p_drv->state = BSP_KEY_STATE_STABLE;
                }
            }
            else
            {
                /* 电平回弹 — 恢复到上一逻辑状态 */
                if (p_drv->stable_level == 1)
                {
                    p_drv->state = BSP_KEY_STATE_STABLE;
                }
                else
                {
                    p_drv->state = BSP_KEY_STATE_IDLE;
                }
            }
            break;

        /*--- 稳定状态 — 持续检测长按 ---*/
        case BSP_KEY_STATE_STABLE:
            if (raw != p_drv->stable_level)
            {
                /* 电平变化 — 进入消抖 */
                p_drv->state     = BSP_KEY_STATE_DEBOUNCING;
                p_drv->last_tick = now;
            }
            else if (raw == 1 && !p_drv->long_press_fired)
            {
                /* 持续按下 — 检测长按超时 */
                if ((now - p_drv->press_tick) >= p_cfg->long_press_ms)
                {
                    p_drv->long_press_fired = 1;
                    Key_Notify(p_drv, BSP_KEY_EVT_LONG_PRESS);
                }
            }
            break;

        default:
            p_drv->state = BSP_KEY_STATE_IDLE;
            break;
    }
}
