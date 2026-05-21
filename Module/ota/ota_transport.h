/**
 * @file    ota_transport.h
 * @brief   OTA transport layer — UART adapter + YMODEM data callback
 * @author  GS_Mark
 */

#ifndef OTA_TRANSPORT_H
#define OTA_TRANSPORT_H

#include <stdint.h>

/** @brief  初始化 OTA 传输模块（p_uart_drv 为 bsp_uart_driver_t*，传 NULL 则降级为轮询） */
void OtaTransport_Init(uint32_t *p_fw_size, uint32_t target_base, void *p_uart_drv);

/** @brief  UART 发送单字节（适配 YMODEM 接口） */
void OtaTransport_SendByte(uint8_t byte, void *p_user);

/** @brief  UART 接收单字节（适配 YMODEM 接口） */
int OtaTransport_RecvByte(uint8_t *p_byte, uint32_t timeout_ms, void *p_user);

/** @brief  YMODEM 数据接收回调（写 Flash + 进度 + 喂狗） */
int OtaTransport_DataCallback(uint32_t offset, const uint8_t *p_data,
                               uint16_t len, void *p_user);

#endif /* OTA_TRANSPORT_H */
