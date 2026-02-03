#ifndef SHADOW_VRAM_H
#define SHADOW_VRAM_H

#include <stdbool.h>
#include <stdint.h>
#include "gba/hw_reg.h"
#include "gba/vram.h"

struct background;
struct tileset;

typedef uint8_t window_id_t;
static const window_id_t window_id_invalid = 0xFF;

struct shadow_vram_init {
	bool enable_bg[4];
	bool enable_obj;
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
void shadow_tiles_window_queue_map_with_border(window_id_t id, unsigned border_tile_start, unsigned border_palette);
void shadow_tiles_window_queue_tiles(window_id_t id, const tile_4bpp_t*);
void shadow_tiles_window_queue_tiles_free(window_id_t id, tile_4bpp_t* data);

struct shadow_tiles_load_tileset {
	unsigned bg;
};
int shadow_tiles_load_tileset(const struct tileset*, struct shadow_tiles_load_tileset);
void shadow_tiles_deallocate_tileset(window_id_t tile_start, const struct tileset*, struct shadow_tiles_load_tileset);

struct shadow_tiles_load_background {
	unsigned bg;
};
bool shadow_tiles_load_background(const struct background*, struct shadow_tiles_load_background);

bool shadow_tiles_load_background_no_palette_vram_op(const struct background*, struct shadow_tiles_load_background);

#endif        //  #ifndef SHADOW_VRAM_H
