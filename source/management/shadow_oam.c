#include "shadow_oam.h"

#include "gba/bios.h"
#include "gba/oam.h"
#include "gba/palette.h"
#include "gba/vram.h"
#include "mgba.h"
#include "vram_op_queue.h"

#define arraycount(a) (sizeof(a) / sizeof(a[0]))


static const struct {
	uint8_t tilecount;
	uint8_t half_width;
	uint8_t half_height;
} tilesize_properties[3][4] = {
	[OAM_SHAPE_SQUARE] = {
		[0] = {
			.tilecount = 1 * 1,
			.half_width = 4,
			.half_height = 4,
		},
		[1] = {
			.tilecount = 2 * 2,
			.half_width = 8,
			.half_height = 8,
		},
		[2] = {
			.tilecount = 4 * 4,
			.half_width = 16,
			.half_height = 16,
		},
		[3] = {
			.tilecount = 8 * 8,
			.half_width = 32,
			.half_height = 32,
		},
	},
	[OAM_SHAPE_HORIZONTAL] = {
		[0] = {
			.tilecount = 1 * 2,
			.half_width = 8,
			.half_height = 4,
		},
		[1] = {
			.tilecount = 1 * 4,
			.half_width = 16,
			.half_height = 4,
		},
		[2] = {
			.tilecount = 2 * 4,
			.half_width = 16,
			.half_height = 8,
		},
		[3] = {
			.tilecount = 4 * 8,
			.half_width = 32,
			.half_height = 16,
		},
	},
	[OAM_SHAPE_VERTICAL] = {
		[0] = {
			.tilecount = 1 * 2,
			.half_width = 4,
			.half_height = 8,
		},
		[1] = {
			.tilecount = 1 * 4,
			.half_width = 4,
			.half_height = 16,
		},
		[2] = {
			.tilecount = 2 * 4,
			.half_width = 8,
			.half_height = 16,
		},
		[3] = {
			.tilecount = 4 * 8,
			.half_width = 16,
			.half_height = 32,
		},
	},
};

static const struct {
	uint8_t x:4;
	uint8_t y:4;
} hotspot_properties[9] = {
	[HOTSPOT_TOPLEFT] = {.x = 0, .y = 0},
	[HOTSPOT_TOP] = {.x = 1, .y = 0},
	[HOTSPOT_TOPRIGHT] = {.x = 2, .y = 0},
	[HOTSPOT_LEFT] = {.x = 0, .y = 1},
	[HOTSPOT_CENTER] = {.x = 1, .y = 1},
	[HOTSPOT_RIGHT] = {.x = 2, .y = 1},
	[HOTSPOT_BOTTOMLEFT] = {.x = 0, .y = 2},
	[HOTSPOT_BOTTOM] = {.x = 1, .y = 2},
	[HOTSPOT_BOTTOMRIGHT] = {.x = 2, .y = 2},
};


struct shadow_oam_template {
	const palette16_t* palette;
	const char* tiles;
	paltag_t paltag;
	tiletag_t tiletag;
	enum oam_shape shape: 2;
	uint8_t size : 2;
};

__attribute__((section(".sbss.shadow_oam.palette")))
static struct shadow_palette {
	uint8_t refcount;
	paltag_t tag;
} shadow_palette[16] = {0};

__attribute__((section(".sbss.shadow_oam.tiles")))
static struct shadow_tile {
	uint8_t refcount;
	tiletag_t tag;
	unsigned tile_start;
	unsigned tile_count;
} shadow_tiles[64] = {0};

__attribute__((aligned(4)))
__attribute__((section(".sbss.shadow_oam.tiles_used")))
static bool shadow_tiles_used[0x400] = {0};

__attribute__((section(".sbss.shadow_oam")))
static struct shadow_oam {
	bool in_use;
	uint8_t palette_index;
	uint8_t shadow_tile_index;
	const struct shadow_oam_template* template;
} shadow_oam[64] = {0};

#define arraycount(a) (sizeof(a) / sizeof(a[0]))

void shadow_oam_init(void) {
	shadow_oam_free_all();
}

void shadow_oam_free_all(void) {
	for (unsigned i = 0; i < arraycount(shadow_palette); i++) {
		shadow_palette[i].refcount = 0;
	}
	for (unsigned i = 0; i < arraycount(shadow_tiles); i++) {
		shadow_tiles[i].refcount = 0;
	}
	for (unsigned i = 0; i < arraycount(shadow_oam); i++) {
		shadow_oam[i].in_use = false;
	}
	uint32_t i = 0;
	_Static_assert(0 == (sizeof(shadow_tiles_used) % 32), "CpuFastSet requires 32-byte-multiple length");
	CpuFastSet(&i, shadow_tiles_used, (struct CpuFastSet) {
		.word_count = sizeof(shadow_tiles_used) / sizeof(uint8_t),
		.mode = CPU_SET_FILL
	});
	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_DISABLE_ALL_OAM,
	});
}

static int shadow_tiles_allocate(unsigned count) {
	unsigned start = 0;
	unsigned span = 0;

	while (start + span < 0x400) {
		if (shadow_tiles_used[start + span]) {
			start += span + 1;
			span = 0;
			continue;
		}
		span++;
		if (span >= count) {
			for (unsigned i = start; i < start + span; i++) {
				shadow_tiles_used[start + i] = true;
			}
			return start;
		}
	}
	return -1;
}

static const shadow_oam_id_t shadow_id_invalid = 0xFF;

