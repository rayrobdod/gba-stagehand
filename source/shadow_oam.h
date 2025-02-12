#ifndef SHADOW_OAM_H
#define SHADOW_OAM_H

#include <stdbool.h>
#include <stdint.h>

typedef uint16_t paltag_t;
typedef uint16_t tiletag_t;
typedef uint8_t shadow_oam_id_t;

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
};

void shadow_oam_init(void);

void shadow_oam_free_all(void);
shadow_oam_id_t shadow_oam_add_sprite(
	const struct shadow_oam_template*,
	const struct shadow_oam_position);
void shadow_oam_remove_sprite(shadow_oam_id_t);
void shadow_oam_move_sprite(shadow_oam_id_t,
	const struct shadow_oam_position);

#endif        //  #ifndef SHADOW_OAM_H
