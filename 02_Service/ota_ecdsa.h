/**
 * @file    ota_ecdsa.h
 * @brief   OTA 固件 ECDSA-P256 签名验证
 * @author  GS_Mark
 */

#ifndef OTA_ECDSA_H
#define OTA_ECDSA_H

#include <stdint.h>

/** @brief  ECDSA-P256 签名长度（r‖s 各 32 字节） */
#define OTA_ECDSA_SIG_SIZE  64

/**
 * @brief  验证固件 ECDSA-P256 签名，同时输出 SHA-256 哈希。
 *
 * @param[out] out_hash  输出计算所得 SHA-256（可为 NULL，表示不需要）。
 * */
int OtaEcdsa_Verify(uint32_t fw_addr, uint32_t fw_len,
                     const uint8_t signature[OTA_ECDSA_SIG_SIZE],
                     uint8_t out_hash[32]);

#endif /* OTA_ECDSA_H */
