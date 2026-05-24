/**
 * @file    bsp_key_driver.c
 * @brief   按键 BSP 驱动实现 — 消抖 FSM + 长按/短按检测
 * @author  GS_Mark
 *
 * @par dependencies
 * - bsp_key_driver.h
 */

//*** 头文件 ***//
#include "bsp_key_driver.h"
#include "system_adaption.h"
#include <string.h>

//*** 私有辅助函数 ***//

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

//*** 公共 API ***//

/**
 * @brief  初始化按键驱动实例。
 *
 * Steps:
 *  1. 将驱动结构体清零。
 *  2. 绑定硬件配置和 OPS 接口。
 *  3. 初始 stable_level 设为 0（未按下）。
 *
 * @param[out] p_drv    : 按键驱动实例指针。
 * @param[in]  p_config : 按键硬件配置（GPIO 端口、引脚、有效电平、消抖时间、长按时间、回调）。
 * @param[in]  p_hw_ops : 硬件 OPS（pin 读取）。
 * @param[in]  p_os_ops : OS OPS（tick 获取）。
 * */
void BspKey_Init(bsp_key_driver_t *p_drv, const bsp_key_config_t *p_config,
                 const key_hw_operations_t *p_hw_ops, const tick_operations_t *p_os_ops)
{
    memset(p_drv, 0, sizeof(*p_drv));
    p_drv->p_config = p_config;
    p_drv->p_hw_ops = p_hw_ops;
    p_drv->p_os_ops = p_os_ops;

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
    raw   = p_drv->p_hw_ops->pf_read_pin(p_cfg);
    now   = p_drv->p_os_ops->pf_get_tick();

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
