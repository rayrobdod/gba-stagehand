#include "gba/oam.h"

__attribute__((section(".oam"), used))
volatile oam_t oam[128];
