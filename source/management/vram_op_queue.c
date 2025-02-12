#include "vram_op_queue.h"

#include "gba/bios.h"
#include "mgba.h"

#define VRAM_OP_QUEUE_CAPACITY (128)
#define arraycount(a) (sizeof(a) / sizeof(a[0]))

__attribute__((section(".sbss.vram_op_queue_count")))
static uint8_t vram_op_queue_count = 0;

__attribute__((section(".sbss.vram_op_queue")))
static struct vram_op vram_op_queue[VRAM_OP_QUEUE_CAPACITY];

void vram_op_queue_execute(void) {
	for (unsigned i = 0; i < vram_op_queue_count; i++) {
		struct vram_op *entry = vram_op_queue + i;
		switch (entry->type) {
		case VRAM_QUEUE_OP_NOOP:
			break;
		case VRAM_QUEUE_OP_DISABLE_ALL_OAM:
			{
				oam_t new_value = {.disabled = true};
				for (unsigned i = 0; i < arraycount(oam); i++) {
					oam[i] = new_value;
				}
			}
			break;
		case VRAM_QUEUE_OP_OAM_PALETTE:
			MgbaPrintf(MGBA_LOG_INFO, "shadow oam push palette {%p, %d}", entry->palette.from, entry->palette.to_index);
			CpuFastSet(
				entry->palette.from,
				&object_palette[entry->palette.to_index],
				(struct CpuFastSet) {
					.word_count = (sizeof(palette16_t) / sizeof(uint32_t)),
					.mode = CPU_SET_COPY,
				});
			break;
		case VRAM_QUEUE_OP_OAM_TILES:
			MgbaPrintf(MGBA_LOG_INFO, "shadow oam push tiles {%p, %d, %d}", entry->tiles.from, entry->tiles.to_index, entry->tiles.count);
			CpuFastSet(
				entry->tiles.from,
				&vram.obj_charblock[0][entry->tiles.to_index],
				(struct CpuFastSet) {
					.word_count = entry->tiles.count * (sizeof(tile_4bpp_t) / sizeof(uint32_t)),
					.mode = CPU_SET_COPY,
				});
			break;
		case VRAM_QUEUE_OP_OAM_ENTRY:
			oam[entry->oam.to_index] = entry->oam.value;
			break;
		}
	}
	vram_op_queue_count = 0;
}

void vram_op_queue_enqueue(const struct vram_op new_op) {
	if (vram_op_queue_count < VRAM_OP_QUEUE_CAPACITY) {
		vram_op_queue[vram_op_queue_count] = new_op;
		vram_op_queue_count++;
	}
	else {
		MgbaPrintf(MGBA_LOG_ERROR, "VRAM Queue exhausted");
	}
}
