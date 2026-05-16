/**
 * @file    bsp_rtt.c
 * @brief   Thin wrapper around SEGGER RTT for debug logging
 */

#include "bsp_rtt.h"

#include "SEGGER_RTT.h"

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

static uint8_t s_rtt_ready;

void BspRtt_Init(void)
{
    if (s_rtt_ready != 0u)
    {
        return;
    }

    SEGGER_RTT_Init();
    s_rtt_ready = 1u;
}

int BspRtt_WriteString(const char *s)
{
    if (s == NULL)
    {
        return -1;
    }

    BspRtt_Init();
    return (int)SEGGER_RTT_WriteString(0u, s);
}

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
