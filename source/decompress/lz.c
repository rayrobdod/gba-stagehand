#include <stdint.h>
#include "decompress/type.h"
#include "gba/hw_reg.h"
#include "gba/screen.h"

bool LZ77UnCompSuspendable(struct suspended_decompression* state) {
	while (state->dest < state->dest_end && (reg_lcd.VCOUNT < (DISPLAY_HEIGHT - 5) || reg_lcd.VCOUNT > DISPLAY_HEIGHT)) {
		unsigned flags = *((state->src)++);

		for (int i = 7; i >= 0; --i) {
			if (state->dest >= state->dest_end)
				return true;

			if (flags & (1 << i)) {
				unsigned width;
				unsigned distance;

				width = (*(state->src) >> 4) + 3;
				distance = (((*(state->src) & 0xF) << 8) | (*((state->src) + 1))) + 1;
				state->src += 2;

				volatile uint8_t* from = state->dest - distance;
				for (; width > 0; --width, ++(state->dest), ++from) {
					*(state->dest) = *(from);
				}
			} else {
				*((state->dest)++) = *((state->src)++);
			}
		}
	}
	return state->dest >= state->dest_end;
}
