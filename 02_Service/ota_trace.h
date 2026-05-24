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

//******************************* 头文件 ************************************//
#include "ota_config.h"
#include <stdint.h>
//******************************* 头文件 ************************************//

//******************************* 类型 **************************************//

/** @brief SYS 驱动前向声明 */
typedef struct bsp_sys_driver bsp_sys_driver_t;

//******************************* 宏定义 ************************************//

/** @brief 追踪区每条记录大小（字节） */
#define OTA_TRACE_ENTRY_SIZE  16u

//******************************* 类型 **************************************//

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

//******************************* 函数声明 **********************************//

/**
 * @brief  初始化追踪模块，注入 SYS 驱动用于获取时间戳。
 *
 * Steps:
 *  1. 保存 SYS 驱动指针。
 *  2. 扫描 entries 数组找到第一个 event == 0xFFFFFFFF 的位置。
 *  3. 记录到静态变量供 OtaTrace_Record 使用。
 *
 * @param[in] p_sys_drv : SYS 驱动实例指针。
 *
 * */
void OtaTrace_Init(bsp_sys_driver_t *p_sys_drv);

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

/**
 * @brief  清除所有追踪记录（擦除 trace 区域为 0xFF）。
 *
 * Steps:
 *  1. 读取 Sector 1 全部有效数据到静态缓冲区。
 *  2. 将 trace 部分填充 0xFF。
 *  3. 擦除 Sector 1，写回全部数据。
 *  4. 重置写入索引为 0。
 *
 * @note   保留 Config 和 Crash Dump 不变。
 *
 * */
void OtaTrace_Clear(void);

#ifdef __cplusplus
}
#endif

#endif /* OTA_TRACE_H */
