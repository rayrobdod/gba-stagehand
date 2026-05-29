#include "decompress/lz16.h"

#include <stdint.h>
#include "decompress/type.h"
#include "gba/hw_reg.h"
#include "gba/screen.h"
#include "mgba.h"

//[[gnu::section(".iwram")]]
void LZ16UnComp(const struct CompressedData* src, volatile void* dest) {
	volatile uint16_t* dest16 = (volatile uint16_t*)dest;
	volatile uint16_t* const dest_end = dest16 + (src->size / sizeof(uint16_t));

	const uint16_t* src16 = (const uint16_t*)src->data;

	while (dest16 < dest_end) {
		unsigned flags = *src16++;

		for (int i = 15; i >= 0; --i) {
			if (dest16 >= dest_end)
				break;

			if (flags & (1 << i)) {
				unsigned width = 3 + (*src16 >> 12);
				unsigned distance = 1 + (*src16 & 0xFFF);
				src16 += 1;

				volatile uint16_t* from = dest16 - distance;
				for (; width > 7; width -= 8, dest16 += 8, from += 8) {
					*(dest16 + 0) = *(from + 0);
					*(dest16 + 1) = *(from + 1);
					*(dest16 + 2) = *(from + 2);
					*(dest16 + 3) = *(from + 3);
					*(dest16 + 4) = *(from + 4);
					*(dest16 + 5) = *(from + 5);
					*(dest16 + 6) = *(from + 6);
					*(dest16 + 7) = *(from + 7);
				}
				for (; width > 0; --width, ++dest16, ++from) {
					*dest16 = *from;
				}
			} else {
				*dest16++ = *src16++;
			}
		}
	}
}

bool LZ16UnCompSuspendable(struct suspended_decompression* state) {
	const uint16_t* src16 = (const uint16_t*)state->src;
	volatile uint16_t* dest16 = (volatile uint16_t*)state->dest;
	volatile uint16_t* const dest_end = (volatile uint16_t*)state->dest_end;

	while (dest16 < dest_end && (reg_lcd.VCOUNT < (DISPLAY_HEIGHT - 15) || reg_lcd.VCOUNT >= DISPLAY_HEIGHT)) {
		unsigned flags = *src16++;

		for (int i = 15; i >= 0; --i) {
			if (dest16 >= dest_end) {
				state->dest = (volatile uint8_t*) dest16;
				state->src = (const uint8_t*) src16;
				return true;
			}

			if (flags & (1 << i)) {
				unsigned width = 3 + (*src16 >> 12);
				unsigned distance = 1 + (*src16 & 0xFFF);
				src16 += 1;

				volatile uint16_t* from = dest16 - distance;
				for (; width > 0; --width, ++dest16, ++from) {
					*dest16 = *from;
				}
			} else {
				*dest16++ = *src16++;
			}
		}
	}
	state->dest = (volatile uint8_t*) dest16;
	state->src = (const uint8_t*) src16;
	return state->dest >= state->dest_end;
}
