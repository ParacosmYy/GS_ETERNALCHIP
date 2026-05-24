/**
 * @file    bsp_uart_handler.h
 * @brief   UART 实例注册表 — HAL 句柄到驱动实例的映射
 * @author  GS_Mark
 *
 * @note    Handler 管理一个静态实例表，供 HAL 回调（ISR 上下文）
 *          从原始 UART_HandleTypeDef 指针查找所属的 bsp_uart_driver_t。
 *          将注册关注点与驱动逻辑分离。
 */

#ifndef BSP_UART_HANDLER_H
#define BSP_UART_HANDLER_H

#ifdef __cplusplus
extern "C"
{
#endif

//*** 头文件 ***//
#include "bsp_uart_driver.h"

    //*** 类型 ***//

    /** @brief  实例注册表 */
    typedef struct
    {
        uint32_t            instance_num;
        bsp_uart_driver_t *instances[BSP_UART_MAX_INSTANCES];
    } uart_instance_table_t;

    //*** API ***//

    /** @brief  初始化实例注册表（清零所有槽位） */
    void              BspUartHandler_Init(void);

    /** @brief  将 UART 驱动实例注册到注册表 */
    void              BspUartHandler_Register(bsp_uart_driver_t *p_drv);

    /** @brief  从注册表注销 UART 驱动实例 */
    void              BspUartHandler_Unregister(bsp_uart_driver_t *p_drv);

    /** @brief  通过 HAL 句柄（void* 不透明指针）查找已注册的驱动实例 */
    bsp_uart_driver_t *BspUartHandler_FindByHandle(void *p_huart);

#ifdef __cplusplus
}
#endif

#endif /* BSP_UART_HANDLER_H */
