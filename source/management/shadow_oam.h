#ifndef SHADOW_OAM_H
#define SHADOW_OAM_H

#include <stdbool.h>
#include <stdint.h>
#include "gba/palette.h"

typedef uint8_t shadow_oam_id_t;
typedef uint8_t shadow_oam_palid_t;
typedef uint8_t shadow_oam_tileid_t;
static const shadow_oam_id_t shadow_id_invalid = 0xFF;
static const shadow_oam_palid_t shadow_palid_invalid = 0xFF;
static const shadow_oam_tileid_t shadow_tileid_invalid = 0xFF;

enum hotspot {
	HOTSPOT_CENTER,
	HOTSPOT_LEFT,
	HOTSPOT_RIGHT,
	HOTSPOT_TOP,
	HOTSPOT_BOTTOM,
	HOTSPOT_TOPLEFT,
	HOTSPOT_TOPRIGHT,
	HOTSPOT_BOTTOMLEFT,
	HOTSPOT_BOTTOMRIGHT,
};

typedef struct {
	uint16_t x;
	uint16_t y;
} ucoords16_t;

struct shadow_oam_template;

struct shadow_oam_position {
	ucoords16_t coord;
	enum hotspot hotspot : 8;
	bool hflip : 1;
	bool vflip : 1;
	unsigned priority : 2;
};

void shadow_oam_init(void);

void shadow_oam_free_all(void);

void shadow_oam_preload_sprite(
	const struct shadow_oam_template*);
void shadow_oam_preload_sprite_no_palette_vram_op(
	union palette512* palette,
	const struct shadow_oam_template*);

shadow_oam_id_t shadow_oam_add_sprite(
	const struct shadow_oam_template*,
	const struct shadow_oam_position);
shadow_oam_id_t shadow_oam_add_sprite_no_palette_vram_op(
	union palette512* palette,
	const struct shadow_oam_template*,
	const struct shadow_oam_position);
void shadow_oam_remove_sprite(shadow_oam_id_t);
bool shadow_oam_rewrite_sprite(
	shadow_oam_id_t shadow_oam_index,
	const struct shadow_oam_template* template,
	const struct shadow_oam_position position);
void shadow_oam_move_sprite(shadow_oam_id_t,
	const struct shadow_oam_position);

#endif        //  #ifndef SHADOW_OAM_H
