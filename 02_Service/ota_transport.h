/**
 * @file    ota_transport.h
 * @brief   OTA transport layer — UART adapter + YMODEM data callback
 * @author  GS_Mark
 *
 * Handles encrypted OTA package format:
 *   [nonce 12B][encrypted_fw][ECDSA sig 64B]
 * Decrypts firmware via AES-256-CTR and extracts signature for verification.
 */

#ifndef OTA_TRANSPORT_H
#define OTA_TRANSPORT_H

#include <stdint.h>

/** @brief Forward declaration for WDG driver */
typedef struct bsp_wdg_driver bsp_wdg_driver_t;

/** @brief  ECDSA-P256 signature size (for OtaTransport_GetSignature param) */
#define OTA_TRANSPORT_SIG_SIZE  64

/** @brief  初始化 OTA 传输模块（p_uart_drv 为 bsp_uart_driver_t*，p_wdg_drv 为 bsp_wdg_driver_t*） */
void OtaTransport_Init(uint32_t *p_fw_size, uint32_t target_base, void *p_uart_drv,
                       bsp_wdg_driver_t *p_wdg_drv);

/** @brief  UART 发送单字节（适配 YMODEM 接口） */
void OtaTransport_SendByte(uint8_t byte, void *p_user);

/** @brief  UART 接收单字节（适配 YMODEM 接口） */
int OtaTransport_RecvByte(uint8_t *p_byte, uint32_t timeout_ms, void *p_user);

/** @brief  YMODEM 数据接收回调（解密 + 写 Flash + 签名提取 + 喂狗） */
int OtaTransport_DataCallback(uint32_t offset, const uint8_t *p_data,
                               uint16_t len, void *p_user);

/** @brief  传输完成后获取提取的 ECDSA 签名（64 字节） */
void OtaTransport_GetSignature(uint8_t sig[OTA_TRANSPORT_SIG_SIZE]);

#endif /* OTA_TRANSPORT_H */
