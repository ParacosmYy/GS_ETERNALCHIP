/**
 * @file    ota_transport.c
 * @brief   OTA transport layer implementation
 * @author  GS_Mark
 *
 * @par dependencies
 * - elog.h
 * - ota_transport.h
 * - bsp_flash.h
 * - iwdg.h
 * - usart.h
 *
 * UART byte-level adapters for YMODEM, and data callback that writes
 * received firmware chunks to Slot B Flash with progress logging.
 */

#define LOG_TAG "TRAN"
#include "elog.h"
#include "ota_transport.h"
#include "bsp_flash.h"
#include "iwdg.h"
#include "usart.h"
#include <string.h>

//*** Private Variables ***//

static uint32_t *s_p_fw_size;

//*** Init ***//

/**
 * @brief  初始化 OTA 传输模块，保存固件大小指针供回调使用。
 *
 * @param[in] p_fw_size : 指向任务级固件大小变量（回调中用于进度日志）。
 * */
void OtaTransport_Init(uint32_t *p_fw_size)
{
    s_p_fw_size = p_fw_size;
}

//*** UART Adapters for YMODEM (polling mode) ***//

/**
 * @brief  UART 发送单字节，适配 YMODEM 接口。
 *
 * @param[in] byte   : 待发送字节。
 * @param[in] p_user : 用户上下文（未使用）。
 * */
void OtaTransport_SendByte(uint8_t byte, void *p_user)
{
    (void)p_user;
    HAL_UART_Transmit(&huart1, &byte, 1, 100);
}

/**
 * @brief  UART 接收单字节，适配 YMODEM 接口。
 *
 * @param[out] p_byte     : 接收字节存放地址。
 * @param[in]  timeout_ms : 接收超时（毫秒）。
 * @param[in]  p_user     : 用户上下文（未使用）。
 *
 * @return   0 : 成功。
 * @return  -1 : 超时。
 * */
int OtaTransport_RecvByte(uint8_t *p_byte, uint32_t timeout_ms, void *p_user)
{
    (void)p_user;

    if (HAL_UART_Receive(&huart1, p_byte, 1, timeout_ms) == HAL_OK)
    {
        return 0;
    }

    return -1;
}

//*** YMODEM Data Callback ***//

/**
 * @brief  YMODEM 数据接收回调，将固件数据写入 Slot B Flash。
 *
 * Steps:
 *  1. 计算目标 Flash 地址（基地址 + 偏移 - 本次长度）。
 *  2. 调用 BspFlash_Write 写入数据。
 *  3. 每 10KB 打印一次进度日志。
 *  4. 喂看门狗防止传输过程中复位。
 *
 * @param[in] offset : 当前累计接收字节数。
 * @param[in] p_data : 本次接收的数据缓冲区。
 * @param[in] len    : 本次数据长度。
 * @param[in] p_user : 用户上下文（未使用）。
 *
 * @return   0 : 写入成功。
 * @return  -1 : Flash 写入失败。
 * */
int OtaTransport_DataCallback(uint32_t offset, const uint8_t *p_data,
                               uint16_t len, void *p_user)
{
    uint32_t addr;

    (void)p_user;

    /* offset 是累计值（含本次），减去 len 得到本次写入的起始地址 */
    addr = FLASH_ADDR_SLOT_B + offset - len;

    if (BspFlash_Write(addr, p_data, len) != 0)
    {
        log_e("Flash write error at 0x%08lX (HAL err=0x%08lX)",
              addr, HAL_FLASH_GetError());
        return -1;
    }

    /* 每 10KB 打印一次进度，避免日志过于频繁 */
    {
        static uint32_t s_last_print = 0;

        if (offset - s_last_print >= 10240u)
        {
            log_i("%lu / %lu bytes", offset, *s_p_fw_size);
            s_last_print = offset;
        }
    }

    /* 每次写入后喂狗，确保长传输不触发看门狗 */
    HAL_IWDG_Refresh(&hiwdg);
    return 0;
}
