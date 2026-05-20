/**
 * @file    ota_ecdsa.h
 * @brief   OTA firmware ECDSA-P256 signature verification
 * @author  GS_Mark
 */

#ifndef OTA_ECDSA_H
#define OTA_ECDSA_H

#include <stdint.h>

/** @brief  ECDSA-P256 签名长度（r‖s 各 32 字节） */
#define OTA_ECDSA_SIG_SIZE  64

/** @brief  验证 Slot B 固件的 ECDSA-P256 签名 */
int OtaEcdsa_Verify(uint32_t fw_addr, uint32_t fw_len,
                     const uint8_t signature[OTA_ECDSA_SIG_SIZE]);

#endif /* OTA_ECDSA_H */
