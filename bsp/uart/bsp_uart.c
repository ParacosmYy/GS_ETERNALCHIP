/**
 * @file    bsp_uart.c
 * @brief   UART BSP driver implementation — DMA + IDLE + Printf
 * @author  GS_Mark
 *
 * @par dependencies
 * - bsp_uart.h
 */

//*** Includes ***//
#include "bsp_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "cmsis_os.h"

//*** Private Variables — Instance Registry ***//

static bsp_uart_driver_t *s_instances[BSP_UART_MAX_INSTANCES];

//*** Private Helpers ***//

/**
 * @brief  将 UART 驱动实例注册到全局实例表。
 *
 * @param[in] p_drv : UART 驱动实例指针。
 * */
static void register_instance(bsp_uart_driver_t *p_drv)
{
    int i;

    for (i = 0; i < BSP_UART_MAX_INSTANCES; i++)
    {
        if (s_instances[i] == NULL)
        {
            s_instances[i] = p_drv;
            return;
        }
    }
}

/**
 * @brief  从全局实例表中注销 UART 驱动实例。
 *
 * @param[in] p_drv : 要注销的 UART 驱动实例指针。
 * */
static void unregister_instance(bsp_uart_driver_t *p_drv)
{
    int i;

    for (i = 0; i < BSP_UART_MAX_INSTANCES; i++)
    {
        if (s_instances[i] == p_drv)
        {
            s_instances[i] = NULL;
            return;
        }
    }
}

/**
 * @brief  根据 HAL UART 句柄查找已注册的驱动实例。
 *
 * @param[in] p_huart : HAL UART 句柄指针。
 *
 * @return  找到的驱动实例指针，未找到则返回 NULL。
 * */
static bsp_uart_driver_t *find_instance(UART_HandleTypeDef *p_huart)
{
    int i;

    for (i = 0; i < BSP_UART_MAX_INSTANCES; i++)
    {
        if (s_instances[i] != NULL && s_instances[i]->p_config->p_huart == p_huart)
        {
            return s_instances[i];
        }
    }
    return NULL;
}

/**
 * @brief  向应用层发送事件通知（若用户已注册回调则调用之）。
 *
 * @param[in] p_drv   : UART 驱动实例指针。
 * @param[in] evt     : 事件类型。
 * @param[in] p_data  : 接收数据指针（发送完成事件时为 NULL）。
 * @param[in] length  : 数据长度。
 * */
static void notify(bsp_uart_driver_t *p_drv, bsp_uart_event_t evt, uint8_t *p_data, uint16_t length)
{
    if (p_drv->p_config->callback != NULL)
    {
        p_drv->p_config->callback(evt, p_data, length, p_drv->p_config->p_user_data);
    }
}

/**
 * @brief  启动 DMA + IDLE 接收。
 *
 * Steps:
 *  1. 调用 HAL_UARTEx_ReceiveToIdle_DMA 启动 DMA 接收。
 *  2. 关闭半传输中断，仅在总线空闲或缓冲区满时触发回调。
 *
 * @param[in] p_drv : UART 驱动实例指针。
 *
 * @return   0 : 启动成功。
 * @return  -1 : HAL 启动失败。
 * */
static int start_dma_rx(bsp_uart_driver_t *p_drv)
{
    const bsp_uart_config_t *cfg;

    cfg = p_drv->p_config;

    if (HAL_UARTEx_ReceiveToIdle_DMA(cfg->p_huart, cfg->p_rx_buf, cfg->rx_buf_size) != HAL_OK)
    {
        return -1;
    }

    /* 关闭半传输中断，仅在 IDLE 或缓冲区满时回调 */
    __HAL_DMA_DISABLE_IT(cfg->p_huart->hdmarx, DMA_IT_HT);

    p_drv->rx_busy = 1;
    return 0;
}

//*** Public API ***//

/**
 * @brief  初始化 UART 驱动实例。
 *
 * Steps:
 *  1. 将驱动结构体清零。
 *  2. 绑定配置指针。
 *  3. 注册到全局实例表（供 HAL 回调查找）。
 *
 * @param[out] p_drv    : UART 驱动实例指针。
 * @param[in]  p_config : UART 配置（HAL 句柄、接收缓冲区、回调等）。
 * */
