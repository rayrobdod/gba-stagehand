#include "vram_op_queue.h"

#include <stdlib.h>
#include "decompress/by_header.h"
#include "gba/bios.h"
#include "utils/arraycount.h"
#include "mgba.h"

#define VRAM_OP_QUEUE_CAPACITY (128)

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
		case VRAM_QUEUE_OP_OAM_TILES_COMPRESSED:
			HeaderUnCompVram(
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
		case VRAM_QUEUE_OP_BG_TILES_COMPRESSED:
			HeaderUnCompVram(
				entry->tiles_compressed.from,
				&vram.bg_charblock[entry->tiles_compressed.to_block][entry->tiles_compressed.to_tile]);
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
		case VRAM_QUEUE_OP_BG_MAP_COMPRESSED:
			HeaderUnCompVram(
				entry->map_compressed.from,
				&vram.screenblock[entry->map_compressed.to_block][entry->map_compressed.to_tile]);
			break;
		case VRAM_QUEUE_OP_BG_MAP_FREE:
			CpuSet(
				entry->map_free.from,
				&vram.screenblock[entry->map_free.to_block][entry->map_free.to_tile],
				(struct CpuSet){
					.word_count = entry->map_free.count,
					.mode = CPU_SET_COPY,
					.datasize = WORDSIZE_16BIT,
				});
			free(entry->map_free.from);
			break;
		case VRAM_QUEUE_OP_BG_MAP_FILL:
			CpuSet(
				&entry->map_fill.value,
				&vram.screenblock[entry->map_fill.to_block][entry->map_fill.to_tile],
				(struct CpuSet){
					.word_count = entry->map_fill.count,
					.mode = CPU_SET_FILL,
					.datasize = WORDSIZE_16BIT,
				});
			break;
		case VRAM_QUEUE_OP_BG_MAP_COLUMN:
			{
				unsigned rows = entry->map.count;
				const bg_tile_t* from = entry->map.from;
				uint16_t to_tile = entry->map.to_tile;

				while (rows > 0) {
					vram.screenblock[entry->map.to_block][to_tile] = *from;
					rows--;
					from++;
					to_tile += 32;
				}
			}
			break;
		case VRAM_QUEUE_OP_OAM_ENTRY:
			oam[entry->oam.to_index] = entry->oam.value;
			break;
		case VRAM_QUEUE_OP_HWREG_DISPCNT:
			reg_lcd.DISPCNT = entry->dispcnt.value;
			break;
		case VRAM_QUEUE_OP_HWREG_BGCNT:
			reg_lcd.BGCNT[entry->bgcnt.to_index] = entry->bgcnt.value;
			break;
		case VRAM_QUEUE_OP_HWREG_BGOFSS:
			reg_lcd.BGOFS[0] = entry->bgofss.value[0];
			reg_lcd.BGOFS[1] = entry->bgofss.value[1];
			reg_lcd.BGOFS[2] = entry->bgofss.value[2];
			reg_lcd.BGOFS[3] = entry->bgofss.value[3];
			break;
		case VRAM_QUEUE_OP_UINT16:
			*(entry->uint16.to) = entry->uint16.value;
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
