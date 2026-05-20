/**
 * @file    ota_verify.h
 * @brief   OTA firmware verification — SHA-256 Flash hash utility
 * @author  GS_Mark
 */

#ifndef OTA_VERIFY_H
#define OTA_VERIFY_H

#include <stdint.h>

/** @brief  计算 Flash 指定区域的 SHA-256 哈希值 */
int OtaVerify_SHA256Flash(uint32_t addr, uint32_t len, uint8_t hash[32]);

#endif /* OTA_VERIFY_H */
