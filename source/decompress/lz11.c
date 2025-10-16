#include "decompress/lz11.h"

#include <stdint.h>
#include "decompress/type.h"
#include "gba/hw_reg.h"
#include "gba/screen.h"
#include "mgba.h"

static inline void parseCopyInstruction(unsigned* width, unsigned* distance, const uint8_t** src8) {
	switch (**src8 >> 4) {
	case 0:
		*width = ((**src8 << 4) | ((*(*src8 + 1) >> 4) & 0xF)) + 0x11;
		*distance = (((*(*src8 + 1) & 0xF) << 8) | (*(*src8 + 2))) + 1;
		*src8 += 3;
		break;
	case 1:
		*width = (((**src8 & 0xF) << 12) | (*(*src8 + 1) << 4) | ((*(*src8 + 2) >> 4) & 0xF)) + 0x111;
		*distance = (((*(*src8 + 2) & 0xF) << 8) | (*(*src8 + 3))) + 1;
		*src8 += 4;
		break;
	default:
		*width = (**src8 >> 4) + 1;
		*distance = (((**src8 & 0xF) << 8) | (*(*src8 + 1))) + 1;
		*src8 += 2;
		break;
	}
}

void LZ11UnCompVram(const struct CompressedData* src, volatile void* dest) {
	volatile uint16_t* dest16 = (volatile uint16_t*)dest;
	volatile uint16_t* const dest_end = dest16 + (src->size / sizeof(uint16_t));

	const uint8_t* src8 = src->data;

	uint16_t buffer;
	bool buffer_has_value = false;

	while (dest16 < dest_end) {
		unsigned flags = *(src8++);

		for (int i = 7; i >= 0; --i) {
			if (dest16 >= dest_end)
				break;

			if (flags & (1 << i)) {
				unsigned width;
				unsigned distance;

				parseCopyInstruction(&width, &distance, &src8);

				if (distance % 2 == 0) {
					if (buffer_has_value) {
						buffer |= (*(dest16 - (distance / 2))) & 0xFF00;
						*(dest16++) = buffer;
						--width;
					}
					while (width >= 2) {
						*dest16 = *(dest16 - distance / 2);
						++dest16;
						width -= 2;
					}
					buffer_has_value = width != 0;
					if (buffer_has_value) {
						buffer = *(dest16 - distance / 2) & 0xFF;
					}
				} else {
					if (buffer_has_value) {
						buffer |= (*(dest16 - (distance / 2)) << 8);
						*(dest16++) = buffer;
						--width;
					}
					while (width >= 2) {
						buffer = *(dest16 - 1 - distance / 2) >> 8;
						buffer |= (*(dest16 - (distance / 2)) << 8);
						*(dest16++) = buffer;
						width -= 2;
					}
					buffer_has_value = width != 0;
					if (buffer_has_value) {
						buffer = *(dest16 - 1 - distance / 2) >> 8;
					}
				}
			} else {
				if (buffer_has_value) {
					*(dest16++) = (buffer) | *(src8++) << 8;
				} else {
					buffer = *(src8++);
				}
				buffer_has_value = ! buffer_has_value;
			}
		}
	}
}

void LZ11UnCompWram(const struct CompressedData* src, volatile void* dest) {
	volatile uint8_t* dest8 = (volatile uint8_t*)dest;
	volatile uint8_t* const dest_end = dest + (src->size / sizeof(uint8_t));

	const uint8_t* src8 = src->data;

	while (dest8 < dest_end) {
		unsigned flags = *(src8++);

		for (int i = 7; i >= 0; --i) {
			if (dest8 >= dest_end)
				break;

			if (flags & (1 << i)) {
				unsigned width;
				unsigned distance;

				parseCopyInstruction(&width, &distance, &src8);

				volatile uint8_t* from = dest8 - distance;
				for (; width > 7; width -= 8, dest8 += 8, from += 8) {
					*(dest8 + 0) = *(from + 0);
					*(dest8 + 1) = *(from + 1);
					*(dest8 + 2) = *(from + 2);
					*(dest8 + 3) = *(from + 3);
					*(dest8 + 4) = *(from + 4);
					*(dest8 + 5) = *(from + 5);
					*(dest8 + 6) = *(from + 6);
					*(dest8 + 7) = *(from + 7);
				}
				for (; width > 0; --width, ++dest8, ++from) {
					*(dest8) = *(from);
				}
			} else {
				*(dest8++) = *(src8++);
			}
		}
	}
}

bool LZ11UnCompSuspendable(struct suspended_decompression* state) {
	while (state->dest < state->dest_end && (reg_lcd.VCOUNT < (DISPLAY_HEIGHT - 15) || reg_lcd.VCOUNT > DISPLAY_HEIGHT)) {
		unsigned flags = *((state->src)++);

		for (int i = 7; i >= 0; --i) {
			if (state->dest >= state->dest_end)
				return true;

			if (flags & (1 << i)) {
				unsigned width;
				unsigned distance;

				parseCopyInstruction(&width, &distance, &state->src);

				volatile uint8_t* from = state->dest - distance;
				/* ???: How much does speed matter here?
				for (; width > 7; width -= 8, state->dest += 8, from += 8) {
					*(state->dest + 0) = *(from + 0);
					*(state->dest + 1) = *(from + 1);
					*(state->dest + 2) = *(from + 2);
					*(state->dest + 3) = *(from + 3);
					*(state->dest + 4) = *(from + 4);
					*(state->dest + 5) = *(from + 5);
					*(state->dest + 6) = *(from + 6);
					*(state->dest + 7) = *(from + 7);
				}
				*/
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
