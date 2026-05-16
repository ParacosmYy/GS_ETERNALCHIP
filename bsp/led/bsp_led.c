/**
 * @file    bsp_led.c
 * @brief   LED BSP driver implementation — STM32 HAL backend
 */

//*** Includes ***//
#include "bsp_led.h"
#include <string.h>

//*** Private Functions — HAL Backend ***//

/**
 * @brief  HAL implementation: turn LED on.
 *         For low-active LED, writes RESET; for high-active, writes SET.
 */
static void HalLed_On(bsp_led_driver_t *p_drv)
{
    if (p_drv->p_config->active_level == 0) {
        HAL_GPIO_WritePin(p_drv->p_config->p_port, p_drv->p_config->pin, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(p_drv->p_config->p_port, p_drv->p_config->pin, GPIO_PIN_SET);
    }
}

/**
 * @brief  HAL implementation: turn LED off.
 *         Opposite polarity of On.
 */
static void HalLed_Off(bsp_led_driver_t *p_drv)
{
    if (p_drv->p_config->active_level == 0) {
        HAL_GPIO_WritePin(p_drv->p_config->p_port, p_drv->p_config->pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(p_drv->p_config->p_port, p_drv->p_config->pin, GPIO_PIN_RESET);
    }
}

/**
 * @brief  HAL implementation: toggle LED.
 */
static void HalLed_Toggle(bsp_led_driver_t *p_drv)
{
    HAL_GPIO_TogglePin(p_drv->p_config->p_port, p_drv->p_config->pin);
}

//*** Public Variables ***//

const led_operations_t bsp_led_hal_ops = {
    .p_On     = HalLed_On,
    .p_Off    = HalLed_Off,
    .p_Toggle = HalLed_Toggle,
};

//*** Public API ***//

void BspLed_Init(bsp_led_driver_t *p_drv, const bsp_led_config_t *p_config)
{
    memset(p_drv, 0, sizeof(*p_drv));
    p_drv->p_config = p_config;
    p_drv->p_ops    = &bsp_led_hal_ops;
}

void BspLed_On(bsp_led_driver_t *p_drv)
{
    p_drv->p_ops->p_On(p_drv);
}

void BspLed_Off(bsp_led_driver_t *p_drv)
{
    p_drv->p_ops->p_Off(p_drv);
}

void BspLed_Toggle(bsp_led_driver_t *p_drv)
{
    p_drv->p_ops->p_Toggle(p_drv);
}

void BspLed_BlinkStart(bsp_led_driver_t *p_drv, uint32_t interval_ms)
{
    p_drv->blink_interval_ms = interval_ms;
    p_drv->blink_last_tick   = HAL_GetTick();
    p_drv->is_blinking       = 1;
}

void BspLed_BlinkStop(bsp_led_driver_t *p_drv)
{
    p_drv->is_blinking       = 0;
    p_drv->blink_interval_ms = 0;
    BspLed_Off(p_drv);
}

void BspLed_TimebaseHook(bsp_led_driver_t *p_drv)
{
    if (!p_drv->is_blinking) {
        return;
    }

    uint32_t now = HAL_GetTick();
    if ((now - p_drv->blink_last_tick) >= p_drv->blink_interval_ms) {
        p_drv->blink_last_tick = now;
        BspLed_Toggle(p_drv);
    }
}
