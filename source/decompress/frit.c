#include "decompress/frit.h"

#include <stdint.h>
#include "decompress/type.h"
#include "gba/hw_reg.h"
#include "gba/screen.h"

static inline unsigned bit_mask_and_shift(unsigned value, unsigned keep, unsigned shift) {
	unsigned retval = value;
	retval <<= (32 - (shift + keep));
	retval >>= (32 - keep);
	return retval;
}

#define FUNCTION_NAME Frit16UnComp
#define FUNCTION_NAME_SUSPEND Frit16UnCompSuspendable
#define WORD uint16_t
#include "decompress/frit_template.h"
#undef FUNCTION_NAME
#undef FUNCTION_NAME_SUSPEND
#undef WORD

#define FUNCTION_NAME Frit8UnCompWram
#define FUNCTION_NAME_SUSPEND Frit8UnCompSuspendable
#define WORD uint8_t
#include "decompress/frit_template.h"
#undef FUNCTION_NAME
#undef FUNCTION_NAME_SUSPEND
#undef WORD

__attribute__((optimize("-O3")))
void Frit8UnCompVram(const struct CompressedData* srcV, volatile void* destV) {
	volatile uint16_t* dest16 = (volatile uint16_t*)destV;
	volatile uint16_t* const dest_end = dest16 + (srcV->size / sizeof(uint16_t));

	const uint8_t* src8 = srcV->data;

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

				if (length > 0 && buffer_has_value) {
					buffer |= regValue << 8;
					*(dest16++) = buffer;
					length--;
					regValue += delta;
				}

				for (; length > 1; length -= 2) {
					buffer = regValue & 0xFF;
					regValue += delta;
					buffer |= regValue << 8;
					regValue += delta;
					*(dest16++) = buffer;
				}

				buffer_has_value = length > 0;
				if (buffer_has_value) {
					buffer = regValue & 0xFF;
					regValue += delta;
				}
				regs[regId] = regValue;
			}
		}
	}
}
