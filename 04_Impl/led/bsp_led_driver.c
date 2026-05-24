/**
 * @file    bsp_led_driver.c
 * @brief   LED BSP driver implementation — STM32 HAL backend
 * @author  GS_Mark
 *
 * @par dependencies
 * - bsp_led_driver.h
 *
 * Steps:
 *  1. HAL cast macro converts plat_gpio_t.void* port to GPIO_TypeDef*.
 *  2. Static functions implement led_operations_t via HAL GPIO API.
 *  3. Public API delegates to ops function pointers.
 */

//*** Includes ***//
#include "bsp_led_driver.h"
#include <string.h>

//*** Private Macros ***//

/** @brief  将 plat_gpio_t 的 opaque port 指针转换为 HAL GPIO_TypeDef* */
#define HAL_GPIO_PORT(p) ((GPIO_TypeDef *)(((const plat_gpio_t *)(p))->port))

/** @brief  从 plat_gpio_t 获取 HAL GPIO pin（1 << pin） */
#define HAL_GPIO_PIN(p)  ((uint16_t)(1U << ((const plat_gpio_t *)(p))->pin))

//*** Private Functions — HAL Backend ***//

/**
 * @brief  HAL 实现：点亮 LED。
 *         低有效 LED 写 RESET，高有效 LED 写 SET。
 *
 * Steps:
 *  1. 从 p_config->gpio 读取 active_level 判断极性。
 *  2. 调用 HAL_GPIO_WritePin 执行 GPIO 写操作。
 *
 * @param[in] p_drv : LED 驱动实例指针。
 * */
static void HalLed_On(bsp_led_driver_t *p_drv)
{
    const plat_gpio_t *p_gpio = &p_drv->p_config->gpio;

    if (p_gpio->active_level == 0)
    {
        HAL_GPIO_WritePin(HAL_GPIO_PORT(p_gpio), HAL_GPIO_PIN(p_gpio), GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(HAL_GPIO_PORT(p_gpio), HAL_GPIO_PIN(p_gpio), GPIO_PIN_SET);
    }
}

/**
 * @brief  HAL 实现：熄灭 LED。
 *         与 On 极性相反。
 *
 * Steps:
 *  1. 从 p_config->gpio 读取 active_level 判断极性。
 *  2. 调用 HAL_GPIO_WritePin 执行 GPIO 写操作（极性反转）。
 *
 * @param[in] p_drv : LED 驱动实例指针。
 * */
static void HalLed_Off(bsp_led_driver_t *p_drv)
{
    const plat_gpio_t *p_gpio = &p_drv->p_config->gpio;

    if (p_gpio->active_level == 0)
    {
        HAL_GPIO_WritePin(HAL_GPIO_PORT(p_gpio), HAL_GPIO_PIN(p_gpio), GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(HAL_GPIO_PORT(p_gpio), HAL_GPIO_PIN(p_gpio), GPIO_PIN_RESET);
    }
}

/**
 * @brief  HAL 实现：翻转 LED 状态。
 *
 * Steps:
 *  1. 调用 HAL_GPIO_TogglePin 翻转 GPIO 输出。
 *
 * @param[in] p_drv : LED 驱动实例指针。
 * */
static void HalLed_Toggle(bsp_led_driver_t *p_drv)
{
    const plat_gpio_t *p_gpio = &p_drv->p_config->gpio;

    HAL_GPIO_TogglePin(HAL_GPIO_PORT(p_gpio), HAL_GPIO_PIN(p_gpio));
}

//*** Public Variables ***//

const led_operations_t bsp_led_hal_ops = {
    .p_On     = HalLed_On,
    .p_Off    = HalLed_Off,
    .p_Toggle = HalLed_Toggle,
};

//*** Public API ***//

/**
 * @brief  初始化 LED 驱动实例。
 *
 * Steps:
 *  1. 将驱动结构体清零。
 *  2. 绑定硬件配置和 HAL 操作函数表。
 *
 * @param[out] p_drv    : LED 驱动实例指针。
 * @param[in]  p_config : LED 硬件配置（plat_gpio_t 格式）。
 * */
void BspLed_Init(bsp_led_driver_t *p_drv, const bsp_led_config_t *p_config)
{
    memset(p_drv, 0, sizeof(*p_drv));
    p_drv->p_config = p_config;
    p_drv->p_ops    = &bsp_led_hal_ops;
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
    p_drv->blink_last_tick   = HAL_GetTick();
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
 * @note TODO: HAL_GetTick() will be replaced by ops in system_adaption phase.
 * */
void BspLed_TimebaseHook(bsp_led_driver_t *p_drv)
{
    uint32_t now = 0;

    if (!p_drv->is_blinking)
    {
        return;
    }

    /* TODO: HAL_GetTick() will be replaced by ops->GetTick() in system_adaption phase */
    now = HAL_GetTick();
    if ((now - p_drv->blink_last_tick) >= p_drv->blink_interval_ms)
    {
        p_drv->blink_last_tick = now;
        BspLed_Toggle(p_drv);
    }
}
