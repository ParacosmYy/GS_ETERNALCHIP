/**
 * @file    bsp_uart.c
 * @brief   UART BSP driver implementation — DMA + IDLE + Printf
 */

//*** Includes ***//
#include "bsp_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

//*** Private Variables — Instance Registry ***//

static bsp_uart_driver_t *s_instances[BSP_UART_MAX_INSTANCES];

//*** Private Helpers ***//

/**
 * @brief  将 UART 驱动实例注册到全局实例表
 *
 *         在实例数组中寻找空槽位并写入指针
 *
 * @param  p_drv  UART 驱动实例指针
 */
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
 * @brief  从全局实例表中注销 UART 驱动实例
 *
 * @param  p_drv  要注销的 UART 驱动实例指针
 */
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
 * @brief  根据 HAL UART 句柄查找已注册的驱动实例
 *
 * @param  p_huart  HAL UART 句柄指针
 * @return 找到的驱动实例指针，未找到则返回 NULL
 */
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
 * @brief  向应用层发送事件通知
 *
 *         若用户已注册回调函数则调用之
 *
 * @param  p_drv   UART 驱动实例指针
 * @param  evt     事件类型
 * @param  p_data  接收数据指针（发送完成事件时为 NULL）
 * @param  length  数据长度
 */
static void notify(bsp_uart_driver_t *p_drv, bsp_uart_event_t evt, uint8_t *p_data, uint16_t length)
{
    if (p_drv->p_config->callback != NULL)
    {
        p_drv->p_config->callback(evt, p_data, length, p_drv->p_config->p_user_data);
    }
}

/**
 * @brief  启动 DMA + IDLE 接收
 *
 *         调用 HAL 接口启动 DMA 接收，并关闭半传输中断，
 *         仅在总线空闲或缓冲区满时触发回调
 *
 * @param  p_drv  UART 驱动实例指针
 * @retval 0   启动成功
 * @retval -1  HAL 启动失败
 */
static int start_dma_rx(bsp_uart_driver_t *p_drv)
{
    const bsp_uart_config_t *cfg;

    cfg = p_drv->p_config;

    if (HAL_UARTEx_ReceiveToIdle_DMA(cfg->p_huart, cfg->p_rx_buf, cfg->rx_buf_size) != HAL_OK)
    {
        return -1;
    }

    /* Disable half-transfer interrupt — we only want IDLE or full */
    __HAL_DMA_DISABLE_IT(cfg->p_huart->hdmarx, DMA_IT_HT);

    p_drv->rx_busy = 1;
    return 0;
}

//*** Public API ***//

/**
 * @brief  初始化 UART 驱动实例
 *
 *         将驱动结构体清零，绑定配置并注册到全局实例表
 *
 * @param  p_drv     UART 驱动实例指针
 * @param  p_config  UART 配置（HAL 句柄、接收缓冲区、回调等）
 */
void BspUart_Init(bsp_uart_driver_t *p_drv, const bsp_uart_config_t *p_config)
{
    memset(p_drv, 0, sizeof(*p_drv));
    p_drv->p_config = p_config;
    register_instance(p_drv);
}

/**
 * @brief  启动 DMA 接收
 *
 * @param  p_drv  UART 驱动实例指针
 * @retval 0   启动成功
 * @retval -1  启动失败
 */
int BspUart_StartReceive(bsp_uart_driver_t *p_drv)
{
    return start_dma_rx(p_drv);
}

/**
 * @brief  停止 DMA 接收
 *
 * @param  p_drv  UART 驱动实例指针
 */
void BspUart_StopReceive(bsp_uart_driver_t *p_drv)
{
    HAL_UART_AbortReceive(p_drv->p_config->p_huart);
    p_drv->rx_busy = 0;
}

/**
 * @brief  非阻塞 DMA 发送
 *
 * @param  p_drv   UART 驱动实例指针
 * @param  p_data  发送数据指针
 * @param  len     数据长度（字节）
 * @retval 0   发送已提交
 * @retval -1  发送忙或 HAL 调用失败
 */
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
 * @brief  阻塞式发送
 *
 * @param  p_drv       UART 驱动实例指针
 * @param  p_data      发送数据指针
 * @param  len         数据长度（字节）
 * @param  timeout_ms  超时时间（ms）
 * @retval 0   发送成功
 * @retval -1  超时或 HAL 调用失败
 */
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
 * @brief  格式化打印到 UART（阻塞式）
 *
 *         内部使用 vsnprintf 格式化到栈缓冲区后调用阻塞发送
 *
 * @param  p_drv  UART 驱动实例指针
 * @param  fmt    格式化字符串（同 printf）
 * @param  ...    可变参数
 * @retval >0  实际发送的字节数
 * @retval -1  格式化失败或发送失败
 */
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
 * @brief  查询发送是否繁忙
 *
 * @param  p_drv  UART 驱动实例指针
 * @return 1 繁忙，0 空闲
 */
uint8_t BspUart_IsTxBusy(const bsp_uart_driver_t *p_drv)
{
    return p_drv->tx_busy;
}

/**
 * @brief  查询接收是否繁忙
 *
 * @param  p_drv  UART 驱动实例指针
 * @return 1 繁忙，0 空闲
 */
uint8_t BspUart_IsRxBusy(const bsp_uart_driver_t *p_drv)
{
    return p_drv->rx_busy;
}

//*** HAL Weak Callback Overrides ***//

/**
 * @brief  HAL UART 接收事件回调（DMA + IDLE 模式）
 *
 *         当总线空闲或接收缓冲区满时由 HAL 调用。
 *         通知应用层后自动重启 DMA 接收
 *
 * @param  huart  HAL UART 句柄指针
 * @param  size   本次接收到的数据长度（字节）
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
    bsp_uart_driver_t *p_drv;

    p_drv = find_instance(huart);
    if (p_drv == NULL)
    {
        return;
    }

    p_drv->rx_busy = 0;

    notify(p_drv, BSP_UART_EVT_RX_DONE, p_drv->p_config->p_rx_buf, size);

    /* Auto-restart DMA reception */
    if (p_drv->p_config->p_huart->gState != HAL_UART_STATE_RESET)
    {
        start_dma_rx(p_drv);
    }
}

/**
 * @brief  HAL UART 发送完成回调
 *
 *         DMA 发送完成后由 HAL 调用，清除忙标志并通知应用层
 *
 * @param  huart  HAL UART 句柄指针
 */
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
 * @brief  HAL UART 错误回调
 *
 *         发生帧错误、溢出等异常时由 HAL 调用，
 *         清除收发忙标志并通知应用层，随后自动重启接收
 *
 * @param  huart  HAL UART 句柄指针
 */
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

    /* Auto-restart reception after error */
    if (huart->gState != HAL_UART_STATE_RESET)
    {
        start_dma_rx(p_drv);
    }
}
