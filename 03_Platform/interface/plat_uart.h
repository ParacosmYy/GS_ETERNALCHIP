/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    plat_uart.h
 *
 * @par dependencies
 * - stdint.h
 * - ring_buffer.h (03_Platform/common)
 *
 * @author  GS_Mark
 *
 * @brief   Platform UART interface — no HAL types in header
 *
 * @version V2.0 2025-05-24
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
#include "ring_buffer.h"
//******************************* Includes **********************************//

//******************************* Types *************************************//

/** @brief  UART event types */
typedef enum
{
    BSP_UART_EVT_RX_DONE,  /**< Reception complete (IDLE or buffer full) */
    BSP_UART_EVT_TX_DONE,  /**< DMA transmit complete */
    BSP_UART_EVT_ERROR,    /**< UART error (auto-restart receive) */
} bsp_uart_event_t;

/** @brief  UART event callback function type */
typedef void (*bsp_uart_callback_t)(bsp_uart_event_t evt,
                                    uint8_t         *p_data,
                                    uint16_t         length,
                                    void            *p_user_data);

/** @brief  UART hardware configuration — HAL handle is opaque void* */
typedef struct
{
    void               *p_huart;      /**< HAL UART handle (opaque) */
    uint8_t            *p_rx_buf;     /**< RX buffer (caller-allocated) */
    uint16_t            rx_buf_size;  /**< RX buffer size */
    bsp_uart_callback_t callback;     /**< Event callback */
    void               *p_user_data;  /**< Callback user context */
} bsp_uart_config_t;

/** @brief  UART driver instance — full definition (no HAL types) */
typedef struct bsp_uart_driver
{
    const bsp_uart_config_t *p_config; /**< Configuration reference */
    volatile uint8_t         tx_busy;  /**< 1 = DMA transmitting */
    volatile uint8_t         rx_busy;  /**< 1 = DMA receiving */
    circular_buffer_t       *p_ring;   /**< ring buffer (optional) */
    volatile uint8_t         ring_mode;/**< 1 = ring buffer mode */
} bsp_uart_driver_t;

//******************************* Declaring ********************************//

/** @brief  Initialize UART driver and register instance */
void     BspUart_Init(bsp_uart_driver_t *p_drv, const bsp_uart_config_t *p_config);

/** @brief  Start DMA + IDLE reception */
int      BspUart_StartReceive(bsp_uart_driver_t *p_drv);

/** @brief  Stop DMA reception */
void     BspUart_StopReceive(bsp_uart_driver_t *p_drv);

/** @brief  Non-blocking DMA transmit */
int      BspUart_Send(bsp_uart_driver_t *p_drv, const uint8_t *p_data, uint16_t len);

/** @brief  Blocking transmit */
int      BspUart_SendBlocking(bsp_uart_driver_t *p_drv, const uint8_t *p_data,
                               uint16_t len, uint32_t timeout_ms);

/** @brief  Printf-style formatted output (blocking, thread-safe) */
int      BspUart_Printf(bsp_uart_driver_t *p_drv, const char *fmt, ...);

/** @brief  Query TX busy state */
uint8_t  BspUart_IsTxBusy(const bsp_uart_driver_t *p_drv);

/** @brief  Query RX busy state */
uint8_t  BspUart_IsRxBusy(const bsp_uart_driver_t *p_drv);

/** @brief  Bind ring buffer to UART driver (DMA + IDLE mode) */
void     BspUart_BindRingBuffer(bsp_uart_driver_t *p_drv, circular_buffer_t *p_ring);

/** @brief  Read one byte from ring buffer (with timeout) */
int      BspUart_ReadByte(bsp_uart_driver_t *p_drv, uint8_t *p_byte, uint32_t timeout_ms);

/** @brief  Flush UART RX data register */
void     BspUart_FlushRx(bsp_uart_driver_t *p_drv);

#ifdef __cplusplus
}
#endif

#endif /* PLAT_UART_H */
