/**
 * @file    bsp_led_driver.c
 * @brief   LED BSP 驱动实现 — STM32 HAL 后端
 * @author  GS_Mark
 *
 * @par dependencies
 * - bsp_led_driver.h
 *
 * Steps:
 *  1. HAL 转换宏将 plat_gpio_t.void* port 转换为 GPIO_TypeDef*。
 *  2. 静态函数通过 HAL GPIO API 实现 led_operations_t。
 *  3. 公共 API 委托到 ops 函数指针。
 */

//*** 头文件 ***//
#include "bsp_led_driver.h"
#include "system_adaption.h"
#include <string.h>

//*** 公共 API ***//

/**
 * @brief  初始化 LED 驱动实例。
 *
 * Steps:
 *  1. 将驱动结构体清零。
 *  2. 绑定硬件配置和 HAL 操作函数表。
 *
 * @param[out] p_drv    : LED 驱动实例指针。
 * @param[in]  p_config : LED 硬件配置（plat_gpio_t 格式）。
 * @param[in]  p_os_ops : OS 操作函数指针（时基服务）。
 * */
void BspLed_Init(bsp_led_driver_t *p_drv, const bsp_led_config_t *p_config,
                 const led_os_operations_t *p_os_ops)
{
    memset(p_drv, 0, sizeof(*p_drv));
    p_drv->p_config = p_config;
    p_drv->p_ops    = &bsp_led_hal_ops;
    p_drv->p_os_ops = p_os_ops;
}

/**
 * @brief  点亮 LED。
 *
 * @param[in] p_drv : LED 驱动实例指针。
 * */
void BspLed_On(bsp_led_driver_t *p_drv)
{
    p_drv->p_ops->p_On(p_drv);
}

/**
 * @brief  熄灭 LED。
 *
 * @param[in] p_drv : LED 驱动实例指针。
 * */
void BspLed_Off(bsp_led_driver_t *p_drv)
{
    p_drv->p_ops->p_Off(p_drv);
}

/**
 * @brief  翻转 LED 状态。
 *
 * @param[in] p_drv : LED 驱动实例指针。
 * */
void BspLed_Toggle(bsp_led_driver_t *p_drv)
{
    p_drv->p_ops->p_Toggle(p_drv);
}

/**
 * @brief  启动 LED 闪烁。
 *
 *         设置闪烁间隔并记录起始时刻，需在主循环中周期调用
 *         BspLed_TimebaseHook() 来驱动翻转。
 *
 * @param[in] p_drv        : LED 驱动实例指针。
 * @param[in] interval_ms  : 闪烁间隔，单位 ms。
 * */
void BspLed_BlinkStart(bsp_led_driver_t *p_drv, uint32_t interval_ms)
{
    p_drv->blink_interval_ms = interval_ms;
    p_drv->blink_last_tick   = p_drv->p_os_ops->pf_get_tick();
    p_drv->is_blinking       = 1;
}

/**
 * @brief  停止 LED 闪烁并熄灭。
 *
 * @param[in] p_drv : LED 驱动实例指针。
 * */
void BspLed_BlinkStop(bsp_led_driver_t *p_drv)
{
    p_drv->is_blinking       = 0;
    p_drv->blink_interval_ms = 0;
    BspLed_Off(p_drv);
}

/**
 * @brief  LED 闪烁时基钩子，需在主循环中周期调用。
 *
 * Steps:
 *  1. 检查是否启动了闪烁，未启动则直接返回。
 *  2. 判断是否到达闪烁间隔，到达后翻转 LED。
 *
 * @param[in] p_drv : LED 驱动实例指针。
 *
 * @note  通过 p_os_ops->pf_get_tick 获取系统时间，无直接 HAL 调用。
 * */
void BspLed_TimebaseHook(bsp_led_driver_t *p_drv)
{
    uint32_t now = 0;

    if (!p_drv->is_blinking)
    {
        return;
    }

    now = p_drv->p_os_ops->pf_get_tick();
    if ((now - p_drv->blink_last_tick) >= p_drv->blink_interval_ms)
    {
        p_drv->blink_last_tick = now;
        BspLed_Toggle(p_drv);
    }
}
