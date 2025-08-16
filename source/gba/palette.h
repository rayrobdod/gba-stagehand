#ifndef GUARD_GBA_PALETTE_H
#define GUARD_GBA_PALETTE_H

#include "gba/shared.h"

typedef rgb_t palette16_t[16];

union palette256 {
	rgb_t _8[256];
	palette16_t _4[16];
};

union palette512 {
	rgb_t all[512];
	struct {
		union palette256 background;
		union palette256 object;
	};
};

extern volatile union palette512 hw_palette;

#endif        //  #ifndef GUARD_GBA_PALETTE_H
