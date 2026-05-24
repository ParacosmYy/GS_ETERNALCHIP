/**
 * @file    ota_transport.c
 * @brief   OTA transport layer implementation
 * @author  GS_Mark
 *
 * @par dependencies
 * - elog.h
 * - ota_transport.h
 * - plat_flash.h
 * - plat_uart.h
 * - plat_wdg.h
 * - plat_sys.h
 * - ota_aes.h
 *
 * UART byte-level adapters for YMODEM, and data callback that:
 *   1. Extracts 12-byte AES nonce from package header
 *   2. Decrypts firmware chunks via AES-256-CTR and writes to Flash
 *   3. Extracts 64-byte ECDSA signature from package tail
 *
 * Package format: [nonce 12B][encrypted_fw][ECDSA sig 64B]
 */

#define LOG_TAG "TRAN"
#include "elog.h"
#include "ota_transport.h"
#include "plat_flash.h"
#include "plat_uart.h"
#include "plat_wdg.h"
#include "plat_sys.h"
#include "ota_aes.h"
#include "ota_ecdsa.h"
#include <string.h>

//*** Private Constants ***//

/** @brief Package overhead: nonce(12B) + signature(64B) */
#define PKG_HEADER_SIZE  OTA_AES_NONCE_SIZE   /* 12 */
#define PKG_TAIL_SIZE    OTA_ECDSA_SIG_SIZE   /* 64 */
#define PKG_OVERHEAD     (PKG_HEADER_SIZE + PKG_TAIL_SIZE)  /* 76 */

//*** Receive State Machine ***//

typedef enum
{
    RECV_STATE_NONCE,      /**< Accumulating 12-byte nonce */
    RECV_STATE_FIRMWARE,   /**< Decrypting + writing firmware */
    RECV_STATE_SIGNATURE,  /**< Accumulating 64-byte signature */
} recv_state_t;

//*** Private Variables ***//

static uint32_t            *s_p_fw_size;
static uint32_t             s_target_base;
static bsp_uart_driver_t  *s_p_uart_drv;

/* State machine for encrypted package parsing */
static recv_state_t         s_recv_state;
static uint32_t             s_total_received;  /* total bytes received from YMODEM */
static uint32_t             s_fw_bytes_written;/* bytes written to Flash (decrypted fw) */

/* Nonce accumulation buffer */
static uint8_t              s_nonce_buf[OTA_AES_NONCE_SIZE];
static uint8_t              s_nonce_collected;

/* Signature accumulation buffer */
static uint8_t              s_sig_buf[OTA_ECDSA_SIG_SIZE];
static uint8_t              s_sig_collected;

/* Temp buffer for chunk processing */
static uint8_t              s_chunk_buf[1024 + PKG_OVERHEAD];

/* Extracted signature (readable by task_ota after transfer) */
static uint8_t              s_extracted_sig[OTA_ECDSA_SIG_SIZE];

//*** Init ***//

/**
 * @brief  初始化 OTA 传输模块，保存固件大小指针和目标 Bank 基地址。
 *
 * @param[in] p_fw_size   : 指向任务级固件大小变量（YMODEM Packet 0 填充）。
 * @param[in] target_base : 目标 Bank 的 Flash 基地址。
 * @param[in] p_uart_drv  : UART 驱动指针。
 * */
void OtaTransport_Init(uint32_t *p_fw_size, uint32_t target_base, void *p_uart_drv)
{
    s_p_fw_size     = p_fw_size;
    s_target_base   = target_base;
    s_p_uart_drv    = (bsp_uart_driver_t *)p_uart_drv;

    /* Reset state machine */
    s_recv_state      = RECV_STATE_NONCE;
    s_total_received  = 0;
    s_fw_bytes_written = 0;
    s_nonce_collected = 0;
    s_sig_collected   = 0;
    memset(s_extracted_sig, 0, sizeof(s_extracted_sig));
}

//*** Signature Accessor ***//

/**
 * @brief  获取传输完成后提取的 ECDSA 签名。
 *
 * @param[out] sig : 输出 64 字节签名（调用者提供缓冲区）。
 * */
void OtaTransport_GetSignature(uint8_t sig[OTA_ECDSA_SIG_SIZE])
{
    memcpy(sig, s_extracted_sig, OTA_ECDSA_SIG_SIZE);
}

//*** UART Adapters for YMODEM (polling mode) ***//

/**
 * @brief  UART 发送单字节，适配 YMODEM 接口。
 * */
void OtaTransport_SendByte(uint8_t byte, void *p_user)
{
    (void)p_user;

    if (s_p_uart_drv != NULL)
    {
        BspUart_SendBlocking(s_p_uart_drv, &byte, 1, 100);
    }
}

/**
 * @brief  UART 接收单字节，适配 YMODEM 接口。
 *
 * @return   0 : 成功。
 * @return  -1 : 超时。
 * */
int OtaTransport_RecvByte(uint8_t *p_byte, uint32_t timeout_ms, void *p_user)
{
    (void)p_user;

    if (s_p_uart_drv != NULL)
    {
        return BspUart_ReadByte(s_p_uart_drv, p_byte, timeout_ms);
    }

    return -1;
}

//*** Internal: Write decrypted firmware to Flash ***//

