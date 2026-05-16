/**
 * @file    ymodem.c
 * @brief   YMODEM protocol receiver implementation
 *
 * Supports 128-byte (SOH) and 1024-byte (STX) packets with CRC-16.
 * Uses send_byte / recv_byte callbacks — no hardware dependencies.
 */

#include "ymodem.h"
#include <string.h>

//*** Private: CRC-16-CCITT ***//

static uint16_t calc_crc16(const uint8_t *p_data, uint16_t len)
{
    uint16_t crc = 0;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)p_data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

//*** Private: Receive helpers ***//

static void send_byte_wrapper(ymodem_send_byte_t send, uint8_t byte, void *user)
{
    send(byte, user);
}

/* Wait for one byte with timeout */
static int recv_byte_wrapper(ymodem_recv_byte_t recv, uint8_t *p_byte,
                             uint32_t timeout_ms, void *user)
{
    return recv(p_byte, timeout_ms, user);
}

/* Receive a complete packet (SOH/STX + seq + ~seq + data + CRC16)
 * Returns: 1 = got packet, 0 = got CAN/EOT/timeout, -1 = error
 * p_len: set to data length (128 or 1024) on success */
static int recv_packet(ymodem_recv_byte_t recv, uint8_t *p_buf,
                       uint16_t *p_len, uint32_t timeout_ms, void *user)
{
    uint8_t header;

    if (recv_byte_wrapper(recv, &header, timeout_ms, user) != 0)
        return 0;   /* timeout */

    if (header == YMODEM_CAN) {
        /* Double CAN = abort */
        uint8_t second;
        if (recv_byte_wrapper(recv, &second, YMODEM_TIMEOUT_MS, user) == 0
            && second == YMODEM_CAN) {
            return -1;
        }
        return 0;
    }

    if (header == YMODEM_EOT) {
        return 0;
    }

    uint16_t data_len;
    if (header == YMODEM_SOH) {
        data_len = 128;
    } else if (header == YMODEM_STX) {
        data_len = 1024;
    } else {
        return -1;  /* unexpected */
    }

    uint8_t seq, seq_compl;
    if (recv_byte_wrapper(recv, &seq, YMODEM_TIMEOUT_MS, user) != 0) return -1;
    if (recv_byte_wrapper(recv, &seq_compl, YMODEM_TIMEOUT_MS, user) != 0) return -1;

    if ((uint8_t)(seq + seq_compl) != 0xFF) return -1;

    for (uint16_t i = 0; i < data_len; i++) {
        if (recv_byte_wrapper(recv, &p_buf[i], YMODEM_TIMEOUT_MS, user) != 0)
            return -1;
    }

    uint8_t crc_hi, crc_lo;
    if (recv_byte_wrapper(recv, &crc_hi, YMODEM_TIMEOUT_MS, user) != 0) return -1;
    if (recv_byte_wrapper(recv, &crc_lo, YMODEM_TIMEOUT_MS, user) != 0) return -1;

    uint16_t recv_crc = ((uint16_t)crc_hi << 8) | crc_lo;
    uint16_t calc_crc = calc_crc16(p_buf, data_len);

    if (recv_crc != calc_crc) return -1;

    *p_len = data_len;
    return 1;
}

//*** Public API ***//

