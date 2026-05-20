/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    ota_led.c
 * @brief   OTA 升级 LED 状态指示模块实现
 * @author  GS_Mark
 *
 * @par dependencies
 * - ota_led.h
 * - bsp_led.h
 * - elog.h
 *
 * 处理流程：
 *  OtaLed_SetMode() 切换闪烁模式。
 *  OtaLed_TimebaseHook() 驱动闪烁时序（每 50ms 调用一次）。
 *  错误模式：连闪 N 次，暂停 2s，循环。
 *
 * @version V1.0 2025-05-20
 *
 * @note 1 tab == 4 spaces
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#define LOG_TAG "LED"
#include "elog.h"
#include "ota_led.h"
#include "bsp_led.h"
#include <string.h>
//******************************** Includes *********************************//

//******************************** Defines **********************************//
#define BLINK_SLOW_MS       500     /* 1Hz: 500ms on / 500ms off */
#define BLINK_FAST_MS       125     /* 4Hz: 125ms on / 125ms off */
#define BLINK_VERY_FAST_MS  50      /* 10Hz: 50ms on / 50ms off */
#define BLINK_ERROR_MS      150     /* Error blink: 150ms per blink */
#define ERROR_PAUSE_MS      2000    /* Error pause: 2s between cycles */
#define TIMEBASE_PERIOD_MS  50      /* Timebase hook call period */
//******************************** Defines **********************************//

//******************************** Types ************************************//
//******************************** Types ************************************//

//******************************** Variables ********************************//
static bsp_led_driver_t s_led;
static ota_led_mode_t   s_mode;
static uint8_t          s_error_code;

/* Error blink state machine */
static uint8_t  s_err_blink_count;      /* Blinks completed in current cycle */
static uint8_t  s_err_phase;            /* 0 = blinking, 1 = pause */
static uint16_t s_err_tick;             /* Tick counter for current phase */
//******************************** Variables ********************************//

//*** 前向声明 ***//
static const char *ErrorDesc(uint8_t code);
//*** 前向声明 ***//

//******************************** Implement ********************************//

static const char *ErrorDesc(uint8_t code)
{
    switch (code)
    {
        case OTA_ERR_ERASE_FAILED:    return "Slot B erase failed";
        case OTA_ERR_YMODEM_FAILED:   return "YMODEM receive failed";
        case OTA_ERR_ECDSA_FAILED:    return "ECDSA signature verify failed";
        case OTA_ERR_SHA256_FAILED:   return "SHA-256 verify failed";
        case OTA_ERR_CONFIG_FAILED:   return "OTA Config update failed";
        default:                      return "Unknown error";
    }
}

void OtaLed_Init(void)
{
    static const bsp_led_config_t led_cfg = { GPIOC, GPIO_PIN_13, 0 };

    BspLed_Init(&s_led, &led_cfg);
    s_mode        = OTA_LED_OFF;
    s_error_code  = 0;
    s_err_blink_count = 0;
    s_err_phase   = 0;
    s_err_tick    = 0;

    log_i("LED module initialized.");
}

void OtaLed_SetMode(ota_led_mode_t mode)
{
    if (mode == s_mode)
    {
        return;
    }

    /* Stop any ongoing blink */
    BspLed_BlinkStop(&s_led);
    BspLed_Off(&s_led);

    s_mode = mode;

    switch (mode)
    {
        case OTA_LED_IDLE:
            log_i("LED mode: IDLE (slow blink 1Hz)");
            BspLed_BlinkStart(&s_led, BLINK_SLOW_MS);
            break;

        case OTA_LED_WORKING:
            log_i("LED mode: WORKING (fast blink 4Hz)");
            BspLed_BlinkStart(&s_led, BLINK_FAST_MS);
            break;

        case OTA_LED_RECEIVING:
            log_i("LED mode: RECEIVING (solid on)");
            BspLed_On(&s_led);
            break;

        case OTA_LED_REBOOTING:
            log_i("LED mode: REBOOTING (very fast blink 10Hz)");
            BspLed_BlinkStart(&s_led, BLINK_VERY_FAST_MS);
            break;

        case OTA_LED_ERROR:
            log_i("LED mode: ERROR (blink x%u, pause 2s)", s_error_code);
            s_err_blink_count = 0;
            s_err_phase       = 0;
            s_err_tick        = 0;
            BspLed_On(&s_led);
            break;

        case OTA_LED_OFF:
            log_i("LED mode: OFF");
            break;
    }
}

void OtaLed_SetErrorCode(uint8_t code)
{
    s_error_code = code;
    log_e("Error code %u: %s", code, ErrorDesc(code));
    OtaLed_SetMode(OTA_LED_ERROR);
}

uint8_t OtaLed_GetErrorCode(void)
{
    return s_error_code;
}

void OtaLed_TimebaseHook(void)
{
    /* Non-error modes: delegate to BSP LED blink engine */
    if (s_mode != OTA_LED_ERROR)
    {
        BspLed_TimebaseHook(&s_led);
        return;
    }

    /* Error mode: custom blink pattern (N blinks + 2s pause) */
    s_err_tick += TIMEBASE_PERIOD_MS;

    if (s_err_phase == 0)
    {
        /* Blinking phase */
        if (s_err_tick >= BLINK_ERROR_MS)
        {
            s_err_tick = 0;
            BspLed_Toggle(&s_led);
            s_err_blink_count++;

            /* Each blink = 1 toggle on + 1 toggle off = 2 toggles */
            if (s_err_blink_count >= (s_error_code * 2))
            {
                /* All blinks done, enter pause */
                BspLed_Off(&s_led);
                s_err_phase = 1;
                s_err_tick  = 0;
            }
        }
    }
    else
    {
        /* Pause phase */
        if (s_err_tick >= ERROR_PAUSE_MS)
        {
            /* Restart blink cycle */
            s_err_blink_count = 0;
            s_err_phase       = 0;
            s_err_tick        = 0;
            BspLed_On(&s_led);
        }
    }
}

void OtaLed_Deinit(void)
{
    BspLed_BlinkStop(&s_led);
    BspLed_Off(&s_led);
    s_mode = OTA_LED_OFF;
}
//******************************** Implement ********************************//
