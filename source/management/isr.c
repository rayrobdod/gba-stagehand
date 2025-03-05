#include "isr.h"

#include "gba/bios.h"
#include "gba/bios_reg.h"
#include "gba/hw_reg.h"
#include "gba/hw_reg_cast.h"
#include "mgba.h"

__attribute__((section(".iwram.isr_table")))
static isr isr_table[II_COUNT] = {0};

__attribute__((noinline))
__attribute__((target("arm")))
static void isr_switchboard_main(void) {
	interrupt_flag_t interrupts = reg_interrupt.IF;

	uint16_t prevIME = reg_interrupt.IME;
	reg_interrupt.IME = 0;

	for (int i = 0; i < II_COUNT; i++) {
		if (interrupts.all & (1 << i)) {
			if (isr_table[i]) {
				isr_table[i]();
			}
		}
	}

	reg_interrupt.IME = prevIME;
	reg_interrupt.IF = interrupts;
	REG_IFBIOS.all |= interrupts.all;
}

void isr_switchboard_init(void) {
	for (int i = 0; i < II_COUNT; i++) {
		isr_table[i] = 0;
	}

	REG_ISR_MAIN = isr_switchboard_main;
	reg_interrupt.IME = 1;
}

isr isr_switchboard_set(enum InterruptType type, isr handler) {
	uint16_t prevIME = reg_interrupt.IME;
	reg_interrupt.IME = 0;

	isr retval = isr_table[type];
	isr_table[type] = handler;

	reg_interrupt.IME = prevIME;
	return retval;
}

__attribute__((section(".text.isr_enable_set")))
static void isr_enable_set(enum InterruptType type, bool value) {
	uint16_t prevIME = reg_interrupt.IME;
	reg_interrupt.IME = 0;

	switch (type) {
	case II_VBLANK:
		reg_interrupt.IE.vblank = value;
		reg_lcd.DISPSTAT.vblank_enable = value;
		break;
	case II_HBLANK:
		reg_interrupt.IE.hblank = value;
		reg_lcd.DISPSTAT.hblank_enable = value;
		break;
	case II_VCOUNT:
		reg_interrupt.IE.vcount = value;
		reg_lcd.DISPSTAT.vcounter_enable = value;
		break;
	case II_KEYPAD:
		reg_interrupt.IE.keypad = value;
		reg_keypad.KEYCNT.enable = value;
		break;
	case II_GAMEPACK:
		reg_interrupt.IE.gamepak = value;
		break;
	case II_TIMER0:
	case II_TIMER1:
	case II_TIMER2:
	case II_TIMER3:
	case II_COM:
	case II_DMA0:
	case II_DMA1:
	case II_DMA2:
	case II_DMA3:
		MgbaPrintf(MGBA_LOG_FATAL, "UNIMPLEMENTED ISR_ENABLE: %d", type);
		break;
	case II_COUNT:
		break;
	}

	reg_interrupt.IME = prevIME;
}

__attribute__((section(".text.isr_enable_set")))
void isr_enable(enum InterruptType type) {
	return isr_enable_set(type, true);
}
__attribute__((section(".text.isr_enable_set")))
void isr_disable(enum InterruptType type) {
	return isr_enable_set(type, false);
}