int Ymodem_Receive(ymodem_send_byte_t  send_byte,
                   ymodem_recv_byte_t  recv_byte,
                   char               *p_filename,
                   uint32_t           *p_filesize,
                   uint32_t            max_size,
                   ymodem_data_cb_t    on_data,
                   void               *p_user)
{
    uint8_t  buf[1024];
    uint16_t pkt_len;
    uint8_t  seq_expected = 0;
    uint8_t  error_count  = 0;
    uint32_t total_received = 0;
    uint32_t file_size = 0;
    int      result = YMODEM_ERR_UNKNOWN;
    int      phase = 0;  /* 0 = waiting packet 0, 1 = data phase */

    if (p_filename) p_filename[0] = '\0';
    if (p_filesize) *p_filesize = 0;

    /* --- Phase 0: Receive packet 0 (file info) --- */
    while (error_count < YMODEM_MAX_ERRORS) {
        send_byte_wrapper(send_byte, YMODEM_CRC16_REQ, p_user);

        int ret = recv_packet(recv_byte, buf, &pkt_len, YMODEM_TIMEOUT_MS, p_user);

        if (ret == 1) {
            /* Got a packet — check if it's packet 0 */
            if (buf[0] == 0x00) {
                /* Packet 0: first byte is 0 = end session batch */
                send_byte_wrapper(send_byte, YMODEM_ACK, p_user);
                result = YMODEM_OK;
                goto done;
            }

            /* Parse filename */
            if (p_filename) {
                uint16_t fn_len = 0;
                while (fn_len < pkt_len && buf[fn_len] != '\0' && fn_len < 255)
                    fn_len++;
                if (fn_len > 0) {
                    memcpy(p_filename, buf, fn_len);
                    p_filename[fn_len] = '\0';
                }
            }

            /* Parse file size (after filename null terminator) */
            {
                uint16_t idx = 0;
                while (idx < pkt_len && buf[idx] != '\0') idx++;
                idx++;  /* skip null */
                if (idx < pkt_len) {
                    /* Parse decimal number */
                    file_size = 0;
                    while (idx < pkt_len && buf[idx] >= '0' && buf[idx] <= '9') {
                        file_size = file_size * 10 + (buf[idx] - '0');
                        idx++;
                    }
                }
            }

            if (file_size > max_size) {
                send_byte_wrapper(send_byte, YMODEM_CAN, p_user);
                send_byte_wrapper(send_byte, YMODEM_CAN, p_user);
                return YMODEM_ERR_TOO_LARGE;
            }

            send_byte_wrapper(send_byte, YMODEM_ACK, p_user);
            send_byte_wrapper(send_byte, YMODEM_CRC16_REQ, p_user);

            seq_expected = 1;
            phase = 1;
            break;
        }
        else if (ret == -1) {
            /* CAN received */
            return YMODEM_ERR_ABORT;
        }

        error_count++;
    }

    if (phase == 0) {
        return YMODEM_ERR_TIMEOUT;
    }

    /* --- Phase 1: Receive data packets --- */
    while (error_count < YMODEM_MAX_ERRORS) {
        int ret = recv_packet(recv_byte, buf, &pkt_len, YMODEM_TIMEOUT_MS, p_user);

        if (ret == 1) {
            error_count = 0;

            /* Check for end-of-session batch (seq 0, data all zeros) */
            int is_end_batch = 1;
            for (uint16_t i = 0; i < pkt_len; i++) {
                if (buf[i] != 0x00) { is_end_batch = 0; break; }
            }
            if (is_end_batch) {
                send_byte_wrapper(send_byte, YMODEM_ACK, p_user);
                result = YMODEM_OK;
                goto done;
            }

            /* Normal data packet */
            uint32_t write_len = pkt_len;
            if (total_received + write_len > file_size) {
                write_len = file_size - total_received;
            }

            if (write_len > 0 && on_data) {
                if (on_data(total_received + write_len, buf,
                            (uint16_t)write_len, p_user) != 0) {
                    send_byte_wrapper(send_byte, YMODEM_CAN, p_user);
                    send_byte_wrapper(send_byte, YMODEM_CAN, p_user);
                    return YMODEM_ERR_WRITE_FAIL;
                }
            }

            total_received += write_len;
            send_byte_wrapper(send_byte, YMODEM_ACK, p_user);
            seq_expected++;

            if (total_received >= file_size) {
                /* All data received, wait for EOT */
                /* Fall through to EOT handling below */
                break;
            }

            continue;
        }
        else if (ret == 0) {
            /* Could be EOT */
            uint8_t byte;
            if (recv_byte_wrapper(recv_byte, &byte, YMODEM_TIMEOUT_MS, p_user) == 0
                && byte == YMODEM_EOT) {
                send_byte_wrapper(send_byte, YMODEM_NAK, p_user);
                /* Wait for second EOT */
                if (recv_byte_wrapper(recv_byte, &byte, YMODEM_TIMEOUT_MS, p_user) == 0
                    && byte == YMODEM_EOT) {
                    send_byte_wrapper(send_byte, YMODEM_ACK, p_user);
                    /* Wait for end batch */
                    send_byte_wrapper(send_byte, YMODEM_CRC16_REQ, p_user);
                    continue;
                }
            }
            /* Timeout or unexpected */
            error_count++;
            continue;
        }
        else {
            /* CAN or error */
            send_byte_wrapper(send_byte, YMODEM_NAK, p_user);
            error_count++;
            continue;
        }
    }

    /* If we broke out of the loop because all data received,
     * wait for EOT sequence */
    while (error_count < YMODEM_MAX_ERRORS) {
        uint8_t byte;
        if (recv_byte_wrapper(recv_byte, &byte, YMODEM_TIMEOUT_MS, p_user) != 0) {
            error_count++;
            continue;
        }

        if (byte == YMODEM_EOT) {
            send_byte_wrapper(send_byte, YMODEM_NAK, p_user);
            if (recv_byte_wrapper(recv_byte, &byte, YMODEM_TIMEOUT_MS, p_user) == 0
                && byte == YMODEM_EOT) {
                send_byte_wrapper(send_byte, YMODEM_ACK, p_user);
                send_byte_wrapper(send_byte, YMODEM_CRC16_REQ, p_user);
                /* Wait for end batch */
                {
                    int r = recv_packet(recv_byte, buf, &pkt_len,
                                        YMODEM_TIMEOUT_MS, p_user);
                    if (r == 1) {
                        send_byte_wrapper(send_byte, YMODEM_ACK, p_user);
                        result = YMODEM_OK;
                        goto done;
                    }
                }
            }
        }
        error_count++;
    }

    if (error_count >= YMODEM_MAX_ERRORS) {
        result = YMODEM_ERR_TIMEOUT;
    }

done:
    if (p_filesize) *p_filesize = file_size;
    return result;
}
