/**
 * @file    ymodem.h
 * @brief   YMODEM protocol receiver — hardware-independent
 *
 * Usage:
 *   Call Ymodem_Receive() with byte-level I/O callbacks.
 *   The library has no hardware dependencies.
 */

#ifndef YMODEM_H
#define YMODEM_H

#include <stdint.h>

//*** Protocol Constants ***//

#define YMODEM_SOH         0x01
#define YMODEM_STX         0x02
#define YMODEM_EOT         0x04
#define YMODEM_ACK         0x06
#define YMODEM_NAK         0x15
#define YMODEM_CAN         0x18
#define YMODEM_CRC16_REQ   0x43   /* 'C' */

#define YMODEM_TIMEOUT_MS  5000
#define YMODEM_MAX_ERRORS  10

//*** Error Codes ***//

typedef enum {
    YMODEM_OK             =  0,
    YMODEM_ERR_TIMEOUT    = -1,
    YMODEM_ERR_CRC        = -2,
    YMODEM_ERR_ABORT      = -3,
    YMODEM_ERR_TOO_LARGE  = -4,
    YMODEM_ERR_WRITE_FAIL = -5,
    YMODEM_ERR_UNKNOWN    = -6,
} ymodem_err_t;

//*** Callback Types ***//

/* Called for each received data block */
typedef int (*ymodem_data_cb_t)(uint32_t offset, const uint8_t *p_data,
                                uint16_t len, void *p_user);

/* Send one byte (blocking). Provided by caller. */
typedef void (*ymodem_send_byte_t)(uint8_t byte, void *p_user);

/* Receive one byte with timeout. Return 0 on success, -1 on timeout. */
typedef int  (*ymodem_recv_byte_t)(uint8_t *p_byte, uint32_t timeout_ms, void *p_user);

//*** Public API ***//

/**
 * @brief  Receive a file via YMODEM protocol.
 * @param  send_byte  Callback to send one byte
 * @param  recv_byte  Callback to receive one byte (with timeout)
 * @param  p_filename Output: received filename (may be NULL)
 * @param  p_filesize Output: received file size in bytes (may be NULL)
 * @param  max_size   Maximum allowed file size
 * @param  on_data    Callback for each data block received
 * @param  p_user     User context passed to all callbacks
 * @return YMODEM_OK or negative error code
 */
int Ymodem_Receive(ymodem_send_byte_t  send_byte,
                   ymodem_recv_byte_t  recv_byte,
                   char               *p_filename,
                   uint32_t           *p_filesize,
                   uint32_t            max_size,
                   ymodem_data_cb_t    on_data,
                   void               *p_user);

#endif /* YMODEM_H */
