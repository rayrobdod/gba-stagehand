#include "gba/hw_reg.h"

[[gnu::section(".hw_reg.lcd"), gnu::used]]
volatile struct reg_lcd reg_lcd;

[[gnu::section(".hw_reg.sound"), gnu::used]]
volatile struct reg_sound reg_sound;

[[gnu::section(".hw_reg.dma"), gnu::used]]
volatile struct reg_dma reg_dma[4];

[[gnu::section(".hw_reg.timer"), gnu::used]]
volatile struct reg_timer reg_timer[4];

[[gnu::section(".hw_reg.keypad"), gnu::used]]
volatile struct reg_keypad reg_keypad;

[[gnu::section(".hw_reg.interrupt"), gnu::used]]
volatile struct reg_interrupt  reg_interrupt;
