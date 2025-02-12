#ifndef VRAM_OP_QUEUE_H
#define VRAM_OP_QUEUE_H

#include "gba/oam.h"
#include "gba/palette.h"
#include "gba/vram.h"

enum vram_queue_op_type {
	VRAM_QUEUE_OP_NOOP = 0,
	VRAM_QUEUE_OP_DISABLE_ALL_OAM,
	VRAM_QUEUE_OP_OAM_PALETTE,
	VRAM_QUEUE_OP_OAM_TILES,
	VRAM_QUEUE_OP_OAM_ENTRY,
};

struct vram_op {
	enum vram_queue_op_type type;
	union {
		struct {
			palette16_t* from;
			uint16_t to_index;
		} palette;
		struct {
			tile_4bpp_t* from;
			uint16_t to_index;
			uint16_t count;
		} tiles;
		struct {
			oam_t value;
			uint16_t to_index;
		} oam;
	};
};

void vram_op_queue_execute(void);
void vram_op_queue_enqueue(const struct vram_op new_op);

#endif        //  #ifndef VRAM_OP_QUEUE_H
