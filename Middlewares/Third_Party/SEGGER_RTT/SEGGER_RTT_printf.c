#include "SEGGER_RTT.h"

#include <stdio.h>

int SEGGER_RTT_vprintf(unsigned BufferIndex, const char *sFormat,
                       va_list *pParamList)
{
    char acBuffer[SEGGER_RTT_PRINTF_BUFFER_SIZE];
    int  len;

    if ((sFormat == NULL) || (pParamList == NULL)) {
        return -1;
    }

    len = vsnprintf(acBuffer, sizeof(acBuffer), sFormat, *pParamList);
    if (len < 0) {
        return -1;
    }

    if ((unsigned)len >= sizeof(acBuffer)) {
        len = (int)(sizeof(acBuffer) - 1u);
    }

    return (int)SEGGER_RTT_Write(BufferIndex, acBuffer, (unsigned)len);
}

int SEGGER_RTT_printf(unsigned BufferIndex, const char *sFormat, ...)
{
    va_list ParamList;
    int     Result;

    va_start(ParamList, sFormat);
    Result = SEGGER_RTT_vprintf(BufferIndex, sFormat, &ParamList);
    va_end(ParamList);

    return Result;
}