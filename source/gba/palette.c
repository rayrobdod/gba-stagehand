#include "gba/palette.h"

__attribute__((section(".palette.bg"), used))
volatile palette_t background_palette;

__attribute__((section(".palette.obj"), used))
volatile palette_t object_palette;
