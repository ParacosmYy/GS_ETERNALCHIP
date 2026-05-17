/**
 * @file    task_test_led_key.c
 * @brief   Test module — KEY short/long press → LED toggle/blink
 *
 * Behaviour:
 *   SHORT_PRESS → LED toggle
 *   LONG_PRESS  → start / stop 500 ms blink
 */

//*** Includes ***//
#define LOG_TAG "KEY"
#include "elog.h"
#include "stm32f4xx_hal.h"
#include "main.h"
#include "task_test_led_key.h"
#include "bsp_led.h"
#include "bsp_key.h"

//*** Private Variables ***//

static bsp_led_driver_t s_led;
static bsp_key_driver_t s_key;
static uint8_t          s_blink_active;

//*** Private Functions ***//

/**
 * @brief  按键事件回调函数
 *
 *         根据按键事件类型执行不同操作：
 *         - SHORT_PRESS：若 LED 正在闪烁则停止，然后翻转 LED
 *         - LONG_PRESS ：切换 LED 500ms 闪烁模式（开/关）
 *
 * @param  evt     按键事件类型
 * @param  p_user  用户数据（未使用）
 */
static void OnKey(bsp_key_event_t evt, void *p_user)
{
    (void)p_user;

    switch (evt)
    {
        case BSP_KEY_EVT_PRESSED:
            log_i("PRESSED");
            break;

        case BSP_KEY_EVT_RELEASED:
            log_i("RELEASED");
            break;

        case BSP_KEY_EVT_SHORT_PRESS:
            log_i("SHORT_PRESS");

            if (s_blink_active)
            {
                BspLed_BlinkStop(&s_led);
                s_blink_active = 0;
            }

            BspLed_Toggle(&s_led);
            break;

        case BSP_KEY_EVT_LONG_PRESS:
            log_i("LONG_PRESS");

            if (!s_blink_active)
            {
                log_i("blink start");
                BspLed_On(&s_led);
                BspLed_BlinkStart(&s_led, 500);
                s_blink_active = 1;
            }
            else
            {
                log_i("blink stop");
                BspLed_BlinkStop(&s_led);
                s_blink_active = 0;
            }
            break;

        default:
            break;
    }
}

//*** Public API ***//

/**
 * @brief  初始化 LED 和按键测试任务
 *
 *         配置 LED（PC13，低有效）和按键（PA0，低有效，20ms 消抖，1000ms 长按），
 *         注册按键事件回调
 */
void TaskTestLedKey_Init(void)
{
    static const bsp_led_config_t led_cfg = { GPIOC, GPIO_PIN_13, 0 };
    static const bsp_key_config_t key_cfg = { GPIOA, GPIO_PIN_0, 0, 20, 1000, OnKey, NULL };

    BspLed_Init(&s_led, &led_cfg);
    BspKey_Init(&s_key, &key_cfg);

    s_blink_active = 0;

    log_i("--- LED/KEY Test Ready ---");
}

/**
 * @brief  测试任务主处理函数，需在主循环中周期调用
 *
 *         轮询按键扫描并驱动 LED 闪烁时基
 */
void TaskTestLedKey_Process(void)
{
    BspKey_Scan(&s_key);
    BspLed_TimebaseHook(&s_led);
}
