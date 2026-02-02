#ifndef VRAM_OP_QUEUE_H
#define VRAM_OP_QUEUE_H

#include "gba/bios.h"
#include "gba/hw_reg.h"
#include "gba/oam.h"
#include "gba/palette.h"
#include "gba/vram.h"

struct CompressedData;

enum vram_queue_op_type {
	/** None */
	VRAM_QUEUE_OP_NOOP = 0,
	/** None */
	VRAM_QUEUE_OP_DISABLE_ALL_OAM,
	/** .palettes */
	VRAM_QUEUE_OP_BG_PALETTES,
	/** .tiles */
	VRAM_QUEUE_OP_BG_TILES,
	/** .tiles_free ; becomes the owner of `.from` and will free `.from` */
	VRAM_QUEUE_OP_BG_TILES_FREE,
	/** .tiles_compressed */
	VRAM_QUEUE_OP_BG_TILES_COMPRESSED,
	/** .tiles_fill */
	VRAM_QUEUE_OP_BG_TILES_FILL,
	/** .tiles_bitunpack */
	VRAM_QUEUE_OP_BG_TILES_BITUNPACK,
	/** .map */
	VRAM_QUEUE_OP_BG_MAP,
	/** .map_free ; becomes the owner of `.from` and will free `.from` */
	VRAM_QUEUE_OP_BG_MAP_FREE,
	/** .map_compressed */
	VRAM_QUEUE_OP_BG_MAP_COMPRESSED,
	/** .map */
	VRAM_QUEUE_OP_BG_MAP_COLUMN,
	/** .map_free ; becomes the owner of `.from` and will free `.from` */
	VRAM_QUEUE_OP_BG_MAP_COLUMN_FREE,
	/** .map_fill */
	VRAM_QUEUE_OP_BG_MAP_FILL,
	/** .palettes */
	VRAM_QUEUE_OP_OAM_PALETTES,
	/** .tiles */
	VRAM_QUEUE_OP_OAM_TILES,
	/** .tiles_free ; becomes the owner of `.from` and will free `.from` */
	VRAM_QUEUE_OP_OAM_TILES_FREE,
	/** .tiles_compressed */
	VRAM_QUEUE_OP_OAM_TILES_COMPRESSED,
	/** .oam */
	VRAM_QUEUE_OP_OAM_ENTRY,
	/** .dispcnt */
	VRAM_QUEUE_OP_HWREG_DISPCNT,
	/** .bgcnt */
	VRAM_QUEUE_OP_HWREG_BGCNT,
	/** .bgofss */
	VRAM_QUEUE_OP_HWREG_BGOFSS,
	/** .uint16 */
	VRAM_QUEUE_OP_UINT16,
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
			tile_4bpp_t* from;
			uint16_t to_block;
			uint16_t to_tile;
			uint16_t count;
		} tiles_free;
		struct {
			uint16_t value: 4;
			uint16_t to_block;
			uint16_t to_tile;
			uint16_t count;
		} tiles_fill;
		struct {
			const struct CompressedData* from;
			uint16_t to_block;
			uint16_t to_tile;
		} tiles_compressed;
		struct {
			const char* from;
			uint16_t to_block;
			uint16_t to_tile;
			struct BitUnPack param;
		} tiles_bitunpack;
		struct {
			const bg_tile_t* from;
			uint16_t to_block;
			uint16_t to_tile;
			uint16_t count;
		} map;
		struct {
			bg_tile_t* from;
			uint16_t to_block;
			uint16_t to_tile;
			uint16_t count;
		} map_free;
		struct {
			bg_tile_t value;
			uint16_t to_block;
			uint16_t to_tile;
			uint16_t count;
		} map_fill;
		struct {
			const struct CompressedData* from;
			uint16_t to_block;
			uint16_t to_tile;
		} map_compressed;
		struct {
			oam_t value;
			uint16_t to_index;
		} oam;
		struct {
			dispcnt_t value;
		} dispcnt;
		struct {
			bgcnt_t value;
			uint16_t to_index;
		} bgcnt;
		struct {
			struct bgofs value[4];
		} bgofss;
		struct {
			uint16_t value;
			volatile uint16_t* to;
		} uint16;
	};
};

void vram_op_queue_execute(void);
void vram_op_queue_enqueue(const struct vram_op* new_op);

#endif //  #ifndef VRAM_OP_QUEUE_H
