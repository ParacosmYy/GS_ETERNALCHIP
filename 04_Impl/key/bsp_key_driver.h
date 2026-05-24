/**
 * @file    bsp_key_driver.h
 * @brief   按键 BSP 驱动 — 消抖状态机 + 长按/短按检测
 * @author  GS_Mark
 *
 * @note    类型和 API 声明位于 plat_key.h（03_Platform/interface）。
 *          本头文件仅供 Impl 层消费者使用，暴露 FSM 状态。
 */

#ifndef BSP_KEY_DRIVER_H
#define BSP_KEY_DRIVER_H

#ifdef __cplusplus
extern "C"
{
#endif

//*** 头文件 ***//
#include "stm32f4xx_hal.h"
#include "plat_key.h"

    //*** FSM 状态（仅 Impl 层） ***//

    enum
    {
        BSP_KEY_STATE_IDLE       = 0, /**< 等待电平变化 */
        BSP_KEY_STATE_DEBOUNCING = 1, /**< 消抖计时中 */
        BSP_KEY_STATE_STABLE     = 2, /**< 稳定状态，持续检测按下 */
    };

#ifdef __cplusplus
}
#endif

#endif /* BSP_KEY_DRIVER_H */
