/**
 * @file    ota_aes_key.c
 * @brief   AES-256 预共享密钥存储 — OTA 固件解密
 * @author  GS_Mark
 *
 * 警告：此密钥必须与 tools/firmware_sign.py 使用的密钥一致
 *       （存储在 PC 端 tools/keys/aes_key.bin）。
 *
 * 密钥生成: python tools/firmware_sign.py --genkey --keydir tools/keys/
 * AES 密钥在首次签名操作时自动生成。
 *
 * 生产环境升级路径:
 *   - STM32 Option Bytes 存储密钥（一次写入）
 *   - ECDH 密钥交换实现会话密钥
 *   - 安全元件（ATECC608A 等）
 */

#include "ota_aes_key.h"

/*
 * AES-256 密钥（32 字节）。
 * 由 tools/firmware_sign.py 生成（首次 --sign 调用时）
 * 对应文件: tools/keys/aes_key.bin
 *
 * Hex: 76fb72e286c9b0715f34f0f1f93fe5b6c51a34c08ded11d4b7e559a7d164911d
 */
static const uint8_t s_aes_key[32] = {
    0x76, 0xfb, 0x72, 0xe2, 0x86, 0xc9, 0xb0, 0x71,
    0x5f, 0x34, 0xf0, 0xf1, 0xf9, 0x3f, 0xe5, 0xb6,
    0xc5, 0x1a, 0x34, 0xc0, 0x8d, 0xed, 0x11, 0xd4,
    0xb7, 0xe5, 0x59, 0xa7, 0xd1, 0x64, 0x91, 0x1d,
};

/**
 * @brief  获取 AES-256 密钥指针。
 *
 * @return 指向 32 字节常量密钥缓冲区的指针。
 * */
const uint8_t *OtaAesKey_Get(void)
{
    return s_aes_key;
}
