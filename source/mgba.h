#ifndef GUARD_MGBA_H
#define GUARD_MGBA_H

#include <stdint.h>

#define MGBA_LOG_FATAL  (0)
#define MGBA_LOG_ERROR  (1)
#define MGBA_LOG_WARN   (2)
#define MGBA_LOG_INFO   (3)
#define MGBA_LOG_DEBUG  (4)

typedef uint32_t bool32;

bool32 MgbaOpen(void);
void MgbaClose(void);
void MgbaPrintf(int32_t level, const char*, ...);
void MgbaAssert(const char*, int32_t nLine, const char*, bool32 nStopProgram);


#endif        //  #ifndef GUARD_MGBA_H
