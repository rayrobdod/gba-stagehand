#include "management/scene_graphics.h"

#include "gba/bios.h"
#include "gba/hw_reg.h"
#include "gba/palette.h"
#include "gba/vram.h"
#include "management/vram_op_queue.h"

struct scene_graphics_bg {
	uint16_t count;
	bgcnt_t cnt;
	bg_tile_t* map;
};

struct scene_graphics {
	uint8_t palettes_count;
	palette16_t* palettes;

	uint16_t tileset_count;
	tile_4bpp_t* tileset;

	struct scene_graphics_bg bg0;
};

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

void queue_load_scene_graphics(
	const struct scene_graphics* data,
	const struct load_scene_graphics param) {

	reg_lcd.DISPCNT = (dispcnt_t){
		.mode = 0,
		.enable_bg0 = true,
		.obj_character_mapping = OBJ_CHAR_MAP_1D,
	};

	reg_lcd.BG0CNT = data->bg0.cnt;
	reg_lcd.BG0HOFS = 0;
	reg_lcd.BG0VOFS = 0;

	struct tileset_graphics tileset_data = (struct tileset_graphics){
		.palettes_count = data->palettes_count,
		.palettes = data->palettes,
		.tileset_count = data->tileset_count,
		.tileset = data->tileset,
	};

	queue_load_tileset_graphics(
		&tileset_data,
		(struct load_tileset_graphics){
			.charblock = 0,
			.palette_offset = 0,
			.tile_offset = 0,
		});

	vram_op_queue_enqueue((struct vram_op){
		.type = VRAM_QUEUE_OP_BG_MAP,
		.map = {
			.from = data->bg0.map,
			.to_block = 31,
			.to_tile = 0,
			.count = data->bg0.count,
		}});
}
