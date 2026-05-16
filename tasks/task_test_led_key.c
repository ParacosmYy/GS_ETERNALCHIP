/**
 * @file    task_test_led_key.c
 * @brief   Test module — KEY short/long press → LED toggle/blink
 *
 * Behaviour:
 *   SHORT_PRESS → LED toggle
 *   LONG_PRESS  → start / stop 500 ms blink
 */

//*** Includes ***//
#include "stm32f4xx_hal.h"
#include "main.h"
#include "task_test_led_key.h"
#include "bsp_led.h"
#include "bsp_key.h"

//*** Debug Switch ***//
#define TASK_LED_KEY_DEBUG  1
#define TASK_LED_KEY_DEBUG_USE_RTT  1

#if TASK_LED_KEY_DEBUG
#if TASK_LED_KEY_DEBUG_USE_RTT
#include "bsp_rtt.h"
#define DBG_PRINT(fmt, ...)  BspRtt_Printf(fmt, ##__VA_ARGS__)
#else
#include "bsp_uart.h"
#include "usart.h"
#define DBG_PRINT(fmt, ...)  BspUart_Printf(&s_uart, fmt, ##__VA_ARGS__)
#endif
#else
#define DBG_PRINT(fmt, ...)  ((void)0)
#endif

//*** Private Variables ***//

static bsp_led_driver_t s_led;
static bsp_key_driver_t s_key;
static uint8_t          s_blink_active;

#if TASK_LED_KEY_DEBUG
#if !TASK_LED_KEY_DEBUG_USE_RTT
static bsp_uart_driver_t s_uart;
static uint8_t           s_uart_rx_buf[64];
#endif
#endif

//*** Private Functions ***//

static void OnKey(bsp_key_event_t evt, void *p_user)
{
    (void)p_user;

    switch (evt) {
    case BSP_KEY_EVT_PRESSED:
        DBG_PRINT("[KEY] PRESSED\r\n");
        break;

    case BSP_KEY_EVT_RELEASED:
        DBG_PRINT("[KEY] RELEASED\r\n");
        break;

    case BSP_KEY_EVT_SHORT_PRESS:
        DBG_PRINT("[KEY] SHORT_PRESS\r\n");
        if (s_blink_active) {
            BspLed_BlinkStop(&s_led);
            s_blink_active = 0;
        }
        BspLed_Toggle(&s_led);
        break;

    case BSP_KEY_EVT_LONG_PRESS:
        DBG_PRINT("[KEY] LONG_PRESS\r\n");
        if (!s_blink_active) {
            DBG_PRINT("[KEY] -> blink start\r\n");
            BspLed_On(&s_led);
            BspLed_BlinkStart(&s_led, 500);
            s_blink_active = 1;
        } else {
            DBG_PRINT("[KEY] -> blink stop\r\n");
            BspLed_BlinkStop(&s_led);
            s_blink_active = 0;
        }
        break;

    default:
        break;
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

#if TASK_LED_KEY_DEBUG
#if TASK_LED_KEY_DEBUG_USE_RTT
    BspRtt_Init();
    DBG_PRINT("--- LED/KEY Test Ready ---\r\n");
#else
    static const bsp_uart_config_t uart_cfg = {
        &huart1, s_uart_rx_buf, sizeof(s_uart_rx_buf), NULL, NULL
    };
    BspUart_Init(&s_uart, &uart_cfg);
    BspUart_StartReceive(&s_uart);
    DBG_PRINT("--- LED/KEY Test Ready ---\r\n");
#endif
#endif
}

void TaskTestLedKey_Process(void)
{
    BspKey_Scan(&s_key);
    BspLed_TimebaseHook(&s_led);
}
