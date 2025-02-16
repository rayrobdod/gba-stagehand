#ifndef VRAM_OP_QUEUE_H
#define VRAM_OP_QUEUE_H

#include "gba/oam.h"
#include "gba/palette.h"
#include "gba/vram.h"

enum vram_queue_op_type {
	/** None */
	VRAM_QUEUE_OP_NOOP = 0,
	/** None */
	VRAM_QUEUE_OP_DISABLE_ALL_OAM,
	/** .palettes */
	VRAM_QUEUE_OP_BG_PALETTES,
	/** .tiles */
	VRAM_QUEUE_OP_BG_TILES,
	/** .map */
	VRAM_QUEUE_OP_BG_MAP,
	/** .palettes */
	VRAM_QUEUE_OP_OAM_PALETTES,
	/** .tiles */
	VRAM_QUEUE_OP_OAM_TILES,
	/** .oam */
	VRAM_QUEUE_OP_OAM_ENTRY,
};

struct vram_op {
	enum vram_queue_op_type type;
	union {
		struct {
			const palette16_t* from;
			uint16_t to_palette;
			uint16_t count;
		} palettes;
		struct {
			const tile_4bpp_t* from;
			uint16_t to_block;
			uint16_t to_tile;
			uint16_t count;
		} tiles;
		struct {
			const bg_tile_t* from;
			uint16_t to_block;
			uint16_t to_tile;
			uint16_t count;
		} map;
		struct {
			oam_t value;
			uint16_t to_index;
		} oam;
	};
};

void vram_op_queue_execute(void);
void vram_op_queue_enqueue(const struct vram_op new_op);

#endif //  #ifndef VRAM_OP_QUEUE_H
