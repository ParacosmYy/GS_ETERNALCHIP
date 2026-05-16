/**
 * @file    task_test_led_key.c
 * @brief   Test module — KEY short/long press → LED toggle/blink
 *
 * Behaviour:
 *   SHORT_PRESS → LED toggle
 *   LONG_PRESS  → start / stop 500 ms blink
 */

//*** Includes ***//
#include "task_test_led_key.h"
#include "bsp_led.h"
#include "bsp_key.h"

//*** Private Variables ***//

static bsp_led_driver_t s_led;
static bsp_key_driver_t s_key;
static uint8_t          s_blink_active;

//*** Private Functions ***//

static void OnKey(bsp_key_event_t evt, void *p_user)
{
    (void)p_user;

    if (evt == BSP_KEY_EVT_SHORT_PRESS) {
        BspLed_BlinkStop(&s_led);
        s_blink_active = 0;
        BspLed_Toggle(&s_led);
    } else if (evt == BSP_KEY_EVT_LONG_PRESS) {
        if (!s_blink_active) {
            BspLed_BlinkStart(&s_led, 500);
            s_blink_active = 1;
        } else {
            BspLed_BlinkStop(&s_led);
            s_blink_active = 0;
        }
    }
}

//*** Public API ***//

void TaskTestLedKey_Init(void)
{
    static const bsp_led_config_t led_cfg = { GPIOC, GPIO_PIN_13, 0 };
    BspLed_Init(&s_led, &led_cfg);

    static const bsp_key_config_t key_cfg = {
        GPIOA, GPIO_PIN_0, 0, 20, 1000, OnKey, NULL
    };
    BspKey_Init(&s_key, &key_cfg);

    s_blink_active = 0;
}

void TaskTestLedKey_Process(void)
{
    BspKey_Scan(&s_key);
    BspLed_TimebaseHook(&s_led);
}
