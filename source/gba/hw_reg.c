#include "gba/hw_reg.h"

__attribute__((section(".hw_reg.lcd"), used))
volatile reg_lcd_t reg_lcd;

[[gnu::section(".hw_reg.sound"), gnu::used]]
volatile struct reg_sound reg_sound;

__attribute__((section(".hw_reg.dma"), used))
volatile struct reg_dma reg_dma[4];

__attribute__((section(".hw_reg.timer"), used))
volatile struct timer reg_timer[4];

__attribute__((section(".hw_reg.keypad"), used))
volatile reg_keypad_t reg_keypad;

__attribute__((section(".hw_reg.interrupt"), used))
volatile reg_interrupt_t reg_interrupt;
