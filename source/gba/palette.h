#ifndef GUARD_GBA_PALETTE_H
#define GUARD_GBA_PALETTE_H

#include <stdint.h>
#include "gba/shared.h"

typedef rgb_t palette16_t[16];

typedef palette16_t palette_t[16];

extern volatile palette_t background_palette;
extern volatile palette_t object_palette;

#endif        //  #ifndef PALETTE_H
