/**
 * @file    ota_aes.c
 * @brief   AES-256-CTR decryption wrapper for OTA firmware
 * @author  GS_Mark
 *
 * @par dependencies
 * - elog.h
 * - ota_aes.h
 * - ota_aes_key.h
 * - aes.h (Vendor/tiny-AES-c)
 */

#define LOG_TAG "AES"
#include "elog.h"
#include "ota_aes.h"
#include "ota_aes_key.h"
#include "aes.h"
#include <string.h>

//*** Private Variables ***//

static struct AES_ctx s_aes_ctx;
static uint8_t        s_initialized;

//*** Implement ***//

void OtaAes_Init(const uint8_t nonce[OTA_AES_NONCE_SIZE])
{
    uint8_t iv[16];

    /* Pad 12-byte nonce to 16-byte IV: nonce(12B) || counter_start(4B) */
    memcpy(iv, nonce, OTA_AES_NONCE_SIZE);
    iv[12] = 0x00;
    iv[13] = 0x00;
    iv[14] = 0x00;
    iv[15] = 0x00;

    AES_init_ctx_iv(&s_aes_ctx, OtaAesKey_Get(), iv);
    s_initialized = 1;

    log_i("AES-256-CTR initialized (nonce=%02X%02X%02X%02X...)",
          nonce[0], nonce[1], nonce[2], nonce[3]);
}

void OtaAes_DecryptChunk(uint8_t *data, uint32_t len)
{
    if (!s_initialized)
    {
        log_e("AES not initialized!");
        return;
    }

    AES_CTR_xcrypt_buffer(&s_aes_ctx, data, len);
}

void OtaAes_Deinit(void)
{
    memset(&s_aes_ctx, 0, sizeof(s_aes_ctx));
    s_initialized = 0;
    log_i("AES context zeroized.");
}
