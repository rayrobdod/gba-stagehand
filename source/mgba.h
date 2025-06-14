#ifndef GUARD_MGBA_H
#define GUARD_MGBA_H

#include <stdint.h>

enum MgbaLogLevel {
	MGBA_LOG_FATAL	= 0,
	MGBA_LOG_ERROR	= 1,
	MGBA_LOG_WARN	= 2,
	MGBA_LOG_INFO	= 3,
	MGBA_LOG_DEBUG	= 4,
};

typedef uint32_t bool32;

bool32 MgbaOpen(void);
void MgbaClose(void);
void MgbaPrintf(enum MgbaLogLevel level, const char*, ...) __attribute__((format(printf, 2, 3)));
void MgbaAssert(const char*, int32_t nLine, const char*, bool32 nStopProgram);


#endif        //  #ifndef GUARD_MGBA_H
