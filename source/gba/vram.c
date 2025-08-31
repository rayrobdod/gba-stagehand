#include "gba/vram.h"

[[gnu::section(".vram"), gnu::used]]
volatile union vram vram;
