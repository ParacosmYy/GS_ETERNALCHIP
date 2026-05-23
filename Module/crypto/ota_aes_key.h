/**
 * @file    ota_aes_key.h
 * @brief   AES-256 key accessor for OTA firmware decryption
 * @author  GS_Mark
 *
 * Key is stored in ota_aes_key.c as a const array.
 * In production, consider using STM32 UID-derived key or secure element.
 */

#ifndef OTA_AES_KEY_H
#define OTA_AES_KEY_H

#include <stdint.h>

/**
 * @brief  Get pointer to the 32-byte AES-256 key.
 *
 * @return Pointer to constant key buffer (32 bytes).
 */
const uint8_t *OtaAesKey_Get(void);

#endif /* OTA_AES_KEY_H */
