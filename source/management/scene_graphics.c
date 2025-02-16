#include "management/scene_graphics.h"

#include "gba/bios.h"
#include "gba/palette.h"
#include "gba/vram.h"
#include "management/vram_op_queue.h"

struct tileset_graphics {
	uint8_t palettes_count;
	palette16_t* palettes;

	uint16_t tileset_count;
	tile_4bpp_t* tileset;
};

void queue_load_tileset_graphics(
	const struct tileset_graphics* data,
	const struct load_tileset_graphics param) {

	vram_op_queue_enqueue((struct vram_op){
		.type = VRAM_QUEUE_OP_BG_PALETTES,
		.palettes = {
			.from = data->palettes,
			.to_palette = param.palette_offset,
			.count = data->palettes_count,
		}});

	vram_op_queue_enqueue((struct vram_op){
		.type = VRAM_QUEUE_OP_BG_TILES,
		.tiles = {
			.from = data->tileset,
			.to_block = param.charblock,
			.to_tile = param.tile_offset,
			.count = data->tileset_count,
		}});
}
