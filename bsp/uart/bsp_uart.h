/**
 * @file    bsp_uart.h
 * @brief   UART BSP driver — DMA + IDLE reception, Printf support
 * @note    Call BspUart_Init() then BspUart_StartReceive() from a task.
 *          Received data is reported via callback event.
 */

#ifndef BSP_UART_H
#define BSP_UART_H

#ifdef __cplusplus
extern "C"
{
#endif

//*** Includes ***//
#include "stm32f4xx_hal.h"
#include <stdint.h>

    //*** Configurable Macros ***//

#ifndef BSP_UART_MAX_INSTANCES
#    define BSP_UART_MAX_INSTANCES 2
#endif

#ifndef BSP_UART_PRINTF_BUF_SIZE
#    define BSP_UART_PRINTF_BUF_SIZE 128
#endif

    //*** Type Definitions ***//

    /** @brief  UART event types reported via callback */
    typedef enum
    {
        BSP_UART_EVT_RX_DONE, /**< Packet received (IDLE or buffer full)  */
        BSP_UART_EVT_TX_DONE, /**< DMA transmit complete                  */
        BSP_UART_EVT_ERROR,   /**< UART error (auto-restart applied)      */
    } bsp_uart_event_t;

    /** @brief  UART event callback function type */
    typedef void (*bsp_uart_callback_t)(bsp_uart_event_t evt,
                                        uint8_t         *p_data,
                                        uint16_t         length,
                                        void            *p_user_data);

    /**
     * @brief  UART hardware configuration (immutable, usually const)
     */
    typedef struct
    {
        UART_HandleTypeDef *p_huart;     /**< HAL UART handle (e.g. &huart1)   */
        uint8_t            *p_rx_buf;    /**< RX buffer (caller-allocated)     */
        uint16_t            rx_buf_size; /**< RX buffer size in bytes          */
        bsp_uart_callback_t callback;    /**< Event callback                   */
        void               *p_user_data; /**< User context passed to callback  */
    } bsp_uart_config_t;

    /**
     * @brief  UART driver instance (one per physical UART peripheral)
     */
    typedef struct
    {
        const bsp_uart_config_t *p_config; /**< Configuration reference         */
        volatile uint8_t         tx_busy;  /**< 1 = DMA TX in progress           */
        volatile uint8_t         rx_busy;  /**< 1 = DMA RX in progress           */
    } bsp_uart_driver_t;

    //*** Public API ***//

    /**
     * @brief  Initialise UART driver and register instance.
     * @param  p_drv    Driver instance (caller-allocated)
     * @param  p_config Hardware configuration (must remain valid)
     */
    void BspUart_Init(bsp_uart_driver_t *p_drv, const bsp_uart_config_t *p_config);

    /**
     * @brief  Start DMA + IDLE reception.
     *         Received data reported via BSP_UART_EVT_RX_DONE callback.
     *         Reception auto-restarts after each packet.
     * @return 0 on success, -1 on HAL error
     */
    int BspUart_StartReceive(bsp_uart_driver_t *p_drv);

    /**
     * @brief  Stop DMA reception.
     */
    void BspUart_StopReceive(bsp_uart_driver_t *p_drv);

    /**
     * @brief  Non-blocking DMA transmit.
     * @return 0 on success, -1 if previous TX still busy or HAL error
     */
    int BspUart_Send(bsp_uart_driver_t *p_drv, const uint8_t *p_data, uint16_t len);

    /**
     * @brief  Blocking transmit with timeout.
     * @return 0 on success, -1 on timeout or error
     */
    int BspUart_SendBlocking(bsp_uart_driver_t *p_drv,
                             const uint8_t     *p_data,
                             uint16_t           len,
                             uint32_t           timeout_ms);

    /**
     * @brief  Printf-style formatted output (blocking).
     *         Uses stack buffer, thread-safe across FreeRTOS tasks.
     *         Do NOT call from ISR context.
     * @return Number of characters written, or -1 on error
     */
    int BspUart_Printf(bsp_uart_driver_t *p_drv, const char *fmt, ...);

    /** @brief  Query TX busy state */
    uint8_t BspUart_IsTxBusy(const bsp_uart_driver_t *p_drv);

    /** @brief  Query RX busy state */
    uint8_t BspUart_IsRxBusy(const bsp_uart_driver_t *p_drv);

#ifdef __cplusplus
}
#endif

#endif /* BSP_UART_H */
