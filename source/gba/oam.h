#ifndef GUARD_GBA_OAM_H
#define GUARD_GBA_OAM_H

#include <stdint.h>
#include <stdbool.h>
#include "gba/shared.h"

enum oam_mode {
	OAM_MODE_NORMAL = 0,
	OAM_MODE_BLEND = 1,
	OAM_MODE_WINDOW = 2,
};

enum oam_shape {
	OAM_SHAPE_SQUARE = 0,
	OAM_SHAPE_HORIZONTAL = 1,
	OAM_SHAPE_VERTICAL = 2,
};

typedef struct {
	unsigned int y : 8;
	bool rotation : 1;
	//union {
		bool disabled : 1;
	//	bool double_sized : 1;
	//};
	enum oam_mode mode : 2;
	bool mosaic: 1;
	palette_mode_t palette_mode : 1;
	enum oam_shape shape : 2;

	unsigned int x: 9;
	//union {
	//	unsigned int rotation_param : 5;
	//	struct {
			int _unused0: 3;
			bool hflip: 1;
			bool vflip: 1;
	//	};
	//};
	unsigned int size: 2;

	unsigned int tile_num: 10;
	unsigned int priority: 2;
	unsigned int palette_num: 4;

	unsigned int _fill_affine: 16;
} oam_t;

extern volatile oam_t oam[128];


#endif
