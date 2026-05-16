/**
 * @file    task_test_led_key.c
 * @brief   Test module — KEY short/long press → LED toggle/blink
 *          + UART debug prints for event tracing
 *
 * Behaviour:
 *   SHORT_PRESS → LED toggle
 *   LONG_PRESS  → start / stop 500 ms blink
 */

//*** Includes ***//
#include "task_test_led_key.h"
#include "bsp_led.h"
#include "bsp_key.h"
#include "bsp_uart.h"
#include "usart.h"

//*** Private Variables ***//

static bsp_led_driver_t s_led;
static bsp_key_driver_t s_key;
static bsp_uart_driver_t s_uart;
static uint8_t           s_uart_rx_buf[64];
static uint8_t           s_blink_active;

//*** Private Functions ***//

static void OnKey(bsp_key_event_t evt, void *p_user)
{
    (void)p_user;

    switch (evt) {
    case BSP_KEY_EVT_PRESSED:
        BspUart_Printf(&s_uart, "[KEY] PRESSED\r\n");
        break;

    case BSP_KEY_EVT_RELEASED:
        BspUart_Printf(&s_uart, "[KEY] RELEASED\r\n");
        break;

    case BSP_KEY_EVT_SHORT_PRESS:
        BspUart_Printf(&s_uart, "[KEY] SHORT_PRESS\r\n");
        BspLed_BlinkStop(&s_led);
        s_blink_active = 0;
        BspLed_Toggle(&s_led);
        break;

    case BSP_KEY_EVT_LONG_PRESS:
        BspUart_Printf(&s_uart, "[KEY] LONG_PRESS\r\n");
        if (!s_blink_active) {
            BspUart_Printf(&s_uart, "[KEY] -> blink start\r\n");
            BspLed_On(&s_led);
            BspLed_BlinkStart(&s_led, 500);
            s_blink_active = 1;
        } else {
            BspUart_Printf(&s_uart, "[KEY] -> blink stop\r\n");
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

    static const bsp_uart_config_t uart_cfg = {
        &huart1, s_uart_rx_buf, sizeof(s_uart_rx_buf), NULL, NULL
    };
    BspUart_Init(&s_uart, &uart_cfg);
    BspUart_StartReceive(&s_uart);

    s_blink_active = 0;

    BspUart_Printf(&s_uart, "--- LED/KEY Test Ready ---\r\n");
}

void TaskTestLedKey_Process(void)
{
    BspKey_Scan(&s_key);
    BspLed_TimebaseHook(&s_led);
}
