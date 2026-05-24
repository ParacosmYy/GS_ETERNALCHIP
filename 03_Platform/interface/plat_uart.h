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
 * @brief   平台 UART 接口 — 头文件不含 HAL 类型
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

/** @brief  UART 事件类型 */
typedef enum
{
    BSP_UART_EVT_RX_DONE,  /**< 接收完成（IDLE 或缓冲区满） */
    BSP_UART_EVT_TX_DONE,  /**< DMA 发送完成 */
    BSP_UART_EVT_ERROR,    /**< UART 错误（自动重启接收） */
} bsp_uart_event_t;

/** @brief  UART 事件回调函数类型 */
typedef void (*bsp_uart_callback_t)(bsp_uart_event_t evt,
                                    uint8_t         *p_data,
                                    uint16_t         length,
                                    void            *p_user_data);

/** @brief  UART 硬件配置 — HAL 句柄使用不透明 void* */
typedef struct
{
    void               *p_huart;      /**< HAL UART 句柄（不透明） */
    uint8_t            *p_rx_buf;     /**< 接收缓冲区（调用方分配） */
    uint16_t            rx_buf_size;  /**< 接收缓冲区大小 */
    bsp_uart_callback_t callback;     /**< 事件回调 */
    void               *p_user_data;  /**< 回调用户上下文 */
} bsp_uart_config_t;

/** @brief  硬件操作 — 抽象 HAL 调用 */
typedef struct
{
    int  (*pf_start_dma_rx)(void *p_huart, uint8_t *buf, uint16_t size);
    void (*pf_stop_rx)(void *p_huart);
    int  (*pf_send_dma)(void *p_huart, const uint8_t *data, uint16_t len);
    int  (*pf_send_blocking)(void *p_huart, const uint8_t *data, uint16_t len, uint32_t timeout);
    void (*pf_flush_dr)(void *p_huart);
} uart_hw_operations_t;

/** @brief  OS 操作 — 抽象 OS/tick 调用 */
typedef struct
{
    uint32_t (*pf_get_tick)(void);
    void     (*pf_delay_ms)(uint32_t ms);
} uart_os_operations_t;

/** @brief  UART 驱动实例 — 完整定义（无 HAL 类型） */
typedef struct bsp_uart_driver
{
    const bsp_uart_config_t    *p_config;  /**< 配置引用 */
    const uart_hw_operations_t *p_hw_ops;  /**< 硬件 ops（注入） */
    const uart_os_operations_t *p_os_ops;  /**< OS ops（注入） */
    volatile uint8_t            tx_busy;   /**< 1 = DMA 发送中 */
    volatile uint8_t            rx_busy;   /**< 1 = DMA 接收中 */
    circular_buffer_t          *p_ring;    /**< 环形缓冲区（可选） */
    volatile uint8_t            ring_mode; /**< 1 = 环形缓冲区模式 */
} bsp_uart_driver_t;

//******************************* Declaring ********************************//

/** @brief  初始化 UART 驱动并注册实例 */
void     BspUart_Init(bsp_uart_driver_t *p_drv, const bsp_uart_config_t *p_config,
                       const uart_hw_operations_t *p_hw_ops, const uart_os_operations_t *p_os_ops);

/** @brief  启动 DMA + IDLE 接收 */
int      BspUart_StartReceive(bsp_uart_driver_t *p_drv);

/** @brief  停止 DMA 接收 */
void     BspUart_StopReceive(bsp_uart_driver_t *p_drv);

/** @brief  非阻塞 DMA 发送 */
int      BspUart_Send(bsp_uart_driver_t *p_drv, const uint8_t *p_data, uint16_t len);

/** @brief  阻塞式发送 */
int      BspUart_SendBlocking(bsp_uart_driver_t *p_drv, const uint8_t *p_data,
                               uint16_t len, uint32_t timeout_ms);

/** @brief  printf 风格格式化输出（阻塞式，线程安全） */
int      BspUart_Printf(bsp_uart_driver_t *p_drv, const char *fmt, ...);

/** @brief  查询 TX 忙碌状态 */
uint8_t  BspUart_IsTxBusy(const bsp_uart_driver_t *p_drv);

/** @brief  查询 RX 忙碌状态 */
uint8_t  BspUart_IsRxBusy(const bsp_uart_driver_t *p_drv);

/** @brief  绑定环形缓冲区到 UART 驱动（DMA + IDLE 模式） */
void     BspUart_BindRingBuffer(bsp_uart_driver_t *p_drv, circular_buffer_t *p_ring);

/** @brief  从环形缓冲区读取一个字节（带超时） */
int      BspUart_ReadByte(bsp_uart_driver_t *p_drv, uint8_t *p_byte, uint32_t timeout_ms);

/** @brief  刷新 UART 接收数据寄存器 */
void     BspUart_FlushRx(bsp_uart_driver_t *p_drv);

#ifdef __cplusplus
}
#endif

#endif /* PLAT_UART_H */
