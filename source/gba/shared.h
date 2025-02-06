#ifndef GUARD_GBA_SHARED_H
#define GUARD_GBA_SHARED_H

#include <stdint.h>

typedef struct {
	uint16_t r: 5;
	uint16_t g: 5;
	uint16_t b: 5;
} rgb_t;

__attribute__((const)) inline static rgb_t rgb(uint16_t r, uint16_t g, uint16_t b) {
	rgb_t retval = {
		.r = r,
		.g = g,
		.b = b,
	};
	return retval;
}

typedef enum {
	PAL_MODE_4BPP = 0,
	PAL_MODE_8BPP = 1,
} palette_mode_t;

#endif
