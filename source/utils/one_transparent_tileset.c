#include "one_transparent_tileset.h"

#include <stddef.h>
#include "decompress/type.h"
#include "gba/vram.h"

static const struct CompressedData transparent_tile = {
	.magic = 0,
	.size = sizeof(tile_4bpp_t),
	.data = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

const struct tileset one_transparent_tileset = {
	.palette = NULL,
	.palette_count = 0,
	.paltag = 1,
	.tileset = &transparent_tile,
	.tileset_count = 1,
	.tiletag = 1,
};
