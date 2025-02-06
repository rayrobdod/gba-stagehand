#include "gba/vram.h"

__attribute__((section(".vram"), used))
volatile vram_t vram;
