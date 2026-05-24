/**
 * @file    bsp_rtt.c
 * @brief   Thin wrapper around SEGGER RTT for debug logging
 * @author  GS_Mark
 *
 * @par dependencies
 * - bsp_rtt.h
 * - SEGGER_RTT.h
 */

#include "bsp_rtt.h"

#include "SEGGER_RTT.h"

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

//*** Private Variables ***//

static uint8_t s_rtt_ready;

//*** Public API ***//

/**
 * @brief  初始化 SEGGER RTT（仅首次调用生效）。
 * */
void BspRtt_Init(void)
{
    if (s_rtt_ready != 0u)
    {
        return;
    }

    SEGGER_RTT_Init();
    s_rtt_ready = 1u;
}

/**
 * @brief  通过 RTT 写入字符串（通道 0）。
 *
 * @param[in] s : 待写入的字符串。
 *
 * @return  >=0 : 写入字节数。
 * @return  -1  : 参数为 NULL。
 * */
int BspRtt_WriteString(const char *s)
{
    if (s == NULL)
    {
        return -1;
    }

    BspRtt_Init();
    return (int)SEGGER_RTT_WriteString(0u, s);
}

/**
 * @brief  通过 RTT 格式化输出（通道 0）。
 *
 * @param[in] fmt : 格式化字符串。
 * @param[in] ... : 可变参数。
 *
 * @return  >=0 : 写入字节数。
 * @return  -1  : 参数为 NULL。
 * */
int BspRtt_Printf(const char *fmt, ...)
{
    va_list args;
    int     result;

    if (fmt == NULL)
    {
        return -1;
    }

    BspRtt_Init();

    va_start(args, fmt);
    result = SEGGER_RTT_vprintf(0u, fmt, &args);
    va_end(args);

    return result;
}
