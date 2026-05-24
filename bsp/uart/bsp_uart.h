/**
 * @file    bsp_uart.h
 * @brief   UART BSP driver — DMA + IDLE reception, Printf support
 * @author  GS_Mark
 */

#ifndef BSP_UART_H
#define BSP_UART_H

#ifdef __cplusplus
extern "C"
{
#endif

//*** Includes ***//
#include "stm32f4xx_hal.h"
#include "../../utils/ring_buffer.h"
#include <stdint.h>

    //*** Configurable Macros ***//

#ifndef BSP_UART_MAX_INSTANCES
#    define BSP_UART_MAX_INSTANCES 2
#endif

#ifndef BSP_UART_PRINTF_BUF_SIZE
#    define BSP_UART_PRINTF_BUF_SIZE 128
#endif

    //*** Type Definitions ***//

    /** @brief  UART 事件类型 */
    typedef enum
    {
        BSP_UART_EVT_RX_DONE, /**< 接收完成（IDLE 或缓冲区满） */
        BSP_UART_EVT_TX_DONE, /**< DMA 发送完成 */
        BSP_UART_EVT_ERROR,   /**< UART 错误（自动重启接收） */
    } bsp_uart_event_t;

    /** @brief  UART 事件回调函数类型 */
    typedef void (*bsp_uart_callback_t)(bsp_uart_event_t evt,
                                        uint8_t         *p_data,
                                        uint16_t         length,
                                        void            *p_user_data);

    /** @brief  UART 硬件配置 */
    typedef struct
    {
        UART_HandleTypeDef *p_huart;     /**< HAL UART 句柄 */
        uint8_t            *p_rx_buf;    /**< RX 缓冲区（调用者分配） */
        uint16_t            rx_buf_size; /**< RX 缓冲区大小 */
        bsp_uart_callback_t callback;    /**< 事件回调 */
        void               *p_user_data; /**< 回调用户上下文 */
    } bsp_uart_config_t;

    /** @brief  UART 驱动实例 */
    typedef struct
    {
        const bsp_uart_config_t *p_config; /**< 配置引用 */
        volatile uint8_t         tx_busy;  /**< 1 = DMA 发送中 */
        volatile uint8_t         rx_busy;  /**< 1 = DMA 接收中 */
        circular_buffer_t       *p_ring;   /**< ring buffer（可选） */
        volatile uint8_t         ring_mode;/**< 1 = ring buffer 模式 */
    } bsp_uart_driver_t;

    //*** Public API ***//

    /** @brief  初始化 UART 驱动并注册实例 */
    void BspUart_Init(bsp_uart_driver_t *p_drv, const bsp_uart_config_t *p_config);

    /** @brief  启动 DMA + IDLE 接收 */
    int BspUart_StartReceive(bsp_uart_driver_t *p_drv);

    /** @brief  停止 DMA 接收 */
    void BspUart_StopReceive(bsp_uart_driver_t *p_drv);

    /** @brief  非阻塞 DMA 发送 */
    int BspUart_Send(bsp_uart_driver_t *p_drv, const uint8_t *p_data, uint16_t len);

    /** @brief  阻塞式发送数据 */
    int BspUart_SendBlocking(bsp_uart_driver_t *p_drv,
                             const uint8_t     *p_data,
                             uint16_t           len,
                             uint32_t           timeout_ms);

    /** @brief  格式化打印输出（阻塞式，线程安全） */
    int BspUart_Printf(bsp_uart_driver_t *p_drv, const char *fmt, ...);

    /** @brief  查询 TX 是否忙碌 */
    uint8_t BspUart_IsTxBusy(const bsp_uart_driver_t *p_drv);

    /** @brief  查询 RX 是否忙碌 */
    uint8_t BspUart_IsRxBusy(const bsp_uart_driver_t *p_drv);

    //*** Ring Buffer Mode API ***//

    /**
     * @brief  绑定 ring buffer 到 UART 驱动（DMA + IDLE 模式）。
     *
     *         ISR 收到数据后 push 到 ring buffer，应用层通过
     *         BspUart_ReadByte() pop 读取。
     *
     * @param[in] p_drv  : UART 驱动实例指针。
     * @param[in] p_ring : 外部分配的 ring buffer。
     * */
    void BspUart_BindRingBuffer(bsp_uart_driver_t *p_drv, circular_buffer_t *p_ring);

    /**
     * @brief  从 ring buffer 读取一个字节（带超时）。
     *
     *         在 FreeRTOS 任务中调用，超时期间让出 CPU。
     *
     * @param[in]  p_drv      : UART 驱动实例指针。
     * @param[out] p_byte     : 存放读取的字节。
     * @param[in]  timeout_ms : 超时时间（毫秒）。
     *
     * @return   0 : 成功。
     * @return  -1 : 超时。
     * */
    int BspUart_ReadByte(bsp_uart_driver_t *p_drv, uint8_t *p_byte, uint32_t timeout_ms);

    /** @brief  清空 UART RX 数据寄存器（丢弃接收缓冲区中的残留数据） */
    void BspUart_FlushRx(bsp_uart_driver_t *p_drv);

#ifdef __cplusplus
}
#endif

#endif /* BSP_UART_H */