void BspUart_Init(bsp_uart_driver_t *p_drv, const bsp_uart_config_t *p_config)
{
    memset(p_drv, 0, sizeof(*p_drv));
    p_drv->p_config = p_config;
    register_instance(p_drv);
}

/**
 * @brief  启动 DMA 接收。
 *
 * @param[in] p_drv : UART 驱动实例指针。
 *
 * @return   0 : 启动成功。
 * @return  -1 : 启动失败。
 * */
int BspUart_StartReceive(bsp_uart_driver_t *p_drv)
{
    return start_dma_rx(p_drv);
}

/**
 * @brief  停止 DMA 接收。
 *
 * @param[in] p_drv : UART 驱动实例指针。
 * */
void BspUart_StopReceive(bsp_uart_driver_t *p_drv)
{
    HAL_UART_AbortReceive(p_drv->p_config->p_huart);
    p_drv->rx_busy = 0;
}

/**
 * @brief  非阻塞 DMA 发送。
 *
 * @param[in] p_drv  : UART 驱动实例指针。
 * @param[in] p_data : 发送数据指针。
 * @param[in] len    : 数据长度（字节）。
 *
 * @return   0 : 发送已提交。
 * @return  -1 : 发送忙或 HAL 调用失败。
 * */
int BspUart_Send(bsp_uart_driver_t *p_drv, const uint8_t *p_data, uint16_t len)
{
    if (p_drv->tx_busy)
    {
        return -1;
    }

    if (HAL_UART_Transmit_DMA(p_drv->p_config->p_huart, p_data, len) != HAL_OK)
    {
        return -1;
    }

    p_drv->tx_busy = 1;
    return 0;
}

/**
 * @brief  阻塞式发送。
 *
 * @param[in] p_drv       : UART 驱动实例指针。
 * @param[in] p_data      : 发送数据指针。
 * @param[in] len         : 数据长度（字节）。
 * @param[in] timeout_ms  : 超时时间（ms）。
 *
 * @return   0 : 发送成功。
 * @return  -1 : 超时或 HAL 调用失败。
 * */
int BspUart_SendBlocking(bsp_uart_driver_t *p_drv,
                         const uint8_t     *p_data,
                         uint16_t           len,
                         uint32_t           timeout_ms)
{
    if (HAL_UART_Transmit(p_drv->p_config->p_huart, p_data, len, timeout_ms) != HAL_OK)
    {
        return -1;
    }
    return 0;
}

/**
 * @brief  格式化打印到 UART（阻塞式）。
 *
 * Steps:
 *  1. 使用 vsnprintf 格式化到栈缓冲区。
 *  2. 截断超出缓冲区大小的部分。
 *  3. 调用阻塞发送输出。
 *
 * @param[in] p_drv : UART 驱动实例指针。
 * @param[in] fmt   : 格式化字符串（同 printf）。
 * @param[in] ...   : 可变参数。
 *
 * @return  >0 : 实际发送的字节数。
 * @return  -1 : 格式化失败或发送失败。
 * */
int BspUart_Printf(bsp_uart_driver_t *p_drv, const char *fmt, ...)
{
    char    buf[BSP_UART_PRINTF_BUF_SIZE];
    va_list args;
    int     len;

    va_start(args, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len <= 0)
    {
        return -1;
    }

    if ((uint16_t)len > sizeof(buf) - 1)
    {
        len = (int)(sizeof(buf) - 1);
    }

    if (BspUart_SendBlocking(p_drv, (const uint8_t *)buf, (uint16_t)len, 100) != 0)
    {
        return -1;
    }
    return len;
}

/**
 * @brief  查询发送是否繁忙。
 *
 * @param[in] p_drv : UART 驱动实例指针。
 *
 * @return  1 : 繁忙。
 * @return  0 : 空闲。
 * */
uint8_t BspUart_IsTxBusy(const bsp_uart_driver_t *p_drv)
{
    return p_drv->tx_busy;
}

/**
 * @brief  查询接收是否繁忙。
 *
 * @param[in] p_drv : UART 驱动实例指针。
 *
 * @return  1 : 繁忙。
 * @return  0 : 空闲。
 * */
uint8_t BspUart_IsRxBusy(const bsp_uart_driver_t *p_drv)
{
    return p_drv->rx_busy;
}

