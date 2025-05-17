#ifndef SHADOW_VRAM_H
#define SHADOW_VRAM_H

#include <stdbool.h>
#include <stdint.h>
#include "gba/vram.h"

struct background;

typedef uint8_t shadow_vram_tiles_id_t;

enum {
	BG_CHARBLOCK_OVERLAP_NONE,
	BG_CHARBLOCK_OVERLAP_01,
	BG_CHARBLOCK_OVERLAP_02,
	BG_CHARBLOCK_OVERLAP_03,
	BG_CHARBLOCK_OVERLAP_12,
	BG_CHARBLOCK_OVERLAP_13,
	BG_CHARBLOCK_OVERLAP_23,
	BG_CHARBLOCK_OVERLAP_012,
	BG_CHARBLOCK_OVERLAP_013,
	BG_CHARBLOCK_OVERLAP_023,
	BG_CHARBLOCK_OVERLAP_123,
	BG_CHARBLOCK_OVERLAP_1234,
};

void shadow_vram_init(void);
void shadow_vram_free_all(void);

int shadow_tiles_load_tileset(unsigned bg, unsigned count, const tile_4bpp_t*);

struct shadow_tiles_load_background {
	unsigned bg;
};
bool shadow_tiles_load_background(struct background*, struct shadow_tiles_load_background);

#endif        //  #ifndef SHADOW_VRAM_H
