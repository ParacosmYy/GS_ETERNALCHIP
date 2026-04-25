/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */
#include <string.h>
#include <stdio.h>

#ifdef DEBUG
/* ---- DEBUG 模式: 原始 echo 回显 ---- */
#define RX_BUF_SIZE 256
static uint8_t rx_buf[RX_BUF_SIZE];
static uint8_t tx_buf[RX_BUF_SIZE];

#else
/* ---- 乒乓缓冲: DMA Circular 模式 ---- */
#define HALF_BUF_SIZE   256
#define FULL_BUF_SIZE   (HALF_BUF_SIZE * 2)  /* 512 bytes */
#define DEBUG_BUF_SIZE  128

static uint8_t rx_buf[FULL_BUF_SIZE];
static uint8_t debug_buf[DEBUG_BUF_SIZE];    /* 调试打印缓冲 */

/* 乒乓处理统计 (demo 调试用) */
static volatile uint32_t ht_count   = 0;     /* 前半填满次数 */
static volatile uint32_t tc_count   = 0;     /* 后半填满次数 */
static volatile uint32_t idle_count = 0;     /* IDLE 事件次数 */
#endif

/* USER CODE END 0 */

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 DMA Init */
    /* USART1_RX Init */
    hdma_usart1_rx.Instance = DMA2_Stream2;
    hdma_usart1_rx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_rx.Init.Mode = DMA_CIRCULAR;
    hdma_usart1_rx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart1_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart1_rx);

    /* USART1_TX Init */
    hdma_usart1_tx.Instance = DMA2_Stream7;
    hdma_usart1_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode = DMA_NORMAL;
    hdma_usart1_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart1_tx);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

    /* USART1 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);
    HAL_DMA_DeInit(uartHandle->hdmatx);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

void UART_StartReceive(void)
{
#ifdef DEBUG
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rx_buf, RX_BUF_SIZE);
    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
#else
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rx_buf, FULL_BUF_SIZE);
    /* 乒乓模式: 保留 HT 中断, 不禁用 */
#endif
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance != USART1) return;

#ifdef DEBUG
    /* ---- DEBUG: 原始 echo 回显 ---- */
    memcpy(tx_buf, rx_buf, Size);
    HAL_UART_Transmit_DMA(&huart1, tx_buf, Size);
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rx_buf, RX_BUF_SIZE);
    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);

#else
    /* ---- 乒乓缓冲 + 调试打印 (demo) ---- */
    int len = 0;

    if (huart->RxEventType == HAL_UART_RXEVENT_HT)
    {
        /* HT: 前半 [0, HALF_BUF_SIZE-1] 已填满,
         * DMA 正在写后半, 前半可安全读取 */
        ht_count++;
        len = snprintf((char *)debug_buf, DEBUG_BUF_SIZE,
            "[HT] #%lu half_A [0..255] first8:"
            " %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
            (unsigned long)ht_count,
            rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3],
            rx_buf[4], rx_buf[5], rx_buf[6], rx_buf[7]);
    }
    else if (huart->RxEventType == HAL_UART_RXEVENT_TC)
    {
        /* TC: 后半 [HALF_BUF_SIZE, FULL_BUF_SIZE-1] 已填满,
         * DMA 已回绕到前半, 后半可安全读取 */
        tc_count++;
        len = snprintf((char *)debug_buf, DEBUG_BUF_SIZE,
            "[TC] #%lu half_B [256..511] first8:"
            " %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
            (unsigned long)tc_count,
            rx_buf[HALF_BUF_SIZE + 0], rx_buf[HALF_BUF_SIZE + 1],
            rx_buf[HALF_BUF_SIZE + 2], rx_buf[HALF_BUF_SIZE + 3],
            rx_buf[HALF_BUF_SIZE + 4], rx_buf[HALF_BUF_SIZE + 5],
            rx_buf[HALF_BUF_SIZE + 6], rx_buf[HALF_BUF_SIZE + 7]);
    }
    else
    {
        /* IDLE: 变长数据帧, Size = 当前 DMA 写入位置 */
        idle_count++;
        uint16_t off = (Size <= HALF_BUF_SIZE) ? 0 : HALF_BUF_SIZE;
        len = snprintf((char *)debug_buf, DEBUG_BUF_SIZE,
            "[IDLE] #%lu Size=%u data@%u:"
            " %02X %02X %02X %02X\r\n",
            (unsigned long)idle_count, Size, off,
            rx_buf[off + 0], rx_buf[off + 1],
            rx_buf[off + 2], rx_buf[off + 3]);
    }

    /* 非阻塞调试打印: 仅在 TX 空闲时发送, 避免覆盖进行中的传输 */
    if (len > 0 && huart1.gState == HAL_UART_STATE_READY)
    {
        HAL_UART_Transmit_DMA(&huart1, debug_buf, (uint16_t)len);
    }
    /* Circular 模式: DMA RX 自动继续, 无需重启 */
#endif
}

/* USER CODE END 1 */
