#include "shadow_vram.h"

#include <stdlib.h>
#include "gba/bios.h"
#include "gba/hw_reg.h"
#include "gba/vram.h"
#include "utils/arraycount.h"
#include "utils/minmax.h"
#include "graphics_types.h"
#include "mgba.h"
#include "vram_op_queue.h"

// The main thing I want is to have "windows" that I can print text to
// without having to predetermine where in VRAM the needed tiles are.

static const unsigned TILES_PER_SCREENBLOCK = sizeof(screenblock_t) / sizeof(tile_4bpp_t);
static const unsigned TILES_PER_CHARBLOCK = sizeof(charblock_t) / sizeof(tile_4bpp_t);
_Static_assert(0x200 == sizeof(charblock_t) / sizeof(tile_4bpp_t), "TILES_PER_CHARBLOCK");

static const struct {
	uint8_t screenblocks;
} bgsize_properties[4] = {
	[0] = {
		.screenblocks = 1,
	},
	[1] = {
		.screenblocks = 2,
	},
	[2] = {
		.screenblocks = 2,
	},
	[3] = {
		.screenblocks = 4,
	},
};


__attribute__((aligned(4)))
__attribute__((section(".sbss.shadow_vram.tiles_used")))
static bool shadow_tiles_used[4 * 0x200] = {0};

__attribute__((aligned(4)))
__attribute__((section(".sbss.shadow_vram.hwreg")))
static bgcnt_t shadow_tiles_bgcnt[4] = {0};

__attribute__((aligned(4)))
__attribute__((section(".sbss.shadow_vram.windows")))
static struct {
	bool in_use;
	uint16_t bg_tile_start;
	struct shadow_tiles_window_allocate args;
} windows[16] = {0};


static inline void shadow_vram_reserve_screenblock(bgcnt_t cnt) {
	MgbaPrintf(MGBA_LOG_DEBUG, "ENTER shadow_vram_reserve_screenblock");
	MgbaPrintf(MGBA_LOG_DEBUG, "  cnt.charblock = %d",  cnt.charblock);
	MgbaPrintf(MGBA_LOG_DEBUG, "  cnt.screenblock = %d",  cnt.screenblock);
	MgbaPrintf(MGBA_LOG_DEBUG, "  cnt.size = %d",  cnt.size);
	const uint32_t one = 0x02020202;

	_Static_assert(0 == (TILES_PER_SCREENBLOCK % 32), "CpuFastSet requires 32-byte-multiple length");
	CpuFastSet(
		&one,
		shadow_tiles_used + cnt.screenblock * TILES_PER_SCREENBLOCK,
		(struct CpuFastSet) {
			.word_count = TILES_PER_SCREENBLOCK * bgsize_properties[cnt.size].screenblocks / sizeof(uint32_t),
			.mode = CPU_SET_FILL
		}
	);
}

void shadow_vram_init(const struct shadow_vram_init* args) {
	shadow_vram_free_all();

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_HWREG_DISPCNT,
		.dispcnt = {
			.value = (dispcnt_t) {
				.mode = 0,
				.obj_character_mapping = OBJ_CHAR_MAP_1D,
				.enable_bg0 = args->enable_bg[0],
				.enable_bg1 = args->enable_bg[1],
				.enable_bg2 = args->enable_bg[2],
				.enable_bg3 = args->enable_bg[3],
				.enable_obj = args->enable_obj,
			}
		}
	});

	for (int i = 0; i < 4; i++) {
		if (args->enable_bg[i]) {
			shadow_vram_reserve_screenblock(args->bgcnt[i]);
			vram_op_queue_enqueue((struct vram_op) {
				.type = VRAM_QUEUE_OP_HWREG_BGCNT,
				.bgcnt = {
					.value = args->bgcnt[i],
					.to_index = i,
				}
			});
			shadow_tiles_bgcnt[i] = args->bgcnt[i];
		} else {
			shadow_tiles_bgcnt[i] = (bgcnt_t) {0};
		}
	}
}

void shadow_vram_free_all(void) {
	for (unsigned i = 0; i < arraycount(windows); i++) {
		windows[i].in_use = false;
	}
	const uint32_t zero = 0;
	_Static_assert(0 == (sizeof(shadow_tiles_used) % 32), "CpuFastSet requires 32-byte-multiple length");
	CpuFastSet(&zero, shadow_tiles_used, (struct CpuFastSet) {
		.word_count = sizeof(shadow_tiles_used) / sizeof(uint32_t),
		.mode = CPU_SET_FILL
	});
}

static int shadow_tiles_allocate(unsigned bg, unsigned tilecount) {
	unsigned charblock = shadow_tiles_bgcnt[bg].charblock;
	unsigned start = charblock * TILES_PER_CHARBLOCK;
	const unsigned max = min(4 * TILES_PER_CHARBLOCK, start + 2 * TILES_PER_CHARBLOCK);
	unsigned span = 0;

	while (start + span < max) {
		if (shadow_tiles_used[start + span]) {
			start += span + 1;
			span = 0;
			continue;
		}
		span++;
		if (span >= tilecount) {
			for (unsigned i = start; i < start + tilecount; i++) {
				shadow_tiles_used[i] = true;
			}
			return start - charblock * TILES_PER_CHARBLOCK;
		}
	}
	return -1;
}

