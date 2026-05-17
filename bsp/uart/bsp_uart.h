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
     * @brief  初始化 UART 驱动并注册实例
     * @param  p_drv     驱动实例（调用者分配）
     * @param  p_config  硬件配置（需保持有效，通常为 const 全局变量）
     */
    void BspUart_Init(bsp_uart_driver_t *p_drv, const bsp_uart_config_t *p_config);

    /**
     * @brief  启动 DMA + IDLE 接收
     *         接收到的数据通过 BSP_UART_EVT_RX_DONE 回调上报。
     *         每次接收完成后自动重启接收。
     * @param  p_drv  驱动实例
     * @retval 0   成功
     * @retval -1  HAL 错误
     */
    int BspUart_StartReceive(bsp_uart_driver_t *p_drv);

    /**
     * @brief  停止 DMA 接收
     * @param  p_drv  驱动实例
     */
    void BspUart_StopReceive(bsp_uart_driver_t *p_drv);

    /**
     * @brief  非阻塞 DMA 发送
     * @param  p_drv   驱动实例
     * @param  p_data  发送数据缓冲区
     * @param  len     发送字节数
     * @retval 0   成功
     * @retval -1  前一次发送未完成或 HAL 错误
     */
    int BspUart_Send(bsp_uart_driver_t *p_drv, const uint8_t *p_data, uint16_t len);

    /**
     * @brief  阻塞式发送数据（带超时）
     * @param  p_drv       驱动实例
     * @param  p_data      发送数据缓冲区
     * @param  len         发送字节数
     * @param  timeout_ms  超时时间（毫秒）
     * @retval 0   成功
     * @retval -1  超时或发送失败
     */
    int BspUart_SendBlocking(bsp_uart_driver_t *p_drv,
                             const uint8_t     *p_data,
                             uint16_t           len,
                             uint32_t           timeout_ms);

    /**
     * @brief  格式化打印输出（阻塞式，线程安全）
     *         使用栈缓冲区，可安全跨 FreeRTOS 任务调用。
     *         禁止在 ISR 中使用。
     * @param  p_drv  驱动实例
     * @param  fmt    格式化字符串
     * @param  ...    可变参数
     * @return 实际写入字符数，失败返回 -1
     */
    int BspUart_Printf(bsp_uart_driver_t *p_drv, const char *fmt, ...);

    /**
     * @brief  查询 TX 是否忙碌
     * @param  p_drv  驱动实例
     * @return 1 忙碌, 0 空闲
     */
    uint8_t BspUart_IsTxBusy(const bsp_uart_driver_t *p_drv);

    /**
     * @brief  查询 RX 是否忙碌
     * @param  p_drv  驱动实例
     * @return 1 忙碌, 0 空闲
     */
    uint8_t BspUart_IsRxBusy(const bsp_uart_driver_t *p_drv);

#ifdef __cplusplus
}
#endif

#endif /* BSP_UART_H */
