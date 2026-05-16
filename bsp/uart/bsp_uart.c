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

static void register_instance(bsp_uart_driver_t *p_drv)
{
    for (int i = 0; i < BSP_UART_MAX_INSTANCES; i++) {
        if (s_instances[i] == NULL) {
            s_instances[i] = p_drv;
            return;
        }
    }
}

static void unregister_instance(bsp_uart_driver_t *p_drv)
{
    for (int i = 0; i < BSP_UART_MAX_INSTANCES; i++) {
        if (s_instances[i] == p_drv) {
            s_instances[i] = NULL;
            return;
        }
    }
}

static bsp_uart_driver_t *find_instance(UART_HandleTypeDef *p_huart)
{
    for (int i = 0; i < BSP_UART_MAX_INSTANCES; i++) {
        if (s_instances[i] != NULL &&
            s_instances[i]->p_config->p_huart == p_huart) {
            return s_instances[i];
        }
    }
    return NULL;
}

static void notify(bsp_uart_driver_t *p_drv, bsp_uart_event_t evt,
                   uint8_t *p_data, uint16_t length)
{
    if (p_drv->p_config->callback != NULL) {
        p_drv->p_config->callback(evt, p_data, length,
                                  p_drv->p_config->p_user_data);
    }
}

static int start_dma_rx(bsp_uart_driver_t *p_drv)
{
    const bsp_uart_config_t *cfg = p_drv->p_config;

    if (HAL_UARTEx_ReceiveToIdle_DMA(cfg->p_huart,
                                     cfg->p_rx_buf,
                                     cfg->rx_buf_size) != HAL_OK) {
        return -1;
    }

    /* Disable half-transfer interrupt — we only want IDLE or full */
    __HAL_DMA_DISABLE_IT(cfg->p_huart->hdmarx, DMA_IT_HT);

    p_drv->rx_busy = 1;
    return 0;
}

//*** Public API ***//

void BspUart_Init(bsp_uart_driver_t *p_drv, const bsp_uart_config_t *p_config)
{
    memset(p_drv, 0, sizeof(*p_drv));
    p_drv->p_config = p_config;
    register_instance(p_drv);
}

int BspUart_StartReceive(bsp_uart_driver_t *p_drv)
{
    return start_dma_rx(p_drv);
}

void BspUart_StopReceive(bsp_uart_driver_t *p_drv)
{
    HAL_UART_AbortReceive(p_drv->p_config->p_huart);
    p_drv->rx_busy = 0;
}

int BspUart_Send(bsp_uart_driver_t *p_drv, const uint8_t *p_data, uint16_t len)
{
    if (p_drv->tx_busy) {
        return -1;
    }

    if (HAL_UART_Transmit_DMA(p_drv->p_config->p_huart,
                              p_data, len) != HAL_OK) {
        return -1;
    }

    p_drv->tx_busy = 1;
    return 0;
}

int BspUart_SendBlocking(bsp_uart_driver_t *p_drv, const uint8_t *p_data,
                         uint16_t len, uint32_t timeout_ms)
{
    if (HAL_UART_Transmit(p_drv->p_config->p_huart,
                          p_data, len, timeout_ms) != HAL_OK) {
        return -1;
    }
    return 0;
}

int BspUart_Printf(bsp_uart_driver_t *p_drv, const char *fmt, ...)
{
    char buf[BSP_UART_PRINTF_BUF_SIZE];
    va_list args;

    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len <= 0) {
        return -1;
    }

    if ((uint16_t)len > sizeof(buf) - 1) {
        len = (int)(sizeof(buf) - 1);
    }

    if (BspUart_SendBlocking(p_drv, (const uint8_t *)buf,
                             (uint16_t)len, 100) != 0) {
        return -1;
    }
    return len;
}

uint8_t BspUart_IsTxBusy(const bsp_uart_driver_t *p_drv)
{
    return p_drv->tx_busy;
}

uint8_t BspUart_IsRxBusy(const bsp_uart_driver_t *p_drv)
{
    return p_drv->rx_busy;
}

//*** HAL Weak Callback Overrides ***//

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
    bsp_uart_driver_t *p_drv = find_instance(huart);
    if (p_drv == NULL) {
        return;
    }

    p_drv->rx_busy = 0;

    notify(p_drv, BSP_UART_EVT_RX_DONE, p_drv->p_config->p_rx_buf, size);

    /* Auto-restart DMA reception */
    if (p_drv->p_config->p_huart->gState != HAL_UART_STATE_RESET) {
        start_dma_rx(p_drv);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    bsp_uart_driver_t *p_drv = find_instance(huart);
    if (p_drv == NULL) {
        return;
    }

    p_drv->tx_busy = 0;
    notify(p_drv, BSP_UART_EVT_TX_DONE, NULL, 0);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    bsp_uart_driver_t *p_drv = find_instance(huart);
    if (p_drv == NULL) {
        return;
    }

    p_drv->rx_busy = 0;
    p_drv->tx_busy = 0;

    notify(p_drv, BSP_UART_EVT_ERROR, NULL, 0);

    /* Auto-restart reception after error */
    if (huart->gState != HAL_UART_STATE_RESET) {
        start_dma_rx(p_drv);
    }
}
