#include "gba/hw_reg.h"

__attribute__((section(".hw_reg.lcd"), used))
volatile reg_lcd_t reg_lcd;

__attribute__((section(".hw_reg.keypad"), used))
volatile reg_keypad_t reg_keypad;

__attribute__((section(".hw_reg.interrupt"), used))
volatile reg_interrupt_t reg_interrupt;
