/**
 * @file    bsp_uart_handler.h
 * @brief   UART instance registry — maps HAL handles to driver instances
 * @author  GS_Mark
 *
 * @note    The Handler manages a static instance table used by HAL callbacks
 *          (ISR context) to find the owning bsp_uart_driver_t from a raw
 *          UART_HandleTypeDef pointer. This separates registry concerns from
 *          the driver logic.
 */

#ifndef BSP_UART_HANDLER_H
#define BSP_UART_HANDLER_H

#ifdef __cplusplus
extern "C"
{
#endif

//*** Includes ***//
#include "bsp_uart_driver.h"

    //*** Types ***//

    /** @brief  Instance registry table */
    typedef struct
    {
        uint32_t            instance_num;
        bsp_uart_driver_t *instances[BSP_UART_MAX_INSTANCES];
    } uart_instance_table_t;

    //*** API ***//

    /** @brief  Initialize the instance registry (zero all slots) */
    void              BspUartHandler_Init(void);

    /** @brief  Register a UART driver instance into the registry */
    void              BspUartHandler_Register(bsp_uart_driver_t *p_drv);

    /** @brief  Unregister a UART driver instance from the registry */
    void              BspUartHandler_Unregister(bsp_uart_driver_t *p_drv);

    /** @brief  Find a registered driver by its HAL handle (void* opaque) */
    bsp_uart_driver_t *BspUartHandler_FindByHandle(void *p_huart);

#ifdef __cplusplus
}
#endif

#endif /* BSP_UART_HANDLER_H */
