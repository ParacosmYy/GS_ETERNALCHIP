/**
 * @file    bsp_uart_handler.c
 * @brief   UART 实例注册表实现
 * @author  GS_Mark
 *
 * @par dependencies
 * - bsp_uart_handler.h
 *
 * 静态实例表，将 HAL UART 句柄映射到 bsp_uart_driver_t 指针。
 * HAL 回调（ISR 上下文）调用 BspUartHandler_FindByHandle()
 * 将事件分发到正确的驱动实例。
 */

//*** 头文件 ***//
#include "bsp_uart_handler.h"
#include "bsp_uart_driver.h"
#include <string.h>

//*** 私有变量 ***//

/** @brief  静态实例表 */
static uart_instance_table_t s_table;

//*** 公共 API ***//

/**
 * @brief  初始化实例注册表（清零所有槽位）。
 * */
void BspUartHandler_Init(void)
{
    memset(&s_table, 0, sizeof(s_table));
}

/**
 * @brief  将 UART 驱动实例注册到全局实例表。
 *
 * Steps:
 *  1. 遍历实例表，找到第一个空槽位。
 *  2. 放入驱动指针，递增计数器。
 *
 * @param[in] p_drv : UART 驱动实例指针。
 * */
void BspUartHandler_Register(bsp_uart_driver_t *p_drv)
{
    int i;

    for (i = 0; i < BSP_UART_MAX_INSTANCES; i++)
    {
        if (s_table.instances[i] == NULL)
        {
            s_table.instances[i] = p_drv;
            s_table.instance_num++;
            return;
        }
    }
}

/**
 * @brief  从全局实例表中注销 UART 驱动实例。
 *
 * Steps:
 *  1. 遍历实例表，找到匹配的驱动指针。
 *  2. 清空槽位，递减计数器。
 *
 * @param[in] p_drv : 要注销的 UART 驱动实例指针。
 * */
void BspUartHandler_Unregister(bsp_uart_driver_t *p_drv)
{
    int i;

    for (i = 0; i < BSP_UART_MAX_INSTANCES; i++)
    {
        if (s_table.instances[i] == p_drv)
        {
            s_table.instances[i] = NULL;
            s_table.instance_num--;
            return;
        }
    }
}

/**
 * @brief  根据 HAL UART 句柄查找已注册的驱动实例。
 *
 * Steps:
 *  1. 遍历实例表，比较每个驱动配置中的 p_huart。
 *  2. 返回匹配的驱动指针，未找到返回 NULL。
 *
 * @param[in] p_huart : HAL UART 句柄指针（void* opaque）。
 *
 * @return  找到的驱动实例指针，未找到则返回 NULL。
 * */
bsp_uart_driver_t *BspUartHandler_FindByHandle(void *p_huart)
{
    int i;

    for (i = 0; i < BSP_UART_MAX_INSTANCES; i++)
    {
        if (s_table.instances[i] != NULL &&
            HAL_UART(s_table.instances[i]->p_config->p_huart) == (UART_HandleTypeDef *)p_huart)
        {
            return s_table.instances[i];
        }
    }
    return NULL;
}
