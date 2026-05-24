/**
 * @file    ota_ecdsa.c
 * @brief   OTA 固件 ECDSA-P256 签名验证实现
 * @author  GS_Mark
 *
 * @par dependencies
 * - elog.h
 * - ota_ecdsa.h
 * - sha256.h
 * - uECC.h
 */

#define LOG_TAG "ECDSA"
#include "elog.h"
#include "ota_ecdsa.h"
#include "sha256.h"
#include "uECC.h"
#include <string.h>

//*** 内嵌 ECDSA 公钥（P-256，未压缩 r||s） ***//

/*
 * 公钥硬编码在固件中，私钥永远不离开 PC 签名工具。
 * 生产环境中可通过安全 Bootloader 更新密钥对。
 * 使用前需要用 tools/gen_keys.py 生成真实密钥替换此处。
 */
static const uint8_t s_ecdsa_pubkey[64] = {
    /* X 坐标 (32 字节) */
    0x02, 0x51, 0x42, 0x40, 0x3A, 0x89, 0xD7, 0x6C,
    0x2B, 0x45, 0xE4, 0x96, 0x23, 0xEF, 0x58, 0x19,
    0x47, 0x62, 0x1D, 0xA2, 0x5C, 0xFC, 0x96, 0xD9,
    0xB7, 0x80, 0xBC, 0x9F, 0x88, 0xCE, 0x74, 0xB0,
    /* Y 坐标 (32 字节) */
    0x5E, 0xF3, 0x86, 0xA4, 0x80, 0x62, 0x10, 0xFC,
    0x55, 0x62, 0x2A, 0xFB, 0x13, 0x98, 0xB8, 0x00,
    0xC5, 0xB2, 0x4C, 0x7B, 0x8D, 0x6B, 0x75, 0x67,
    0x40, 0x69, 0xB8, 0xE7, 0x86, 0x44, 0x87, 0x69,
};

//*** 实现 ***//

/**
 * @brief  验证固件 ECDSA-P256 签名。
 *
 * Steps:
 *  1. 以 256 字节为单位从 Flash 读取固件，流式计算 SHA-256。
 *  2. 调用 micro-ecc 的 uECC_verify 验证签名。
 *  3. 签名格式为 raw r‖s（各 32 字节，共 64 字节）。
 *
 * @param[in]  fw_addr   : 固件 Flash 起始地址。
 * @param[in]  fw_len    : 固件长度（字节，不含签名尾部）。
 * @param[in]  signature : 64 字节 ECDSA-P256 签名 (r‖s)。
 * @param[out] out_hash  : 输出 SHA-256（可为 NULL）。
 *
 * @return   0 : 验签通过。
 * @return  -1 : 验签失败。
 *
 * @note  micro-ecc 在 Cortex-M4 @100MHz 上验签约需 150~300ms，
 *        对于一次性 OTA 操作完全可接受。
 * */
int OtaEcdsa_Verify(uint32_t fw_addr, uint32_t fw_len,
                     const uint8_t signature[OTA_ECDSA_SIG_SIZE],
                     uint8_t out_hash[32])
{
    SHA256_CTX                  ctx;
    uint8_t                     hash[32];
    uint8_t                     buf[256];
    uint32_t                    i;
    uint32_t                    chunk;
    const struct uECC_Curve_t *curve;

    /* 1. 流式计算固件 SHA-256（与 OtaVerify_SHA256Flash 逻辑一致） */
    sha256_init(&ctx);

    for (i = 0; i < fw_len; i += sizeof(buf))
    {
        chunk = fw_len - i;
        if (chunk > sizeof(buf))
        {
            chunk = sizeof(buf);
        }

        /* 直接通过地址读取 Flash（STM32 Flash 可直接寻址） */
        memcpy(buf, (const void *)(fw_addr + i), chunk);
        sha256_update(&ctx, buf, chunk);
    }

    sha256_final(&ctx, hash);

    /* 输出 SHA-256 供调用者复用，避免重复计算 */
    if (out_hash != NULL)
    {
        memcpy(out_hash, hash, 32);
    }

    /* 2. ECDSA-P256 验签（secp256r1 = NIST P-256） */
    curve = uECC_secp256r1();

    if (uECC_verify(s_ecdsa_pubkey, hash, 32, signature, curve) != 1)
    {
        log_e("ECDSA 验签失败！");
        return -1;
    }

    log_i("ECDSA 验签通过。");
    return 0;
}
