#include "gba/palette.h"

[[gnu::section(".palette")]]
volatile union palette512 hw_palette;
