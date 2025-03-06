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
		struct vram_op* entry = vram_op_queue + i;
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
		case VRAM_QUEUE_OP_BG_PALETTES:
			CpuFastSet(
				entry->palettes.from,
				&background_palette[entry->palettes.to_palette],
				(struct CpuFastSet){
					.word_count = entry->palettes.count * (sizeof(palette16_t) / sizeof(uint32_t)),
					.mode = CPU_SET_COPY,
				});
			break;
		case VRAM_QUEUE_OP_OAM_PALETTES:
			CpuFastSet(
				entry->palettes.from,
				&object_palette[entry->palettes.to_palette],
				(struct CpuFastSet){
					.word_count = entry->palettes.count * (sizeof(palette16_t) / sizeof(uint32_t)),
					.mode = CPU_SET_COPY,
				});
			break;
		case VRAM_QUEUE_OP_OAM_TILES:
			CpuFastSet(
				entry->tiles.from,
				&vram.obj_charblock[entry->tiles.to_block][entry->tiles.to_tile],
				(struct CpuFastSet){
					.word_count = entry->tiles.count * (sizeof(tile_4bpp_t) / sizeof(uint32_t)),
					.mode = CPU_SET_COPY,
				});
			break;
		case VRAM_QUEUE_OP_OAM_TILES_LZ:
			LZ77UnCompVram(
				entry->tiles_compressed.from,
				&vram.obj_charblock[entry->tiles_compressed.to_block][entry->tiles_compressed.to_tile]);
			break;
		case VRAM_QUEUE_OP_BG_TILES:
			CpuFastSet(
				entry->tiles.from,
				&vram.bg_charblock[entry->tiles.to_block][entry->tiles.to_tile],
				(struct CpuFastSet){
					.word_count = entry->tiles.count * (sizeof(tile_4bpp_t) / sizeof(uint32_t)),
					.mode = CPU_SET_COPY,
				});
			break;
		case VRAM_QUEUE_OP_BG_TILES_BITUNPACK:
			BitUnPack(
				entry->tiles_bitunpack.from,
				&vram.bg_charblock[entry->tiles_bitunpack.to_block][entry->tiles_bitunpack.to_tile],
				&entry->tiles_bitunpack.param);
			break;
		case VRAM_QUEUE_OP_BG_MAP:
			CpuSet(
				entry->map.from,
				&vram.screenblock[entry->map.to_block][entry->map.to_tile],
				(struct CpuSet){
					.word_count = entry->map.count,
					.mode = CPU_SET_COPY,
					.datasize = WORDSIZE_16BIT,
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
	} else {
		MgbaPrintf(MGBA_LOG_ERROR, "VRAM Queue exhausted");
	}
}
