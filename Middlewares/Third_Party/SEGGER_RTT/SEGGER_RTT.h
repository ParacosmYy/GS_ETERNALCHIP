#ifndef SEGGER_RTT_H
#define SEGGER_RTT_H

#include "SEGGER_RTT_Conf.h"

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *sName;
    char       *pBuffer;
    unsigned    SizeOfBuffer;
    unsigned    WrOff;
    volatile unsigned RdOff;
    unsigned    Flags;
} SEGGER_RTT_BUFFER_UP;

typedef struct {
    const char *sName;
    char       *pBuffer;
    unsigned    SizeOfBuffer;
    volatile unsigned WrOff;
    unsigned    RdOff;
    unsigned    Flags;
} SEGGER_RTT_BUFFER_DOWN;

typedef struct {
    char                   acID[16];
    int                    MaxNumUpBuffers;
    int                    MaxNumDownBuffers;
    SEGGER_RTT_BUFFER_UP   aUp[SEGGER_RTT_MAX_NUM_UP_BUFFERS];
    SEGGER_RTT_BUFFER_DOWN aDown[SEGGER_RTT_MAX_NUM_DOWN_BUFFERS];
} SEGGER_RTT_CB;

extern SEGGER_RTT_CB _SEGGER_RTT;

void     SEGGER_RTT_Init(void);
int      SEGGER_RTT_ConfigUpBuffer(unsigned BufferIndex, const char *sName,
                                   void *pBuffer, unsigned BufferSize,
                                   unsigned Flags);
int      SEGGER_RTT_ConfigDownBuffer(unsigned BufferIndex, const char *sName,
                                     void *pBuffer, unsigned BufferSize,
                                     unsigned Flags);
unsigned SEGGER_RTT_Write(unsigned BufferIndex, const void *pBuffer,
                          unsigned NumBytes);
unsigned SEGGER_RTT_WriteNoLock(unsigned BufferIndex, const void *pBuffer,
                                unsigned NumBytes);
unsigned SEGGER_RTT_WriteString(unsigned BufferIndex, const char *s);
unsigned SEGGER_RTT_WriteStringNoLock(unsigned BufferIndex, const char *s);
int      SEGGER_RTT_SetTerminal(unsigned char TerminalId);
int      SEGGER_RTT_TerminalOut(unsigned char TerminalId, const char *s);
int      SEGGER_RTT_printf(unsigned BufferIndex, const char *sFormat, ...);
int      SEGGER_RTT_vprintf(unsigned BufferIndex, const char *sFormat,
                            va_list *pParamList);

#ifdef __cplusplus
}
#endif

#endif /* SEGGER_RTT_H */