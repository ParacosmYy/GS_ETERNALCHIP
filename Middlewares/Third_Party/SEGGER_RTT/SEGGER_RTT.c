#include "SEGGER_RTT.h"

#include <string.h>

#include "stm32f4xx.h"

SEGGER_RTT_CB _SEGGER_RTT;

static char s_up_buffer0[BUFFER_SIZE_UP];
static char s_up_buffer1[BUFFER_SIZE_UP];
static char s_down_buffer0[BUFFER_SIZE_DOWN];

static unsigned char s_initialized;
static unsigned char s_terminal_id;

static uint32_t RTT_Lock(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void RTT_Unlock(uint32_t primask)
{
    __set_PRIMASK(primask);
}

static SEGGER_RTT_BUFFER_UP *GetUpBuffer(unsigned BufferIndex)
{
    if (BufferIndex >= (unsigned)SEGGER_RTT_MAX_NUM_UP_BUFFERS) {
        return NULL;
    }

    return &_SEGGER_RTT.aUp[BufferIndex];
}

static SEGGER_RTT_BUFFER_DOWN *GetDownBuffer(unsigned BufferIndex)
{
    if (BufferIndex >= (unsigned)SEGGER_RTT_MAX_NUM_DOWN_BUFFERS) {
        return NULL;
    }

    return &_SEGGER_RTT.aDown[BufferIndex];
}

static unsigned WriteUpBufferNoLock(SEGGER_RTT_BUFFER_UP *pBuffer,
                                    const void *pData,
                                    unsigned NumBytes)
{
    const unsigned char *pSrc = (const unsigned char *)pData;
    unsigned written = 0;

    if ((pBuffer == NULL) || (pBuffer->pBuffer == NULL) ||
        (pBuffer->SizeOfBuffer < 2u) || (pSrc == NULL)) {
        return 0u;
    }

    while (written < NumBytes) {
        unsigned rdOff = pBuffer->RdOff;
        unsigned wrOff = pBuffer->WrOff;
        unsigned freeSpace;

        if (rdOff <= wrOff) {
            freeSpace = pBuffer->SizeOfBuffer - (wrOff - rdOff) - 1u;
        } else {
            freeSpace = (rdOff - wrOff) - 1u;
        }

        if (freeSpace == 0u) {
            break;
        }

        unsigned chunk = NumBytes - written;
        if (chunk > freeSpace) {
            chunk = freeSpace;
        }

        unsigned firstPart = pBuffer->SizeOfBuffer - wrOff;
        if (firstPart > chunk) {
            firstPart = chunk;
        }

        memcpy(&pBuffer->pBuffer[wrOff], &pSrc[written], firstPart);
        wrOff += firstPart;
        if (wrOff == pBuffer->SizeOfBuffer) {
            wrOff = 0u;
        }
        written += firstPart;
        chunk -= firstPart;

        if (chunk > 0u) {
            memcpy(&pBuffer->pBuffer[wrOff], &pSrc[written], chunk);
            wrOff += chunk;
            if (wrOff == pBuffer->SizeOfBuffer) {
                wrOff = 0u;
            }
            written += chunk;
        }

        pBuffer->WrOff = wrOff;
    }

    return written;
}

static unsigned WriteDownBufferNoLock(SEGGER_RTT_BUFFER_DOWN *pBuffer,
                                      const void *pData,
                                      unsigned NumBytes)
{
    const unsigned char *pSrc = (const unsigned char *)pData;
    unsigned written = 0;

    if ((pBuffer == NULL) || (pBuffer->pBuffer == NULL) ||
        (pBuffer->SizeOfBuffer < 2u) || (pSrc == NULL)) {
        return 0u;
    }

    while (written < NumBytes) {
        unsigned rdOff = pBuffer->RdOff;
        unsigned wrOff = pBuffer->WrOff;
        unsigned freeSpace;

        if (rdOff <= wrOff) {
            freeSpace = pBuffer->SizeOfBuffer - (wrOff - rdOff) - 1u;
        } else {
            freeSpace = (rdOff - wrOff) - 1u;
        }

        if (freeSpace == 0u) {
            break;
        }

        unsigned chunk = NumBytes - written;
        if (chunk > freeSpace) {
            chunk = freeSpace;
        }

        unsigned firstPart = pBuffer->SizeOfBuffer - wrOff;
        if (firstPart > chunk) {
            firstPart = chunk;
        }

        memcpy(&pBuffer->pBuffer[wrOff], &pSrc[written], firstPart);
        wrOff += firstPart;
        if (wrOff == pBuffer->SizeOfBuffer) {
            wrOff = 0u;
        }
        written += firstPart;
        chunk -= firstPart;

        if (chunk > 0u) {
            memcpy(&pBuffer->pBuffer[wrOff], &pSrc[written], chunk);
            wrOff += chunk;
            if (wrOff == pBuffer->SizeOfBuffer) {
                wrOff = 0u;
            }
            written += chunk;
        }

        pBuffer->WrOff = wrOff;
    }

    return written;
}

int SEGGER_RTT_ConfigUpBuffer(unsigned BufferIndex, const char *sName,
                              void *pBuffer, unsigned BufferSize,
                              unsigned Flags)
{
    SEGGER_RTT_BUFFER_UP *pDst = GetUpBuffer(BufferIndex);

    if ((pDst == NULL) || (pBuffer == NULL) || (BufferSize < 2u)) {
        return -1;
    }

    pDst->sName = sName;
    pDst->pBuffer = (char *)pBuffer;
    pDst->SizeOfBuffer = BufferSize;
    pDst->WrOff = 0u;
    pDst->RdOff = 0u;
    pDst->Flags = Flags;
    return 0;
}

int SEGGER_RTT_ConfigDownBuffer(unsigned BufferIndex, const char *sName,
                                void *pBuffer, unsigned BufferSize,
                                unsigned Flags)
{
    SEGGER_RTT_BUFFER_DOWN *pDst = GetDownBuffer(BufferIndex);

    if ((pDst == NULL) || (pBuffer == NULL) || (BufferSize < 2u)) {
        return -1;
    }

    pDst->sName = sName;
    pDst->pBuffer = (char *)pBuffer;
    pDst->SizeOfBuffer = BufferSize;
    pDst->WrOff = 0u;
    pDst->RdOff = 0u;
    pDst->Flags = Flags;
    return 0;
}

void SEGGER_RTT_Init(void)
{
    if (s_initialized != 0u) {
        return;
    }

    memset(&_SEGGER_RTT, 0, sizeof(_SEGGER_RTT));
    memcpy(_SEGGER_RTT.acID, "SEGGER RTT", 10u);
    _SEGGER_RTT.MaxNumUpBuffers = SEGGER_RTT_MAX_NUM_UP_BUFFERS;
    _SEGGER_RTT.MaxNumDownBuffers = SEGGER_RTT_MAX_NUM_DOWN_BUFFERS;

    (void)SEGGER_RTT_ConfigUpBuffer(0u, "Terminal", s_up_buffer0,
                                    sizeof(s_up_buffer0), SEGGER_RTT_MODE_DEFAULT);

    if (SEGGER_RTT_MAX_NUM_UP_BUFFERS > 1) {
        (void)SEGGER_RTT_ConfigUpBuffer(1u, "Terminal 1", s_up_buffer1,
                                        sizeof(s_up_buffer1), SEGGER_RTT_MODE_DEFAULT);
    }

    (void)SEGGER_RTT_ConfigDownBuffer(0u, "Input", s_down_buffer0,
                                      sizeof(s_down_buffer0), SEGGER_RTT_MODE_DEFAULT);

    s_terminal_id = 0u;
    s_initialized = 1u;
}

unsigned SEGGER_RTT_WriteNoLock(unsigned BufferIndex, const void *pBuffer,
                                unsigned NumBytes)
{
    SEGGER_RTT_BUFFER_UP *pDst;

    if (s_initialized == 0u) {
        SEGGER_RTT_Init();
    }

    pDst = GetUpBuffer(BufferIndex);
    if (pDst == NULL) {
        return 0u;
    }

    return WriteUpBufferNoLock(pDst, pBuffer, NumBytes);
}

unsigned SEGGER_RTT_Write(unsigned BufferIndex, const void *pBuffer,
                          unsigned NumBytes)
{
    uint32_t primask = RTT_Lock();
    unsigned written = SEGGER_RTT_WriteNoLock(BufferIndex, pBuffer, NumBytes);
    RTT_Unlock(primask);
    return written;
}

unsigned SEGGER_RTT_WriteStringNoLock(unsigned BufferIndex, const char *s)
{
    if (s == NULL) {
        return 0u;
    }

    return SEGGER_RTT_WriteNoLock(BufferIndex, s, (unsigned)strlen(s));
}

unsigned SEGGER_RTT_WriteString(unsigned BufferIndex, const char *s)
{
    uint32_t primask = RTT_Lock();
    unsigned written = SEGGER_RTT_WriteStringNoLock(BufferIndex, s);
    RTT_Unlock(primask);
    return written;
}

int SEGGER_RTT_SetTerminal(unsigned char TerminalId)
{
    if (TerminalId >= (unsigned char)_SEGGER_RTT.MaxNumUpBuffers) {
        return -1;
    }

    s_terminal_id = TerminalId;
    return 0;
}

int SEGGER_RTT_TerminalOut(unsigned char TerminalId, const char *s)
{
    if (TerminalId >= (unsigned char)_SEGGER_RTT.MaxNumUpBuffers) {
        TerminalId = s_terminal_id;
    }

    return (SEGGER_RTT_WriteString(TerminalId, s) > 0u) ? 0 : -1;
}