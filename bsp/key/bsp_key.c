/**
 * @file    bsp_key.c
 * @brief   Key BSP driver implementation — debounce FSM + long/short press
 */

//*** Includes ***//
#include "bsp_key.h"
#include <string.h>

//*** Private Helpers ***//

/**
 * @brief  Read raw pin level and convert to logical level (0/1).
 *         active_level=0 (low-active): pin RESET → logical 1 (pressed)
 *         active_level=1 (high-active): pin SET   → logical 1 (pressed)
 */
static uint8_t Key_ReadRaw(const bsp_key_config_t *p_cfg)
{
    GPIO_PinState pin;

    pin = HAL_GPIO_ReadPin(p_cfg->p_port, p_cfg->pin);

    if (p_cfg->active_level == 0)
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
 * @brief  Invoke the user callback if configured.
 */
static void Key_Notify(const bsp_key_driver_t *p_drv, bsp_key_event_t evt)
{
    if (p_drv->p_config->callback != NULL)
    {
        p_drv->p_config->callback(evt, p_drv->p_config->p_user_data);
    }
}

//*** Public API ***//

void BspKey_Init(bsp_key_driver_t *p_drv, const bsp_key_config_t *p_config)
{
    memset(p_drv, 0, sizeof(*p_drv));
    p_drv->p_config = p_config;

    /* Seed stable_level to the current (inactive) state so first scan works */
    p_drv->stable_level = 0;
}

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
        /*--- Waiting for any level change ---*/
        case BSP_KEY_STATE_IDLE:
            if (raw != p_drv->stable_level)
            {
                p_drv->state     = BSP_KEY_STATE_DEBOUNCING;
                p_drv->last_tick = now;
            }
            break;

        /*--- Debounce timer running ---*/
        case BSP_KEY_STATE_DEBOUNCING:
            if (raw != p_drv->stable_level)
            {
                /* Level still different — wait for debounce window */
                if ((now - p_drv->last_tick) >= p_cfg->debounce_ms)
                {
                    /* Debounce confirmed — accept new level */
                    p_drv->stable_level = raw;

                    if (raw == 1)
                    {
                        /* Key pressed */
                        p_drv->press_tick       = now;
                        p_drv->long_press_fired = 0;
                        Key_Notify(p_drv, BSP_KEY_EVT_PRESSED);
                    }
                    else
                    {
                        /* Key released */
                        Key_Notify(p_drv, BSP_KEY_EVT_RELEASED);

                        /* Short press: release detected and no long press fired */
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
                /* Level bounced back — return to previous logical state */
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

        /*--- Stable state — track long press while held ---*/
        case BSP_KEY_STATE_STABLE:
            if (raw != p_drv->stable_level)
            {
                /* Level changed — go to debounce for the new edge */
                p_drv->state     = BSP_KEY_STATE_DEBOUNCING;
                p_drv->last_tick = now;
            }
            else if (raw == 1 && !p_drv->long_press_fired)
            {
                /* Still held — check long press timeout */
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
