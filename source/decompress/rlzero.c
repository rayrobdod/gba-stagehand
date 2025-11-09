#include "decompress/huff.h"

#include <stdint.h>
#include "decompress/type.h"
#include "gba/hw_reg.h"
#include "gba/screen.h"
#include "utils/arraycount.h"
#include "mgba.h"

bool RlZeroUnCompSuspendable(struct suspended_decompression* state) {
	const uint16_t* src16 = (const uint16_t*) state->src;
	volatile uint16_t* dest16 = (volatile uint16_t*) state->dest;
	volatile uint16_t* dest_end16 = (volatile uint16_t*) state->dest_end;

	while (dest16 < dest_end16 && (reg_lcd.VCOUNT < (DISPLAY_HEIGHT - 10) || reg_lcd.VCOUNT > DISPLAY_HEIGHT)) {
		uint16_t counts = *src16++;
		unsigned countCopy = counts >> 8;
		unsigned countZero = counts & 0xFF;

		for (unsigned i = 0; i < countZero; i++) {
			*dest16++ = 0;
		}
		for (unsigned i = 0; i < countCopy; i++) {
			*dest16++ = *src16++;
		}
	}

	state->src = (const uint8_t*) src16;
	state->dest = (volatile uint8_t*) dest16;

	return state->dest >= state->dest_end;
}

