/**
 * @file    ota_aes.c
 * @brief   AES-256-CTR 解密封装实现
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

//*** 私有变量 ***//

static struct AES_ctx s_aes_ctx;
static uint8_t        s_initialized;

//*** 实现 ***//

/**
 * @brief  初始化 AES-256-CTR 解密上下文。
 *
 * Steps:
 *  1. 将 12 字节 nonce 填充为 16 字节 IV: nonce(12B) || 0x00000000(4B)。
 *  2. 调用 AES_init_ctx_iv 初始化上下文。
 *
 * @param[in] nonce : 来自 OTA 包头的 12 字节 nonce。
 * */
void OtaAes_Init(const uint8_t nonce[OTA_AES_NONCE_SIZE])
{
    uint8_t iv[16];

    /* 将 12 字节 nonce 填充为 16 字节 IV: nonce(12B) || counter_start(4B) */
    memcpy(iv, nonce, OTA_AES_NONCE_SIZE);
    iv[12] = 0x00;
    iv[13] = 0x00;
    iv[14] = 0x00;
    iv[15] = 0x00;

    AES_init_ctx_iv(&s_aes_ctx, OtaAesKey_Get(), iv);
    s_initialized = 1;

    log_i("AES-256-CTR 已初始化 (nonce=%02X%02X%02X%02X...)",
          nonce[0], nonce[1], nonce[2], nonce[3]);
}

/**
 * @brief  原地解密一段固件数据。
 *
 * Steps:
 *  1. 检查是否已初始化。
 *  2. 调用 AES_CTR_xcrypt_buffer 流式解密。
 *
 * @param[in,out] data : 数据缓冲区，原地解密。
 * @param[in]     len  : 待解密字节数。
 *
 * @note  支持流式调用 — 计数器状态跨调用保持。
 * */
void OtaAes_DecryptChunk(uint8_t *data, uint32_t len)
{
    if (!s_initialized)
    {
        log_e("AES 未初始化！");
        return;
    }

    AES_CTR_xcrypt_buffer(&s_aes_ctx, data, len);
}

/**
 * @brief  反初始化 AES 上下文（清零状态）。
 * */
void OtaAes_Deinit(void)
{
    memset(&s_aes_ctx, 0, sizeof(s_aes_ctx));
    s_initialized = 0;
    log_i("AES 上下文已清零。");
}
