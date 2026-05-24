/**
 * @file    ota_trace.c
 * @brief   OTA Flash 故障追踪实现
 * @author  GS_Mark
 *
 * @par dependencies
 * - ota_trace.h
 * - plat_flash.h
 * - plat_sys.h
 * - elog.h
 */

//*** Includes ***//
#define LOG_TAG "TRACE"
#include "elog.h"
#include "ota_trace.h"
#include "plat_flash.h"
#include "plat_sys.h"
#include <string.h>

//*** Private Variables ***//

/** @brief 当前写入索引（下一个空位） */
static uint32_t s_write_index;

//*** Private Helpers ***//

/**
 * @brief  获取事件名称字符串。
 *
 * @param[in] event : 事件类型。
 *
 * @return 事件名称字符串。
 * */
static const char *event_name(uint32_t event)
{
    if (event == OTA_TRACE_APP_START)
    {
        return "APP_START   ";
    }
    if (event == OTA_TRACE_TRIGGER)
    {
        return "TRIGGER     ";
    }
    if (event == OTA_TRACE_ERASE_START)
    {
        return "ERASE_START ";
    }
    if (event == OTA_TRACE_ERASE_DONE)
    {
        return "ERASE_DONE  ";
    }
    if (event == OTA_TRACE_YMODEM_START)
    {
        return "YMODEM_START";
    }
    if (event == OTA_TRACE_YMODEM_DONE)
    {
        return "YMODEM_DONE ";
    }
    if (event == OTA_TRACE_ECDSA_START)
    {
        return "ECDSA_START ";
    }
    if (event == OTA_TRACE_ECDSA_DONE)
    {
        return "ECDSA_DONE  ";
    }
    if (event == OTA_TRACE_SHA256_START)
    {
        return "SHA256_START";
    }
    if (event == OTA_TRACE_SHA256_DONE)
    {
        return "SHA256_DONE ";
    }
    if (event == OTA_TRACE_CFG_WRITE)
    {
        return "CFG_WRITE   ";
    }
    if (event == OTA_TRACE_REBOOT)
    {
        return "REBOOT      ";
    }
    if (event == OTA_TRACE_ERROR)
    {
        return "ERROR       ";
    }

    return "UNKNOWN     ";
}

//*** Public API ***//

/**
 * @brief  初始化追踪模块，找到下一个空位。
 *
 * Steps:
 *  1. 从 Flash 地址 OTA_TRACE_ADDR 开始扫描。
 *  2. 找到第一个 event 字段 == 0xFFFFFFFF 的条目。
 *  3. 记录索引到 s_write_index。
 *
 * */
void OtaTrace_Init(void)
{
    uint32_t i;
    uint32_t addr;

    s_write_index = OTA_TRACE_MAX_ENTRIES;
    addr = OTA_TRACE_ADDR;

    for (i = 0; i < OTA_TRACE_MAX_ENTRIES; i++)
    {
        uint32_t evt;

        BspFlash_Read(addr, (uint8_t *)&evt, sizeof(evt));

        if (evt == 0xFFFFFFFF)
        {
            s_write_index = i;
            break;
        }

        addr += OTA_TRACE_ENTRY_SIZE;
    }
}

/**
 * @brief  记录一条追踪事件到 Flash。
 *
 * Steps:
 *  1. 检查是否已写满。
 *  2. 填充 ota_trace_entry_t 结构。
 *  3. 使用 BspFlash_Write 写入 16 字节。
 *  4. 推进写入索引。
 *
 * @param[in] event  : 事件类型。
 * @param[in] result : 结果（0 = 成功）。
 * @param[in] data   : 附加数据。
 *
 * */
void OtaTrace_Record(uint32_t event, uint32_t result, uint32_t data)
{
    ota_trace_entry_t entry;
    uint32_t          addr;

    if (s_write_index >= OTA_TRACE_MAX_ENTRIES)
    {
        return;
    }

    entry.event     = event;
    entry.timestamp = BspSys_GetTick();
    entry.result    = result;
    entry.data      = data;

    addr = OTA_TRACE_ADDR + s_write_index * OTA_TRACE_ENTRY_SIZE;

    if (BspFlash_Write(addr, (const uint8_t *)&entry, sizeof(entry)) != 0)
    {
        return;
    }

    s_write_index++;
}

/**
 * @brief  通过 elog 打印所有追踪记录。
 *
 * Steps:
 *  1. 遍历 entries 数组。
 *  2. 跳过空条目（event == 0xFFFFFFFF）。
 *  3. 格式化输出每条记录。
 *
 * */
void OtaTrace_PrintAll(void)
{
    uint32_t i;
    uint32_t count;
    uint32_t addr;

    log_i("=== OTA Trace Log ===");

    count = 0;
    addr  = OTA_TRACE_ADDR;

    for (i = 0; i < OTA_TRACE_MAX_ENTRIES; i++)
    {
        ota_trace_entry_t entry;

        BspFlash_Read(addr, (uint8_t *)&entry, sizeof(entry));

        if (entry.event == 0xFFFFFFFF)
        {
            break;
        }

        count++;

        log_i("#%02lu %-14s t=%-6lu %s data=%lu",
              count,
              event_name(entry.event),
              entry.timestamp,
              entry.result == 0 ? "OK " : "ERR",
              entry.data);

        addr += OTA_TRACE_ENTRY_SIZE;
    }

    if (count == 0)
    {
        log_i("(empty)");
    }
    else
    {
        log_i("=== %lu entries ===", count);
    }
}
