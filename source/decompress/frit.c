#include "decompress/frit.h"

#include <stdbool.h>
#include <stdint.h>

static inline unsigned bit_mask_and_shift(unsigned value, unsigned keep, unsigned shift) {
	unsigned retval = value;
	retval <<= (32 - (shift + keep));
	retval >>= (32 - keep);
	return retval;
}

#define FUNCTION_NAME Frit16UnComp
#define WORD uint16_t
#include "decompress/frit_template.h"
#undef FUNCTION_NAME
#undef WORD

#define FUNCTION_NAME Frit8UnCompWram
#define WORD uint8_t
#include "decompress/frit_template.h"
#undef FUNCTION_NAME
#undef WORD

__attribute__((optimize("-O3")))
void Frit8UnCompVram(const void* src, volatile void* dest) {
	const uint32_t* src32 = (const uint32_t*)src;
	volatile uint16_t* dest16 = (volatile uint16_t*)dest;
	_Static_assert(2 == sizeof(uint16_t));
	volatile uint16_t* const dest_end = dest16 + (*(src32++) >> 9);

	const uint8_t* src8 = (const uint8_t*)src32;

	unsigned buffer = 0;
	bool buffer_has_value = false;

	uint8_t regs[4] = {0};

	while (dest16 < dest_end) {
		uint8_t op = *(src8++);
		unsigned op_code = op >> 6;

		if (0 == op_code) {
			unsigned from = op >> 4;
			unsigned to = bit_mask_and_shift(op, 2, 2);
			unsigned hi = (op & 0x2);
			unsigned low = (op & 0x1);
			if (hi) {src8++;}
			unsigned operand = (low ? *(src8++) : 0);

			regs[to] = regs[from] ^ operand;
		} else {
			unsigned regId = bit_mask_and_shift(op, 2, 4);
			unsigned length = (op & 0x0F) + 1;
			if (length == 0x10) {
				length = *(src8++) + 31;
			}
			unsigned regValue = regs[regId];

			if (2 == op_code) {
				if (length > 0 && buffer_has_value) {
					buffer |= regValue << 8;
					*(dest16++) = buffer;
					length--;
				}

				buffer = (regValue << 8) | regValue;

				for (; length > 1; length -= 2) {
					*(dest16++) = buffer;
				}

				buffer_has_value = length > 0;
				if (buffer_has_value) {
					buffer = regValue;
				}
			} else {
				signed delta = ((signed) op_code) - 2;
				signed deltaSIMD = delta * 0x0202;

				if (length > 0 && buffer_has_value) {
					buffer |= regValue << 8;
					*(dest16++) = buffer;
					length--;
					regValue += delta;
				}

				if (length > 1) {
					buffer = regValue;
					regValue += delta;
					buffer |= regValue << 8;
					regValue += delta;
					*(dest16++) = buffer;
					length -= 2;

					for (; length > 1; length -= 2) {
						buffer += deltaSIMD;
						regValue += delta * 2;
						*(dest16++) = buffer;
					}
				}

				buffer_has_value = length > 0;
				if (buffer_has_value) {
					buffer = regValue;
					regValue += delta;
				}
				regs[regId] = regValue;
			}
		}
	}
}
