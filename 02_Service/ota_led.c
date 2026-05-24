/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    ota_led.c
 * @brief   OTA 升级 LED 状态指示模块实现
 * @author  GS_Mark
 *
 * @par dependencies
 * - ota_led.h
 * - bsp_led_driver.h (LED driver + platform interface)
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

//******************************** 头文件 ***********************************//
#define LOG_TAG "LED"
#include "elog.h"
#include "ota_led.h"
#include "bsp_led_driver.h"
#include <string.h>
//******************************** 头文件 ***********************************//

//******************************** 宏定义 ***********************************//
#define BLINK_SLOW_MS       500     /* 1Hz: 500ms 亮 / 500ms 灭 */
#define BLINK_FAST_MS       125     /* 4Hz: 125ms 亮 / 125ms 灭 */
#define BLINK_VERY_FAST_MS  50      /* 10Hz: 50ms 亮 / 50ms 灭 */
#define BLINK_ERROR_MS      150     /* 错误闪烁: 每次闪烁 150ms */
#define ERROR_PAUSE_MS      2000    /* 错误暂停: 周期间隔 2s */
#define TIMEBASE_PERIOD_MS  50      /* 时基钩子调用周期 */
//******************************** 宏定义 ***********************************//

//******************************** 类型 *************************************//
//******************************** 类型 *************************************//

//******************************** 变量 *************************************//
static bsp_led_driver_t s_led;
static ota_led_mode_t   s_mode;
static uint8_t          s_error_code;

/* 错误闪烁状态机 */
static uint8_t  s_err_blink_count;      /* 当前周期已完成闪烁次数 */
static uint8_t  s_err_phase;            /* 0 = 闪烁中, 1 = 暂停 */
static uint16_t s_err_tick;             /* 当前相位计数器 */
//******************************** 变量 *************************************//

//*** 前向声明 ***//
static const char *ErrorDesc(uint8_t code);
//*** 前向声明 ***//

//******************************** 实现 ************************************//

static const char *ErrorDesc(uint8_t code)
{
    switch (code)
    {
        case OTA_ERR_ERASE_FAILED:    return "Slot B 擦除失败";
        case OTA_ERR_YMODEM_FAILED:   return "YMODEM 接收失败";
        case OTA_ERR_ECDSA_FAILED:    return "ECDSA 签名验证失败";
        case OTA_ERR_SHA256_FAILED:   return "SHA-256 校验失败";
        case OTA_ERR_CONFIG_FAILED:   return "OTA Config 更新失败";
        default:                      return "未知错误";
    }
}

void OtaLed_Init(void)
{
    static const bsp_led_config_t led_cfg = { { (void *)GPIOC, 13, 0 } };

    BspLed_Init(&s_led, &led_cfg);
    s_mode        = OTA_LED_OFF;
    s_error_code  = 0;
    s_err_blink_count = 0;
    s_err_phase   = 0;
    s_err_tick    = 0;

    log_i("LED 模块已初始化。");
}

void OtaLed_SetMode(ota_led_mode_t mode)
{
    if (mode == s_mode)
    {
        return;
    }

    /* 停止当前闪烁 */
    BspLed_BlinkStop(&s_led);
    BspLed_Off(&s_led);

    s_mode = mode;

    switch (mode)
    {
        case OTA_LED_IDLE:
            log_i("LED 模式: IDLE（慢闪 1Hz）");
            BspLed_BlinkStart(&s_led, BLINK_SLOW_MS);
            break;

        case OTA_LED_WORKING:
            log_i("LED 模式: WORKING（快闪 4Hz）");
            BspLed_BlinkStart(&s_led, BLINK_FAST_MS);
            break;

        case OTA_LED_RECEIVING:
            log_i("LED 模式: RX（常亮）");
            BspLed_On(&s_led);
            break;

        case OTA_LED_REBOOTING:
            log_i("LED 模式: REBOOTING（极快闪 10Hz）");
            BspLed_BlinkStart(&s_led, BLINK_VERY_FAST_MS);
            break;

        case OTA_LED_ERROR:
            log_i("LED 模式: ERROR（闪烁 x%u，暂停 2s）", s_error_code);
            s_err_blink_count = 0;
            s_err_phase       = 0;
            s_err_tick        = 0;
            BspLed_On(&s_led);
            break;

        case OTA_LED_OFF:
            log_i("LED 模式: OFF");
            break;
    }
}

void OtaLed_SetErrorCode(uint8_t code)
{
    s_error_code = code;
    log_e("错误码 %u: %s", code, ErrorDesc(code));
    OtaLed_SetMode(OTA_LED_ERROR);
}

uint8_t OtaLed_GetErrorCode(void)
{
    return s_error_code;
}

void OtaLed_TimebaseHook(void)
{
    /* 非错误模式：委托 BSP LED 闪烁引擎 */
    if (s_mode != OTA_LED_ERROR)
    {
        BspLed_TimebaseHook(&s_led);
        return;
    }

    /* 错误模式：自定义闪烁模式（N 次闪烁 + 2s 暂停） */
    s_err_tick += TIMEBASE_PERIOD_MS;

    if (s_err_phase == 0)
    {
        /* 闪烁阶段 */
        if (s_err_tick >= BLINK_ERROR_MS)
        {
            s_err_tick = 0;
            BspLed_Toggle(&s_led);
            s_err_blink_count++;

            /* 每次闪烁 = 1 次开 + 1 次关 = 2 次翻转 */
            if (s_err_blink_count >= (s_error_code * 2))
            {
                /* 所有闪烁完成，进入暂停 */
                BspLed_Off(&s_led);
                s_err_phase = 1;
                s_err_tick  = 0;
            }
        }
    }
    else
    {
        /* 暂停阶段 */
        if (s_err_tick >= ERROR_PAUSE_MS)
        {
            /* 重新开始闪烁周期 */
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
//******************************** 实现 ************************************//
