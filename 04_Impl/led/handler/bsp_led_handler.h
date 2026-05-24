/**
 * @file    bsp_led_handler.h
 * @brief   LED 实例注册表 — 多 LED 统一管理
 * @author  GS_Mark
 *
 * @note    Handler 管理一个静态实例表，支持多 LED 场景：
 *          - Register/Unregister 单个 LED 驱动实例
 *          - TimebaseHookAll 驱动所有已注册 LED 的闪烁
 *          - GetByIndex 按索引查找实例
 */

#ifndef BSP_LED_HANDLER_H
#define BSP_LED_HANDLER_H

#ifdef __cplusplus
extern "C"
{
#endif

//*** 头文件 ***//
#include "bsp_led_driver.h"

    //*** 类型 ***//

    /** @brief  LED 实例注册表 */
    typedef struct
    {
        uint32_t           count;
        bsp_led_driver_t *instances[BSP_LED_MAX_INSTANCES];
    } led_instance_table_t;

    //*** API ***//

    /** @brief  初始化 LED 实例注册表（清零所有槽位） */
    void BspLedHandler_Init(void);

    /** @brief  将 LED 驱动实例注册到注册表 */
    void BspLedHandler_Register(bsp_led_driver_t *p_drv);

    /** @brief  从注册表注销 LED 驱动实例 */
    void BspLedHandler_Unregister(bsp_led_driver_t *p_drv);

    /** @brief  驱动所有已注册 LED 的闪烁时基（需在主循环中周期调用） */
    void BspLedHandler_TimebaseHookAll(void);

    /** @brief  通过索引查找已注册的 LED 驱动实例 */
    bsp_led_driver_t *BspLedHandler_GetByIndex(uint32_t index);

#ifdef __cplusplus
}
#endif

#endif /* BSP_LED_HANDLER_H */
