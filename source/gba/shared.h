#ifndef GUARD_GBA_SHARED_H
#define GUARD_GBA_SHARED_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	uint16_t r: 5;
	uint16_t g: 5;
	uint16_t b: 5;
} rgb_t;
_Static_assert(2 == sizeof(rgb_t));

__attribute__((const)) inline static rgb_t rgb(uint16_t r, uint16_t g, uint16_t b) {
	rgb_t retval = {
		.r = r,
		.g = g,
		.b = b,
	};
	return retval;
}

enum WordSize {
	WORDSIZE_16BIT = 0,
	WORDSIZE_32BIT = 1,
};

typedef enum {
	PAL_MODE_4BPP = 0,
	PAL_MODE_8BPP = 1,
} palette_mode_t;


typedef union {
	struct {
		bool vblank: 1;
		bool hblank: 1;
		bool vcount: 1;
		bool timer0: 1;
		bool timer1: 1;
		bool timer2: 1;
		bool timer3: 1;
		bool com: 1;
		bool dma0: 1;
		bool dma1: 1;
		bool dma2: 1;
		bool dma3: 1;
		bool keypad: 1;
		bool gamepak: 1;
	};
	uint16_t all;
} interrupt_flag_t;
_Static_assert(2 == sizeof(interrupt_flag_t));


#endif
