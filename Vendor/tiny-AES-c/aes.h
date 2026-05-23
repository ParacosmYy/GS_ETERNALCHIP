/**
 * @file    aes.h
 * @brief   Tiny AES-256 implementation (CTR mode only)
 * @author  kokke (original), adapted for STM32 OTA
 *
 * Original: https://github.com/kokke/tiny-AES-c
 * License:  Public Domain / Unlicense
 *
 * Stripped to AES-256-CTR only for embedded OTA use.
 * No ECB/CBC modes included to minimize code size.
 */

#ifndef _AES_H_
#define _AES_H_

#include <stdint.h>

#define AES_BLOCKLEN      16  /**< Block length in bytes */
#define AES_KEYLEN        32  /**< Key length in bytes (AES-256) */
#define AES_keyExpSize   240  /**< Expanded key length */

struct AES_ctx
{
    uint8_t RoundKey[AES_keyExpSize];
    uint8_t Iv[AES_BLOCKLEN];
    uint8_t keystream[AES_BLOCKLEN]; /**< Cached keystream block for CTR */
    uint8_t block_offset;            /**< Current position in keystream (0..15).
                                          AES_BLOCKLEN = exhausted, trigger regeneration */
};

/**
 * @brief  Initialize AES-256 context with key and IV.
 *
 * @param[in,out] ctx  : AES context.
 * @param[in]     key  : 32-byte AES key.
 * @param[in]     iv   : 16-byte initial counter (nonce || counter).
 */
void AES_init_ctx_iv(struct AES_ctx *ctx, const uint8_t *key, const uint8_t *iv);

/**
 * @brief  AES-256-CTR encrypt/decrypt in-place (same operation).
 *
 * @param[in,out] ctx    : AES context.
 * @param[in,out] buffer : Data buffer (encrypted/decrypted in-place).
 * @param[in]     length : Number of bytes to process.
 */
void AES_CTR_xcrypt_buffer(struct AES_ctx *ctx, uint8_t *buffer, uint32_t length);

#endif /* _AES_H_ */
