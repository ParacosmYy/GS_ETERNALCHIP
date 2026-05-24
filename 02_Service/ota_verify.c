/**
 * @file    ota_verify.c
 * @brief   OTA firmware verification — SHA-256 Flash hash
 * @author  GS_Mark
 *
 * @par dependencies
 * - elog.h
 * - ota_verify.h
 * - plat_flash.h
 * - sha256.h
 */

//*** Includes ***//
#define LOG_TAG "VERIFY"
#include "elog.h"
#include "ota_verify.h"
#include "plat_flash.h"
#include "sha256.h"
#include <string.h>

//*** Implement ***//

/**
 * @brief  计算 Flash 指定区域的 SHA-256 哈希值。
 *
 * Steps:
 *  1. 初始化 SHA-256 上下文。
 *  2. 以 256 字节为单位从 Flash 读取数据并更新哈希。
 *  3. 输出最终 32 字节哈希值。
 *
 * @param[in]  addr : Flash 起始地址。
 * @param[in]  len  : 数据长度（字节）。
 * @param[out] hash : 输出 32 字节 SHA-256 哈希值。
 *
 * @return  0 : 成功。
 * */
int OtaVerify_SHA256Flash(uint32_t addr, uint32_t len, uint8_t hash[32])
{
    SHA256_CTX ctx;
    uint8_t    buf[256];
    uint32_t   i;
    uint32_t   chunk;

    sha256_init(&ctx);

    for (i = 0; i < len; i += sizeof(buf))
    {
        /* 最后一次可能不足 256 字节，取剩余长度 */
        chunk = len - i;
        if (chunk > sizeof(buf))
        {
            chunk = sizeof(buf);
        }

        /* 直接通过地址读取 Flash 内容（STM32 Flash 可直接寻址） */
        memcpy(buf, (const void *)(addr + i), chunk);
        sha256_update(&ctx, buf, chunk);
    }

    sha256_final(&ctx, hash);
    return 0;
}
