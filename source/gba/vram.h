#ifndef GUARD_GBA_VRAM_H
#define GUARD_GBA_VRAM_H

#include <stdint.h>
#include <stdbool.h>
#include "gba/shared.h"

typedef struct {
	uint16_t tile : 10;
	bool hflip : 1;
	bool vflip : 1;
	uint16_t palette : 4;
} bg_tile_t;

typedef uint16_t tile_4bpp_t[8 * 8 * 4 / 16];

typedef bg_tile_t screenblock_t[0x400];
typedef tile_4bpp_t charblock_t[0x200];

typedef union {
	screenblock_t screenblock[32];
	struct {
		charblock_t bg_charblock[4];
		charblock_t obj_charblock[2];
	};

	rgb_t mode3[160][240];
} vram_t;

extern volatile vram_t vram;

#endif
