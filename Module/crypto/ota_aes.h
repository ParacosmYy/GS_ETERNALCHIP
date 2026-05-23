/**
 * @file    ota_aes.h
 * @brief   AES-256-CTR decryption wrapper for OTA firmware
 * @author  GS_Mark
 *
 * @par dependencies
 * - aes.h (Vendor/tiny-AES-c)
 *
 * Wraps tiny-AES-c for OTA encrypted firmware streaming decryption.
 * Package format: [nonce 12B][encrypted_fw][ECDSA sig 64B]
 *
 * Usage:
 *   1. OtaAes_Init()   — called once after nonce extracted
 *   2. OtaAes_DecryptChunk() — called for each firmware chunk
 *   3. OtaAes_Deinit() — cleanup after transfer complete
 */

#ifndef OTA_AES_H
#define OTA_AES_H

#include <stdint.h>

/** @brief Nonce size in bytes (stored in OTA package header) */
#define OTA_AES_NONCE_SIZE  12

/** @brief AES-256 key size in bytes */
#define OTA_AES_KEY_SIZE    32

/**
 * @brief  Initialize AES-256-CTR decryption context.
 *
 * @param[in] nonce : 12-byte nonce from OTA package header.
 *
 * Internally pads nonce to 16 bytes: nonce(12B) || 0x00000000(4B).
 */
void OtaAes_Init(const uint8_t nonce[OTA_AES_NONCE_SIZE]);

/**
 * @brief  Decrypt a chunk of firmware data in-place.
 *
 * @param[in,out] data : Data buffer, decrypted in-place.
 * @param[in]     len  : Number of bytes to decrypt.
 *
 * Supports streaming — state (counter) is preserved across calls.
 */
void OtaAes_DecryptChunk(uint8_t *data, uint32_t len);

/**
 * @brief  Deinitialize AES context (zeroize state).
 */
void OtaAes_Deinit(void);

#endif /* OTA_AES_H */
