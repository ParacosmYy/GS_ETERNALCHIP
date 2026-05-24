/**
 * @file    ota_aes.h
 * @brief   AES-256-CTR 解密封装 — OTA 固件流式解密
 * @author  GS_Mark
 *
 * @par dependencies
 * - aes.h (Vendor/tiny-AES-c)
 *
 * 封装 tiny-AES-c 库，用于 OTA 加密固件流式解密。
 * 包格式: [nonce 12B][encrypted_fw][ECDSA sig 64B]
 *
 * 用法:
 *   1. OtaAes_Init()        — 提取 nonce 后调用一次
 *   2. OtaAes_DecryptChunk() — 每接收一段固件数据调用一次
 *   3. OtaAes_Deinit()      — 传输完成后清理
 */

#ifndef OTA_AES_H
#define OTA_AES_H

#include <stdint.h>

/** @brief Nonce 大小（字节），存储在 OTA 包头 */
#define OTA_AES_NONCE_SIZE  12

/** @brief AES-256 密钥大小（字节） */
#define OTA_AES_KEY_SIZE    32

/** @brief  初始化 AES-256-CTR 解密上下文 */
void OtaAes_Init(const uint8_t nonce[OTA_AES_NONCE_SIZE]);

/** @brief  原地解密一段固件数据 */
void OtaAes_DecryptChunk(uint8_t *data, uint32_t len);

/** @brief  反初始化 AES 上下文（清零状态） */
void OtaAes_Deinit(void);

#endif /* OTA_AES_H */