static void shadow_tiles_deallocate(unsigned bg, unsigned bg_tile_start, unsigned tilecount) {
	unsigned charblock = shadow_tiles_bgcnt[bg].charblock;
	unsigned start = charblock * TILES_PER_CHARBLOCK + bg_tile_start;

	for (unsigned i = 0; i < tilecount; i++) {
		shadow_tiles_used[start + i] = false;
	}
}

window_id_t shadow_tiles_window_allocate(const struct shadow_tiles_window_allocate* args) {
	window_id_t index;
	for (index = 0; index < arraycount(windows); index++) {
		if (!windows[index].in_use)
			break;
	}
	if (index >= arraycount(windows)) {
		return -1;
	}

	int bg_tile_start = shadow_tiles_allocate(args->bg, args->width * args->height);
	if (bg_tile_start < 0) {
		return -1;
	}

	windows[index].in_use = true;
	windows[index].bg_tile_start = bg_tile_start;
	windows[index].args = *args;
	return index;
}

void shadow_tiles_window_deallocate(window_id_t id) {
	if (id >= arraycount(windows)) return;
	if (! windows[id].in_use) return;

	shadow_tiles_deallocate(windows[id].args.bg, windows[id].bg_tile_start,
			windows[id].args.width * windows[id].args.height);
	windows[id].in_use = false;
}

void shadow_tiles_window_queue_map(window_id_t id) {
	if (id >= arraycount(windows)) return;
	if (! windows[id].in_use) return;

	const unsigned width = windows[id].args.width;
	const unsigned height = windows[id].args.height;
	const unsigned x = windows[id].args.x;
	const unsigned y = windows[id].args.y;
	const unsigned screenblock = shadow_tiles_bgcnt[windows[id].args.bg].screenblock;

	unsigned bg_tile_id = windows[id].bg_tile_start | (windows[id].args.palette << 12);
	unsigned map_tile_id = x + y * 32;


	for (unsigned j = 0; j < height; j++) {
		uint16_t* buffer = malloc(sizeof(uint16_t) * width);
		if (!buffer) return;

		for (unsigned i = 0; i < width; i++) {
			buffer[i] = bg_tile_id;
			++bg_tile_id;
		}
		vram_op_queue_enqueue((struct vram_op){
			.type = VRAM_QUEUE_OP_BG_MAP_FREE,
			.map_free = {
				.from = (bg_tile_t *) buffer,
				.to_block = screenblock,
				.to_tile = map_tile_id,
				.count = width,
			}});
		map_tile_id += 32;
	}
}

void shadow_tiles_window_queue_map_with_border(window_id_t id, unsigned border_tile_start, unsigned border_palette) {
	if (id >= arraycount(windows)) return;
	if (! windows[id].in_use) return;

	const unsigned width = windows[id].args.width;
	const unsigned height = windows[id].args.height;
	const unsigned x = windows[id].args.x;
	const unsigned y = windows[id].args.y;
	const unsigned screenblock = shadow_tiles_bgcnt[windows[id].args.bg].screenblock;

	unsigned bg_tile_id = windows[id].bg_tile_start | (windows[id].args.palette << 12);
	unsigned map_tile_id = x - 1 + y * 32;

	{
		uint16_t* buffer = malloc(sizeof(uint16_t) * (width + 2));
		if (!buffer) return;
		buffer[0] = border_tile_start | (border_palette << 12);
		for (unsigned i = 0; i < width; i++) {
			buffer[i + 1] = (border_tile_start + 1) | (border_palette << 12);
		}
		buffer[width + 1] = (border_tile_start + 2) | (border_palette << 12);
		vram_op_queue_enqueue((struct vram_op){
			.type = VRAM_QUEUE_OP_BG_MAP_FREE,
			.map_free = {
				.from = (bg_tile_t *) buffer,
				.to_block = screenblock,
				.to_tile = map_tile_id - 32,
				.count = width + 2,
			}});
	}
	for (unsigned j = 0; j < height; j++) {
		uint16_t* buffer = malloc(sizeof(uint16_t) * (width + 2));
		if (!buffer) return;

		buffer[0] = (border_tile_start + 3) | (border_palette << 12);
		for (unsigned i = 0; i < width; i++) {
			buffer[i+1] = bg_tile_id;
			++bg_tile_id;
		}
		buffer[width + 1] = (border_tile_start + 5) | (border_palette << 12);
		vram_op_queue_enqueue((struct vram_op){
			.type = VRAM_QUEUE_OP_BG_MAP_FREE,
			.map_free = {
				.from = (bg_tile_t *) buffer,
				.to_block = screenblock,
				.to_tile = map_tile_id,
				.count = width + 2,
			}});
		map_tile_id += 32;
	}
	{
		uint16_t* buffer = malloc(sizeof(uint16_t) * (width + 2));
		if (!buffer) return;
		buffer[0] = (border_tile_start + 6) | (border_palette << 12);
		for (unsigned i = 0; i < width; i++) {
			buffer[i + 1] = (border_tile_start + 7) | (border_palette << 12);
		}
		buffer[width + 1] = (border_tile_start + 8) | (border_palette << 12);
		vram_op_queue_enqueue((struct vram_op){
			.type = VRAM_QUEUE_OP_BG_MAP_FREE,
			.map_free = {
				.from = (bg_tile_t *) buffer,
				.to_block = screenblock,
				.to_tile = map_tile_id,
				.count = width + 2,
			}});
	}
}

