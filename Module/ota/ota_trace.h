/**
 * @file    ota_trace.h
 * @brief   OTA Flash 故障追踪 — 记录 OTA 各阶段事件到 Flash
 * @author  GS_Mark
 *
 * @par dependencies
 * - ota_config.h
 * - stdint.h
 *
 * 追踪数据存储在 Sector 1 的 ota_config_t 之后（偏移 0x100），
 * 每条 16 字节，最多 32 条。写入使用 BspFlash_Write 追加到 0xFF
 * 位置，无需擦除。OTA Config 更新时整个 Sector 1 擦除，追踪自动清除。
 *
 * @version V1.0 2025-05-22
 *
 * @note 1 tab == 4 spaces
 *
 * */
#ifndef OTA_TRACE_H
#define OTA_TRACE_H

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include "ota_config.h"
#include <stdint.h>
//******************************* Includes **********************************//

//******************************* Defines ***********************************//

/** @brief 追踪区每条记录大小（字节） */
#define OTA_TRACE_ENTRY_SIZE  16u

//******************************* Types *************************************//

/** @brief OTA 追踪事件类型 */
typedef enum
{
    OTA_TRACE_APP_START     = 0x01,  /**< App 启动 */
    OTA_TRACE_TRIGGER       = 0x02,  /**< 触发（按键/远程） */
    OTA_TRACE_ERASE_START   = 0x03,  /**< 开始擦除 Slot */
    OTA_TRACE_ERASE_DONE    = 0x04,  /**< 擦除完成 */
    OTA_TRACE_YMODEM_START  = 0x05,  /**< 开始 YMODEM 接收 */
    OTA_TRACE_YMODEM_DONE   = 0x06,  /**< YMODEM 接收完成 */
    OTA_TRACE_ECDSA_START   = 0x07,  /**< 开始 ECDSA 验签 */
    OTA_TRACE_ECDSA_DONE    = 0x08,  /**< ECDSA 验签完成 */
    OTA_TRACE_SHA256_START  = 0x09,  /**< 开始 SHA-256 校验 */
    OTA_TRACE_SHA256_DONE   = 0x0A,  /**< SHA-256 校验完成 */
    OTA_TRACE_CFG_WRITE     = 0x0B,  /**< 写入 OTA Config */
    OTA_TRACE_REBOOT        = 0x0C,  /**< 即将重启 */
    OTA_TRACE_ERROR         = 0xFF,  /**< 错误，result = 错误码 */
} ota_trace_event_t;

/* ota_trace_entry_t 定义在 ota_config.h（Boot/APP 共用） */

//******************************* Declaring *********************************//

/**
 * @brief  初始化追踪模块，找到下一个空位。
 *
 * Steps:
 *  1. 扫描 entries 数组找到第一个 event == 0xFFFFFFFF 的位置。
 *  2. 记录到静态变量供 OtaTrace_Record 使用。
 *
 * */
void OtaTrace_Init(void);

/**
 * @brief  记录一条追踪事件到 Flash。
 *
 * Steps:
 *  1. 检查是否已写满。
 *  2. 使用 BspFlash_Write 写入 16 字节到当前位置。
 *  3. 推进写入位置。
 *
 * @param[in] event  : 事件类型。
 * @param[in] result : 结果（0 = 成功）。
 * @param[in] data   : 附加数据。
 *
 * */
void OtaTrace_Record(uint32_t event, uint32_t result, uint32_t data);

/**
 * @brief  通过 elog 打印所有追踪记录。
 *
 * Steps:
 *  1. 遍历 entries 数组。
 *  2. 格式化输出每条非空记录（序号、事件名、时间戳、结果、数据）。
 *
 * */
void OtaTrace_PrintAll(void);

#ifdef __cplusplus
}
#endif

#endif /* OTA_TRACE_H */
