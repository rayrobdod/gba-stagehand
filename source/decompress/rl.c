#include "decompress/huff.h"

#include <stdint.h>
#include "decompress/type.h"
#include "gba/hw_reg.h"
#include "gba/screen.h"
#include "utils/arraycount.h"
#include "mgba.h"

bool RLUnCompSuspendable(struct suspended_decompression* state) {
	while (state->dest < state->dest_end && (reg_lcd.VCOUNT < (DISPLAY_HEIGHT - 10) || reg_lcd.VCOUNT > DISPLAY_HEIGHT)) {
		uint8_t flag = *state->src++;

		if (flag & 0x80) {
			unsigned length = (flag & 0x7F) + 3;
			uint8_t value = *state->src++;
			for (unsigned i = 0; i < length; i++) {
				*state->dest++ = value;
			}
		} else {
			unsigned length = (flag & 0x7F) + 1;
			for (unsigned i = 0; i < length; i++) {
				*state->dest++ = *state->src++;
			}
		}
	}

	return state->dest >= state->dest_end;
}