//*** Ring Buffer Mode API ***//

/**
 * @brief  绑定 ring buffer 到 UART 驱动。
 *
 * @param[in] p_drv  : UART 驱动实例指针。
 * @param[in] p_ring : 外部分配的 ring buffer。
 * */
void BspUart_BindRingBuffer(bsp_uart_driver_t *p_drv, circular_buffer_t *p_ring)
{
    p_drv->p_ring     = p_ring;
    p_drv->ring_mode  = (p_ring != NULL) ? 1 : 0;
}

/**
 * @brief  从 ring buffer 读取一个字节（带超时）。
 *
 * @param[in]  p_drv      : UART 驱动实例指针。
 * @param[out] p_byte     : 存放读取的字节。
 * @param[in]  timeout_ms : 超时时间（毫秒）。
 *
 * @return   0 : 成功。
 * @return  -1 : 超时。
 * */
int BspUart_ReadByte(bsp_uart_driver_t *p_drv, uint8_t *p_byte, uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();

    do
    {
        int16_t val;

        if (p_drv->p_ring == NULL)
        {
            return -1;
        }

        val = ring_buffer_get_byte(p_drv->p_ring);
        if (val >= 0)
        {
            *p_byte = (uint8_t)val;
            return 0;
        }

        if (timeout_ms == 0)
        {
            return -1;
        }

        osDelay(1);
    } while ((HAL_GetTick() - start) < timeout_ms);

    return -1;
}

//*** HAL Weak Callback Overrides ***//

/**
 * @brief  HAL UART 接收事件回调（DMA + IDLE 模式）。
 *
 * Steps:
 *  1. 通过 huart 查找已注册的驱动实例。
 *  2. 清除 rx_busy 标志，通知应用层接收完成。
 *  3. 自动重启 DMA 接收（若 UART 未被复位）。
 *
 * @param[in] huart : HAL UART 句柄指针。
 * @param[in] size  : 本次接收到的数据长度（字节）。
 * */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
    bsp_uart_driver_t *p_drv;

    p_drv = find_instance(huart);
    if (p_drv == NULL)
    {
        return;
    }

    p_drv->rx_busy = 0;

    /* Ring buffer 模式：push 数据到 ring buffer，不调回调 */
    if (p_drv->ring_mode && p_drv->p_ring != NULL)
    {
        uint8_t *p_data;
        uint16_t i;

        p_data = p_drv->p_config->p_rx_buf;

        for (i = 0; i < size; i++)
        {
            if (!ring_buffer_put_byte(p_drv->p_ring, p_data[i]))
            {
                break;
            }
        }
    }
    else
    {
        notify(p_drv, BSP_UART_EVT_RX_DONE, p_drv->p_config->p_rx_buf, size);
    }

    /* 自动重启 DMA 接收 */
    if (p_drv->p_config->p_huart->gState != HAL_UART_STATE_RESET)
    {
        start_dma_rx(p_drv);
    }
}

/**
 * @brief  HAL UART 发送完成回调。
 *
 * Steps:
 *  1. 查找驱动实例。
 *  2. 清除 tx_busy 标志，通知应用层发送完成。
 *
 * @param[in] huart : HAL UART 句柄指针。
 * */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    bsp_uart_driver_t *p_drv;

    p_drv = find_instance(huart);
    if (p_drv == NULL)
    {
        return;
    }

    p_drv->tx_busy = 0;
    notify(p_drv, BSP_UART_EVT_TX_DONE, NULL, 0);
}

/**
 * @brief  HAL UART 错误回调。
 *
 * Steps:
 *  1. 查找驱动实例。
 *  2. 清除收发忙标志，通知应用层错误事件。
 *  3. 自动重启接收。
 *
 * @param[in] huart : HAL UART 句柄指针。
 * */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    bsp_uart_driver_t *p_drv;

    p_drv = find_instance(huart);
    if (p_drv == NULL)
    {
        return;
    }

    p_drv->rx_busy = 0;
    p_drv->tx_busy = 0;

    notify(p_drv, BSP_UART_EVT_ERROR, NULL, 0);

    /* 错误后自动重启接收 */
    if (huart->gState != HAL_UART_STATE_RESET)
    {
        start_dma_rx(p_drv);
    }
}
