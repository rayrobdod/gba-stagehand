#include "decompress/lz11.h"

#include <stdbool.h>
#include <stdint.h>
#include "mgba.h"

void LZ11UnCompVram(const void* src, volatile void* dest) {
	const uint32_t* src32 = (const uint32_t*)src;
	volatile uint16_t* dest16 = (volatile uint16_t*)dest;
	_Static_assert(2 == sizeof(uint16_t));
	volatile uint16_t* const dest_end = dest16 + (*(src32++) >> 9);

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

void LZ11UnCompWram(const void* srcV, volatile void* destV) {
	const uint32_t* src32 = (const uint32_t*)srcV;
	volatile uint8_t* dest = (volatile uint8_t*)destV;
	_Static_assert(1 == sizeof(uint8_t));
	volatile uint8_t* const dest_end = dest + (*(src32++) >> 8);

	const uint8_t* src8 = (const uint8_t*)src32;

	while (dest < dest_end) {
		unsigned flags = *(src8++);

		for (int i = 7; i >= 0; --i) {
			if (dest >= dest_end)
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

				volatile uint8_t* from = dest - distance;
				for (; width > 7; width -= 8, dest += 8, from += 8) {
					*(dest + 0) = *(from + 0);
					*(dest + 1) = *(from + 1);
					*(dest + 2) = *(from + 2);
					*(dest + 3) = *(from + 3);
					*(dest + 4) = *(from + 4);
					*(dest + 5) = *(from + 5);
					*(dest + 6) = *(from + 6);
					*(dest + 7) = *(from + 7);
				}
				for (; width > 0; --width, ++dest, ++from) {
					*(dest) = *(from);
				}
			} else {
				*(dest++) = *(src8++);
			}
		}
	}
}