void shadow_tiles_window_queue_tiles(window_id_t id, const tile_4bpp_t* data) {
	if (id >= arraycount(windows)) return;
	if (! windows[id].in_use) return;

	const unsigned charblock = shadow_tiles_bgcnt[windows[id].args.bg].charblock;
	const unsigned bg_tile_id = windows[id].bg_tile_start;
	const unsigned area = windows[id].args.width * windows[id].args.height;

	vram_op_queue_enqueue((struct vram_op){
		.type = VRAM_QUEUE_OP_BG_TILES,
		.tiles = {
			.from = data,
			.to_block = charblock,
			.to_tile = bg_tile_id,
			.count = area,
		}});
}

void shadow_tiles_window_queue_tiles_free(window_id_t id, tile_4bpp_t* data) {
	if (id >= arraycount(windows)) return;
	if (! windows[id].in_use) return;

	const unsigned charblock = shadow_tiles_bgcnt[windows[id].args.bg].charblock;
	const unsigned bg_tile_id = windows[id].bg_tile_start;
	const unsigned area = windows[id].args.width * windows[id].args.height;

	vram_op_queue_enqueue((struct vram_op){
		.type = VRAM_QUEUE_OP_BG_TILES_FREE,
		.tiles_free = {
			.from = data,
			.to_block = charblock,
			.to_tile = bg_tile_id,
			.count = area,
		}});
}

int shadow_tiles_load_tileset(const struct tileset* data, struct shadow_tiles_load_tileset args) {
	int start = shadow_tiles_allocate(args.bg, data->tileset_count);
	if (start < 0) {
		return -1;
	}

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_TILES_COMPRESSED,
		.tiles_compressed = {
			.from = data->tileset,
			.to_block = shadow_tiles_bgcnt[args.bg].charblock,
			.to_tile = start,
		},
	});

	return start;
}
void shadow_tiles_deallocate_tileset(window_id_t tile_start, const struct tileset* data, struct shadow_tiles_load_tileset args) {
	shadow_tiles_deallocate(args.bg, tile_start, data->tileset_count);
}

bool shadow_tiles_load_tileset_fixed(unsigned bg, unsigned start, unsigned count, const tile_4bpp_t* tileset) {
	start += TILES_PER_CHARBLOCK * shadow_tiles_bgcnt[bg].charblock;
	for (unsigned i = start; i < start + count; i++) {
		if (shadow_tiles_used[i])
			return false;
	}
	for (unsigned i = start; i < start + count; i++) {
		shadow_tiles_used[i] = true;
	}

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_TILES,
		.tiles = {
			.from = tileset,
			.to_block = 0,
			.to_tile = start,
			.count = count,
		},
	});

	return true;
}

bool shadow_tiles_load_tileset_fixed_compressed(unsigned bg, unsigned start, unsigned count, const struct CompressedData* tileset) {
	start += TILES_PER_CHARBLOCK * shadow_tiles_bgcnt[bg].charblock;
	for (unsigned i = start; i < start + count; i++) {
		if (shadow_tiles_used[i])
			return false;
	}
	for (unsigned i = start; i < start + count; i++) {
		shadow_tiles_used[i] = true;
	}

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_TILES_COMPRESSED,
		.tiles_compressed = {
			.from = tileset,
			.to_block = 0,
			.to_tile = start,
		},
	});

	return true;
}

bool shadow_tiles_load_background(const struct background* data, struct shadow_tiles_load_background args) {
	MgbaPrintf(MGBA_LOG_DEBUG, "ENTER shadow_tiles_load_background");
	MgbaPrintf(MGBA_LOG_DEBUG, "  data->tilemap_count = %d",  data->tilemap_count);
	MgbaPrintf(MGBA_LOG_DEBUG, "  data->tileset_count = %d",  data->tileset_count);

	if (! shadow_tiles_load_tileset_fixed_compressed(args.bg, 0, data->tileset_count, data->tileset)) {
		return false;
	}

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_PALETTES,
		.palettes = {
			.from = data->palette,
			.to_palette = 0,
			.count = data->palette_count,
		},
	});

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_MAP_COMPRESSED,
		.map_compressed = {
			.from = data->tilemap,
			.to_block = shadow_tiles_bgcnt[args.bg].screenblock,
			.to_tile = 0,
		},
	});

	return true;
}
