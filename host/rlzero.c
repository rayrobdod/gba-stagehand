#include "decompress/rlzero.h"

#include <stdint.h>
#include "decompress/type.h"

void RlZeroUnComp(const struct CompressedData* src, volatile void* dest) {
	volatile uint16_t* dest16 = (volatile uint16_t*)dest;
	volatile uint16_t* const dest_end = dest16 + (src->size / sizeof(uint16_t));

	const uint16_t* src16 = (const uint16_t*) src->data;

	while (dest16 < dest_end) {
		uint16_t cmd = *src16++;

		uint8_t zeros = cmd & 0xFF;
		for (unsigned i = 0; i < zeros; i++) {
			*dest16++ = 0;
		}
		uint8_t copies = cmd >> 8;
		for (unsigned i = 0; i < copies; i++) {
			*dest16++ = *src16++;
		}
	}
}
