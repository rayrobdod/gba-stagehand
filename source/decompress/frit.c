#include "decompress/frit.h"

#include <stdint.h>

__attribute__((optimize("-O3")))
void Frit16UnComp(const void* src, volatile void* dest) {
	const uint32_t* src32 = (const uint32_t*)src;
	volatile uint16_t* dest16 = (volatile uint16_t*)dest;
	const uint32_t len = *(src32++) >> 8;
	volatile uint16_t* const dest_end = dest16 + len / sizeof(uint16_t);

	const uint8_t* src8 = (const uint8_t*)src32;

	uint16_t regs[4] = {0};

	while (dest16 < dest_end) {
		uint8_t op = *(src8++);
		unsigned op_code = op >> 6;

		if (0 == op_code) {
			unsigned from = (op & 0x30) >> 4;
			unsigned to = (op & 0xC) >> 2;
			unsigned hi = (op & 0x2);
			unsigned low = (op & 0x1);
			unsigned hiValue = (hi ? *(src8++) << 8 : 0);
			unsigned lowValue = (low ? *(src8++) : 0);
			unsigned operand = hiValue | lowValue;

			regs[to] = regs[from] ^ operand;
		} else {
			unsigned regId = (op & 0x30) >> 4;
			unsigned length = (op & 0x0F) + 1;
			if (length == 0x10) {
				length = *(src8++) + 31;
			}
			unsigned regValue = regs[regId];

			if (2 == op_code) {
				for (; length > 0; --length) {
					*(dest16++) = regValue;
				}
			} else {
				signed delta = ((signed) op_code) - 2;

				for (; length > 0; --length) {
					*(dest16++) = regValue;
					regValue += delta;
				}
				regs[regId] = regValue;
			}
		}
	}
}
