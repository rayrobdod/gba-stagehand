#ifndef SHADOW_VRAM_H
#define SHADOW_VRAM_H

#include <stdbool.h>
#include <stdint.h>
#include "gba/hw_reg.h"
#include "gba/palette.h"
#include "gba/vram.h"

struct background;
struct tileset;

typedef uint8_t window_id_t;
typedef uint8_t shadow_vram_palid_t;
typedef uint16_t shadow_vram_tileid_t;
static const window_id_t window_id_invalid = 0xFF;
static const shadow_vram_palid_t shadow_vram_palid_invalid = 0xFF;
static const shadow_vram_tileid_t shadow_vram_tileid_invalid = 0xFFFF;

typedef struct {
	shadow_vram_tileid_t tileid;
	shadow_vram_palid_t palid;
} shadow_tiles_load_tileset_retval_t;


struct shadow_vram_init {
	bool enable_bg[4];
	bool enable_obj;
	bool enable_win[3];
	bgcnt_t bgcnt[4];
};
void shadow_vram_init(const struct shadow_vram_init*);
void shadow_vram_free_all(void);

struct shadow_tiles_window_allocate {
	uint8_t bg;
	uint8_t palette;
	uint8_t x;
	uint8_t y;
	uint8_t width;
	uint8_t height;
};
window_id_t shadow_tiles_window_allocate(const struct shadow_tiles_window_allocate*);
void shadow_tiles_window_deallocate(window_id_t id);

void shadow_tiles_window_queue_map(window_id_t id);
void shadow_tiles_window_queue_map_with_border(window_id_t id, shadow_tiles_load_tileset_retval_t border_ids);
void shadow_tiles_window_queue_tiles(window_id_t id, const tile_4bpp_t*);
void shadow_tiles_window_queue_tiles_free(window_id_t id, tile_4bpp_t* data);

typedef struct {
	unsigned bg;
} shadow_tiles_load_tileset_args_t;
shadow_tiles_load_tileset_retval_t shadow_tiles_load_tileset(
	const struct tileset*,
	shadow_tiles_load_tileset_args_t);
shadow_tiles_load_tileset_retval_t shadow_tiles_load_tileset_no_palette_vram_op(
	union palette512* palette,
	const struct tileset*,
	shadow_tiles_load_tileset_args_t);

void shadow_tiles_deallocate_tileset(
	shadow_tiles_load_tileset_retval_t,
	const struct tileset*,
	shadow_tiles_load_tileset_args_t);


typedef struct {
	uint8_t bg;
	uint8_t start_palette;
	uint16_t start_tiles;
} shadow_tiles_load_tileset_fixed_t;
bool shadow_tiles_load_tileset_fixed(
	const struct tileset*,
	shadow_tiles_load_tileset_fixed_t);
bool shadow_tiles_load_tileset_fixed_no_palette_vram_op(
	union palette512* palette,
	const struct tileset*,
	shadow_tiles_load_tileset_fixed_t);


struct shadow_tiles_load_background {
	unsigned bg;
};
bool shadow_tiles_load_background(
	const struct background*,
	struct shadow_tiles_load_background);
bool shadow_tiles_load_background_no_palette_vram_op(
	union palette512* palette,
	const struct background*,
	struct shadow_tiles_load_background);

#endif        //  #ifndef SHADOW_VRAM_H
