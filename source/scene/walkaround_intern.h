void MainCB_walkaround_main(void);

#include "gba/hw_reg.h"
#include "management/shadow_oam.h"
#include "graphics_types.h"

enum direction {
	DIRECTION_NORTH,
	DIRECTION_SOUTH,
	DIRECTION_EAST,
	DIRECTION_WEST,
};

/* tile position relative to the top-left of the map */
typedef struct { int16_t x; int16_t y; } tile_coord_t;
/* pixel position relative to the top-left of the map */
typedef struct { int16_t x; int16_t y; } mapoffs_t;
/* pixel position relative to the screen */
typedef struct { int16_t x; int16_t y; } screenoffs_t;

extern struct walkaround_model {
	const struct tile16x3map* map;
	struct {
		tile_coord_t pos;
		enum direction facing;
	} player;
} walkaround_state;

extern struct walkaround_viewmodel {
	struct {
		bgofs_t bgofs;
		mapoffs_t mapoffs;
	} camera;
	struct {
		shadow_oam_id_t oam_id;
		mapoffs_t mapoffs;
		const struct oam_animation_cell* anim;
		uint8_t anim_frame;
		uint8_t anim_delay;
	} player;
} walkaround_viewmodel;
