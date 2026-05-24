/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    plat_uart.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author  GS_Mark
 *
 * @brief   Platform UART interface — opaque driver handle
 *
 * @version V1.0 2025-05-24
 *
 * @note    1 tab == 4 spaces
 *
 *****************************************************************************/
#ifndef PLAT_UART_H
#define PLAT_UART_H

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include <stdint.h>
//******************************* Includes **********************************//

//******************************* Types *************************************//
/** @brief  Opaque UART driver — consumers never see internals */
typedef struct bsp_uart_driver bsp_uart_driver_t;

//******************************* Declaring ********************************//
void BspUart_FlushRx(bsp_uart_driver_t *p_drv);
int  BspUart_ReadByte(bsp_uart_driver_t *p_drv, uint8_t *p_byte, uint32_t timeout_ms);
int  BspUart_SendBlocking(bsp_uart_driver_t *p_drv, const uint8_t *p_data,
                           uint16_t len, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* PLAT_UART_H */
