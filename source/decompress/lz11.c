#include "decompress/lz11.h"

#include <stdbool.h>
#include <stdint.h>
#include "mgba.h"

void LZ11UnCompVram(const void* src, volatile void* dest) {
	const uint32_t* src32 = (const uint32_t*)src;
	volatile uint16_t* dest16 = (volatile uint16_t*)dest;
	const uint32_t len = *(src32++) >> 8;
	volatile uint16_t* const dest_end = dest16 + len / 2;

	const uint8_t* src8 = (const uint8_t*)src32;

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

				switch (*src8 >> 4) {
				case 0:
					width = ((*src8 << 4) | ((*(src8 + 1) >> 4) & 0xF)) + 0x11;
					distance = (((*(src8 + 1) & 0xF) << 8) | (*(src8 + 2))) + 1;
					src8 += 3;
					break;
				case 1:
					width = (((*src8 & 0xF) << 12) | (*(src8 + 1) << 4) | ((*(src8 + 2) >> 4) & 0xF)) + 0x111;
					distance = (((*(src8 + 2) & 0xF) << 8) | (*(src8 + 3))) + 1;
					src8 += 4;
					break;
				default:
					width = (*src8 >> 4) + 1;
					distance = (((*src8 & 0xF) << 8) | (*(src8 + 1))) + 1;
					src8 += 2;
					break;
				}

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
