/**
 * @file    bsp_led_handler.c
 * @brief   LED 实例注册表实现 — 多 LED 统一管理
 * @author  GS_Mark
 *
 * @par dependencies
 * - bsp_led_handler.h
 */

//*** 头文件 ***//
#include "bsp_led_handler.h"
#include <string.h>

//*** 静态实例 ***//

static led_instance_table_t s_led_table;

//*** 公共 API ***//

/**
 * @brief  初始化 LED 实例注册表（清零所有槽位）。
 * */
void BspLedHandler_Init(void)
{
    memset(&s_led_table, 0, sizeof(s_led_table));
}

/**
 * @brief  将 LED 驱动实例注册到注册表。
 *
 * @param[in] p_drv : LED 驱动实例指针。
 * */
void BspLedHandler_Register(bsp_led_driver_t *p_drv)
{
    uint32_t i;

    if (p_drv == NULL)
    {
        return;
    }

    /* 避免重复注册 */
    for (i = 0; i < s_led_table.count; i++)
    {
        if (s_led_table.instances[i] == p_drv)
        {
            return;
        }
    }

    if (s_led_table.count < BSP_LED_MAX_INSTANCES)
    {
        s_led_table.instances[s_led_table.count] = p_drv;
        s_led_table.count++;
    }
}

/**
 * @brief  从注册表注销 LED 驱动实例。
 *
 * @param[in] p_drv : LED 驱动实例指针。
 * */
void BspLedHandler_Unregister(bsp_led_driver_t *p_drv)
{
    uint32_t i;
    uint32_t j;

    if (p_drv == NULL)
    {
        return;
    }

    for (i = 0; i < s_led_table.count; i++)
    {
        if (s_led_table.instances[i] == p_drv)
        {
            /* 后面的实例前移 */
            for (j = i; j < s_led_table.count - 1; j++)
            {
                s_led_table.instances[j] = s_led_table.instances[j + 1];
            }
            s_led_table.count--;
            s_led_table.instances[s_led_table.count] = NULL;
            return;
        }
    }
}

/**
 * @brief  驱动所有已注册 LED 的闪烁时基。
 *
 *         需在主循环中周期调用（建议 10~50ms）。
 * */
void BspLedHandler_TimebaseHookAll(void)
{
    uint32_t i;

    for (i = 0; i < s_led_table.count; i++)
    {
        if (s_led_table.instances[i] != NULL)
        {
            BspLed_TimebaseHook(s_led_table.instances[i]);
        }
    }
}

/**
 * @brief  通过索引查找已注册的 LED 驱动实例。
 *
 * @param[in] index : 实例索引（0 ~ count-1）。
 *
 * @return  LED 驱动实例指针，索引越界返回 NULL。
 * */
bsp_led_driver_t *BspLedHandler_GetByIndex(uint32_t index)
{
    if (index >= s_led_table.count)
    {
        return NULL;
    }
    return s_led_table.instances[index];
}
