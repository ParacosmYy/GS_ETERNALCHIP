/**
 * @file    bsp_key_handler.h
 * @brief   按键实例注册表 — 多按键统一管理
 * @author  GS_Mark
 *
 * @note    Handler 管理一个静态实例表，支持多按键场景：
 *          - Register/Unregister 单个按键驱动实例
 *          - ScanAll 批量扫描所有已注册按键
 *          - GetByIndex 按索引查找实例
 */

#ifndef BSP_KEY_HANDLER_H
#define BSP_KEY_HANDLER_H

#ifdef __cplusplus
extern "C"
{
#endif

//*** 头文件 ***//
#include "bsp_key_driver.h"

    //*** 类型 ***//

    /** @brief  按键实例注册表 */
    typedef struct
    {
        uint32_t           count;
        bsp_key_driver_t *instances[BSP_KEY_MAX_INSTANCES];
    } key_instance_table_t;

    //*** API ***//

    /** @brief  初始化按键实例注册表（清零所有槽位） */
    void BspKeyHandler_Init(void);

    /** @brief  将按键驱动实例注册到注册表 */
    void BspKeyHandler_Register(bsp_key_driver_t *p_drv);

    /** @brief  从注册表注销按键驱动实例 */
    void BspKeyHandler_Unregister(bsp_key_driver_t *p_drv);

    /** @brief  批量扫描所有已注册按键（需在主循环中周期调用） */
    void BspKeyHandler_ScanAll(void);

    /** @brief  通过索引查找已注册的按键驱动实例 */
    bsp_key_driver_t *BspKeyHandler_GetByIndex(uint32_t index);

#ifdef __cplusplus
}
#endif

#endif /* BSP_KEY_HANDLER_H */
