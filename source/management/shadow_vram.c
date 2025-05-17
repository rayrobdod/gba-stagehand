#include "shadow_vram.h"

#include "gba/bios.h"
#include "gba/hw_reg.h"
#include "gba/vram.h"
#include "mgba.h"
#include "vram_op_queue.h"
#include "graphics.h"

// The main thing I want is to have "windows" that I can print text to
// without having to predetermine where in VRAM the needed tiles are.

#define arraycount(a) (sizeof(a) / sizeof(a[0]))
#define min(a,b) (a < b ? a : b)

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
	const uint32_t zero = 0;
	_Static_assert(0 == (sizeof(shadow_tiles_used) % 32), "CpuFastSet requires 32-byte-multiple length");
	CpuFastSet(&zero, shadow_tiles_used, (struct CpuFastSet) {
		.word_count = sizeof(shadow_tiles_used) / sizeof(uint8_t),
		.mode = CPU_SET_FILL
	});
}

int shadow_tiles_allocate(unsigned charblock, unsigned tilecount) {
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

int shadow_tiles_load_tileset(unsigned bg, unsigned count, const tile_4bpp_t* tileset) {
	int start = shadow_tiles_allocate(shadow_tiles_bgcnt[bg].charblock, count);
	if (start < 0) {
		return -1;
	}

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_TILES,
		.tiles = {
			.from = tileset,
			.to_block = shadow_tiles_bgcnt[bg].charblock,
			.to_tile = start,
			.count = count,
		},
	});

	return start;
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

bool shadow_tiles_load_background(struct background* data, struct shadow_tiles_load_background args) {
	MgbaPrintf(MGBA_LOG_DEBUG, "ENTER shadow_tiles_load_background");
	MgbaPrintf(MGBA_LOG_DEBUG, "  data->tilemap_count = %d",  data->tilemap_count);
	MgbaPrintf(MGBA_LOG_DEBUG, "  data->tileset_count = %d",  data->tileset_count);

	if (! shadow_tiles_load_tileset_fixed(args.bg, 0, data->tileset_count, data->tileset)) {
		return false;
	}

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_PALETTES,
		.palettes = {
			.from = data->palette,
			.to_palette = 0,
			.count = 1,
		},
	});

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_MAP,
		.map = {
			.from = data->tilemap,
			.to_block = shadow_tiles_bgcnt[args.bg].screenblock,
			.to_tile = 0,
			.count = data->tilemap_count,
		},
	});

	return true;
}
