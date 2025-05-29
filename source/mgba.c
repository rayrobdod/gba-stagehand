#include "mgba.h"
#include <stdarg.h>
#include <stdio.h>

// hardware extensions for LOG_HANDLER_MGBA_PRINT
#define REG_DEBUG_ENABLE ((volatile uint16_t*) (0x4FFF780)) // handshake: (w)[0xC0DE] -> (r)[0x1DEA]
#define REG_DEBUG_FLAGS  ((volatile uint16_t*) (0x4FFF700))
#define REG_DEBUG_STRING ((char*) (0x4FFF600))

#define MGBA_REG_DEBUG_MAX (256)

bool32 MgbaOpen(void)
{
    *REG_DEBUG_ENABLE = 0xC0DE;
    return *REG_DEBUG_ENABLE == 0x1DEA;
}

void MgbaClose(void)
{
    *REG_DEBUG_ENABLE = 0;
}

void MgbaPrintf(enum MgbaLogLevel level, const char* ptr, ...)
{
    va_list args;

    level &= 0x7;
    va_start(args, ptr);
    vsnprintf(REG_DEBUG_STRING, MGBA_REG_DEBUG_MAX, ptr, args);
    va_end(args);
    *REG_DEBUG_FLAGS = level | 0x100;
}

void MgbaAssert(const char *pFile, int32_t nLine, const char *pExpression, bool32 nStopProgram)
{
    if (nStopProgram)
    {
        MgbaPrintf(MGBA_LOG_ERROR, "ASSERTION FAILED  FILE=[%s] LINE=[%d]  EXP=[%s]", pFile, nLine, pExpression);
        asm(".hword 0xEFFF");
    }
    else
    {
        MgbaPrintf(MGBA_LOG_WARN, "WARING FILE=[%s] LINE=[%d]  EXP=[%s]", pFile, nLine, pExpression);
    }
}