/**
 * @brief  写入一段解密后的固件数据到 Flash，并打印进度。
 *
 * @param[in] p_data : 解密后的数据。
 * @param[in] len    : 数据长度。
 *
 * @return   0 : 成功。
 * @return  -1 : Flash 写入失败。
 * */
static int WriteDecryptedFirmware(const uint8_t *p_data, uint16_t len)
{
    uint32_t addr;

    addr = s_target_base + s_fw_bytes_written;

    if (BspFlash_Write(addr, p_data, len) != 0)
    {
        log_e("Flash write error at 0x%08lX",
              addr);
        return -1;
    }

    s_fw_bytes_written += len;

    /* Progress logging (based on total received vs expected total) */
    {
        static uint32_t s_last_print = 0;

        if (s_total_received - s_last_print >= 10240u)
        {
            log_i("%lu / %lu bytes (raw)", s_total_received, *s_p_fw_size);
            s_last_print = s_total_received;
        }
    }

    return 0;
}

//*** YMODEM Data Callback ***//

/**
 * @brief  YMODEM 数据接收回调 — 解析加密包格式，流式解密，写入 Flash。
 *
 * Package format: [nonce 12B][encrypted_fw][ECDSA sig 64B]
 *
 * State machine:
 *   RECV_STATE_NONCE     — collect first 12 bytes, init AES-CTR
 *   RECV_STATE_FIRMWARE  — decrypt chunks, write to Flash
 *   RECV_STATE_SIGNATURE — collect last 64 bytes, store signature
 *
 * @param[in] offset : 当前累计接收字节数（含本次）。
 * @param[in] p_data : 本次接收的数据缓冲区。
 * @param[in] len    : 本次数据长度。
 * @param[in] p_user : 用户上下文（未使用）。
 *
 * @return   0 : 成功。
 * @return  -1 : Flash 写入失败。
 * */
int OtaTransport_DataCallback(uint32_t offset, const uint8_t *p_data,
                               uint16_t len, void *p_user)
{
    uint32_t real_fw_size;  /* expected firmware bytes (total - overhead) */
    uint16_t consumed;
    uint16_t to_process;

    (void)p_user;

    s_total_received = offset;

    /* fw_size from YMODEM includes nonce + sig overhead */
    real_fw_size = (*s_p_fw_size > PKG_OVERHEAD) ? (*s_p_fw_size - PKG_OVERHEAD) : 0;

    consumed = 0;

    while (consumed < len)
    {
        switch (s_recv_state)
        {
        case RECV_STATE_NONCE:
        {
            uint16_t need = OTA_AES_NONCE_SIZE - s_nonce_collected;
            to_process = (len - consumed) < need ? (len - consumed) : need;

            memcpy(&s_nonce_buf[s_nonce_collected], &p_data[consumed], to_process);
            s_nonce_collected += to_process;
            consumed          += to_process;

            if (s_nonce_collected >= OTA_AES_NONCE_SIZE)
            {
                /* Nonce complete — initialize AES-256-CTR */
                OtaAes_Init(s_nonce_buf);
                log_i("Nonce extracted, AES initialized.");
                s_recv_state = RECV_STATE_FIRMWARE;
            }
            break;
        }

        case RECV_STATE_FIRMWARE:
        {
            uint32_t fw_remaining = real_fw_size - s_fw_bytes_written;
            to_process = ((uint32_t)(len - consumed) < fw_remaining)
                         ? (len - consumed) : (uint16_t)fw_remaining;

            if (to_process > 0)
            {
                /* Copy to temp buffer for in-place AES decryption */
                if (to_process > sizeof(s_chunk_buf))
                {
                    to_process = sizeof(s_chunk_buf);
                }

                memcpy(s_chunk_buf, &p_data[consumed], to_process);

                /* Decrypt in-place */
                OtaAes_DecryptChunk(s_chunk_buf, to_process);

                /* Write decrypted data to Flash */
                if (WriteDecryptedFirmware(s_chunk_buf, to_process) != 0)
                {
                    return -1;
                }

                consumed += to_process;
            }

            /* Check if all firmware bytes received */
            if (s_fw_bytes_written >= real_fw_size)
            {
                log_i("Firmware complete (%lu bytes decrypted).", s_fw_bytes_written);
                OtaAes_Deinit();
                s_recv_state = RECV_STATE_SIGNATURE;
            }
            break;
        }

        case RECV_STATE_SIGNATURE:
        {
            if (s_sig_collected >= OTA_ECDSA_SIG_SIZE)
            {
                /* Signature already extracted — silently consume remaining bytes */
                consumed = len;
                break;
            }

            uint16_t need = OTA_ECDSA_SIG_SIZE - s_sig_collected;
            to_process = (len - consumed) < need ? (len - consumed) : need;

            memcpy(&s_sig_buf[s_sig_collected], &p_data[consumed], to_process);
            s_sig_collected += to_process;
            consumed         += to_process;

            if (s_sig_collected >= OTA_ECDSA_SIG_SIZE)
            {
                memcpy(s_extracted_sig, s_sig_buf, OTA_ECDSA_SIG_SIZE);
                log_i("Signature extracted (%u bytes).", OTA_ECDSA_SIG_SIZE);
            }
            break;
        }

        default:
            log_e("Invalid receive state!");
            return -1;
        }
    }

    /* Feed watchdog */
    BspWdg_Feed();
    return 0;
}
