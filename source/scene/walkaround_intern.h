void MainCB_walkaround_main(void);

#include "gba/hw_reg.h"
#include "management/shadow_vram.h"
#include "management/shadow_oam.h"
#include "graphics_types.h"

enum direction {
	DIRECTION_NORTH,
	DIRECTION_SOUTH,
	DIRECTION_EAST,
	DIRECTION_WEST,
};

enum action {
	ACTION_NONE,
	ACTION_WALKING,
	ACTION_TURNING,
};

/* tile position relative to the top-left of the map */
typedef struct { int16_t x; int16_t y; } tile_coord_t;
/* pixel position relative to the top-left of the map */
typedef struct { int16_t x; int16_t y; } mapoffs_t;
/* pixel position relative to the screen */
typedef struct { int16_t x; int16_t y; } screenoffs_t;

struct oam_animation;

extern struct walkaround_model {
	const struct tile16x3map* map;
	struct {
		tile_coord_t pos;
		uint8_t turn_timer;
		enum action action;
		enum direction facing;
	} player;
} walkaround_state;

extern struct walkaround_viewmodel {
	struct {
		bool is_open;
		uint8_t selection;
		uint8_t num_items;
		uint8_t enabled_items[8];
		window_id_t window_id;
		uint16_t border_tile_start;
		shadow_oam_id_t pointer_oam_id;
	} start_menu;
	struct {
		bgofs_t bgofs;
		mapoffs_t mapoffs;
	} camera;
	struct {
		shadow_oam_id_t oam_id;
		mapoffs_t mapoffs;
		const struct oam_animation* anim;
		uint8_t anim_frame;
		uint8_t anim_delay;
	} player;
} walkaround_viewmodel;