shadow_oam_id_t shadow_oam_add_sprite(
		const struct shadow_oam_template* template,
		const struct shadow_oam_position position) {
	MgbaPrintf(MGBA_LOG_INFO, "ENTER shadow_oam_add_sprite");

	unsigned pal_index;
	for (pal_index = 0; pal_index < arraycount(shadow_palette); pal_index++) {
		if (template->paltag == shadow_palette[pal_index].tag) {
			shadow_palette[pal_index].refcount += 1;
			goto palette_decided;
		}
	}
	for (pal_index = 0; pal_index < arraycount(shadow_palette); pal_index++) {
		if (0 == shadow_palette[pal_index].refcount) {
			shadow_palette[pal_index].refcount = 1;
			shadow_palette[pal_index].tag = template->paltag;
			vram_op_queue_enqueue((struct vram_op) {
				.type = VRAM_QUEUE_OP_OAM_PALETTES,
				.palettes = {
					.from = template->palette,
					.to_palette = pal_index,
					.count = 1,
				}
			});

			goto palette_decided;
		}
	}
	MgbaPrintf(MGBA_LOG_ERROR, "Palettes exhausted");
	return shadow_id_invalid;

	palette_decided:
	MgbaPrintf(MGBA_LOG_INFO, "  pal_index = %d", pal_index);

	const unsigned tilecount = tilesize_properties[template->shape][template->size].tilecount;

	unsigned shadow_tile_index;
	unsigned tile_index;
	for (shadow_tile_index = 0; shadow_tile_index < arraycount(shadow_tiles); shadow_tile_index++) {
		if (template->tiletag == shadow_tiles[shadow_tile_index].tag) {
			shadow_tiles[shadow_tile_index].refcount += 1;
			tile_index = shadow_tiles[shadow_tile_index].tile_start;
			goto tile_decided;
		}
	}
	tile_index = shadow_tiles_allocate(tilecount);
	if (tile_index < 0) {
		MgbaPrintf(MGBA_LOG_ERROR, "Shadow Tiles exhausted");
		return shadow_id_invalid;
	}
	for (shadow_tile_index = 0; shadow_tile_index < arraycount(shadow_tiles); shadow_tile_index++) {
		if (0 == shadow_tiles[shadow_tile_index].refcount) {
			shadow_tiles[shadow_tile_index].refcount = 1;
			shadow_tiles[shadow_tile_index].tag = template->tiletag;
			shadow_tiles[shadow_tile_index].tile_start = tile_index;
			shadow_tiles[shadow_tile_index].tile_count = tilecount;

			vram_op_queue_enqueue((struct vram_op) {
				.type = VRAM_QUEUE_OP_OAM_TILES_LZ,
				.tiles_compressed = {
					.from = template->tiles,
					.to_block = 0,
					.to_tile = tile_index,
				}
			});

			goto tile_decided;
		}
	}
	MgbaPrintf(MGBA_LOG_ERROR, "Tiles exhausted");
	return shadow_id_invalid;

	tile_decided:
	MgbaPrintf(MGBA_LOG_INFO, "  tile_index = %d", tile_index);
	MgbaPrintf(MGBA_LOG_INFO, "  shadow_tile_index = %d", shadow_tile_index);


	unsigned shadow_oam_index;
	for (shadow_oam_index = 0; shadow_oam_index < arraycount(shadow_oam); shadow_oam_index++) {
		if (! shadow_oam[shadow_oam_index].in_use) {
			shadow_oam[shadow_oam_index].in_use = true;
			shadow_oam[shadow_oam_index].palette_index = pal_index;
			shadow_oam[shadow_oam_index].shadow_tile_index = shadow_tile_index;
			shadow_oam[shadow_oam_index].template = template;

			shadow_oam_move_sprite(shadow_oam_index, position);

			break;
		}
	}

	if (shadow_oam_index >= arraycount(shadow_oam)) {
		MgbaPrintf(MGBA_LOG_INFO, "  shadow oams exhausted");
		shadow_oam_index = shadow_id_invalid;
	}

	return shadow_oam_index;
}

void shadow_oam_remove_sprite(shadow_oam_id_t index) {
	if (index >= arraycount(shadow_oam) || index < 0)
		return;

	struct shadow_oam* oam = &shadow_oam[index];

	struct shadow_tile* tile = &shadow_tiles[oam->shadow_tile_index];
	for (int i = tile->tile_start; i < tile->tile_start + tile->tile_count; i++) {
		shadow_tiles_used[i] = false;
	}
	tile->refcount--;

	struct shadow_palette* pal = &shadow_palette[oam->palette_index];
	pal->refcount--;

	oam->in_use = false;

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_OAM_ENTRY,
		.oam = {
			.to_index = index,
			.value = (oam_t) {
				.disabled = true,
			}
		}
	});
}

void shadow_oam_move_sprite(
	shadow_oam_id_t index,
	const struct shadow_oam_position position
) {
	if (index >= arraycount(shadow_oam) || index < 0)
		return;

	const struct shadow_oam* oam = &shadow_oam[index];

	const int dx = hotspot_properties[position.hotspot].x * tilesize_properties[oam->template->shape][oam->template->size].half_width;
	const int dy = hotspot_properties[position.hotspot].y * tilesize_properties[oam->template->shape][oam->template->size].half_height;

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_OAM_ENTRY,
		.oam = {
			.to_index = index,
			.value = (oam_t) {
				.y = position.coord.y - dy,
				.x = position.coord.x - dx,
				.hflip = position.hflip,
				.vflip = position.vflip,

				.tile_num = shadow_tiles[oam->shadow_tile_index].tile_start,
				.palette_num = oam->palette_index,
				.shape = oam->template->shape,
				.size = oam->template->size,
			}
		}
	});
}
