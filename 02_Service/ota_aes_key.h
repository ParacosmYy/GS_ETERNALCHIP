/**
 * @file    ota_aes_key.h
 * @brief   AES-256 密钥访问接口 — OTA 固件解密
 * @author  GS_Mark
 *
 * 密钥存储在 ota_aes_key.c 中为 const 数组。
 * 生产环境建议使用 STM32 UID 派生密钥或安全元件。
 */

#ifndef OTA_AES_KEY_H
#define OTA_AES_KEY_H

#include <stdint.h>

/** @brief  获取 32 字节 AES-256 密钥指针 */
const uint8_t *OtaAesKey_Get(void);

#endif /* OTA_AES_KEY_H */
