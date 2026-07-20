#include "decompress/identity.h"

#include <stdint.h>

void IdentityUnComp(const struct CompressedData* src, volatile void* dest) {
	const uint32_t* src32 = (const uint32_t*)src;
	volatile uint32_t* dest32 = (volatile uint32_t*)dest;
	uint32_t len = *(src32++) >> 8;

	for (; len > 3; len -= 4) {
		*(dest32++) = *(src32++);
	}

	const uint16_t* src16 = (const uint16_t*)src32;
	volatile uint16_t* dest16 = (volatile uint16_t*)dest32;

	for (; len > 1; len -= 2) {
		*(dest16++) = *(src16++);
	}

	for (; len != 0; len--) {
		const uint8_t* src8 = (const uint8_t*)src16;
		volatile uint8_t* dest8 = (volatile uint8_t*)dest16;

		*(dest8++) = *(src8++);
	}
}
