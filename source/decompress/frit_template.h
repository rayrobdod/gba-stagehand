__attribute__((optimize("-O3")))
void FUNCTION_NAME(const void* srcV, volatile void* destV) {
	const uint32_t* src32 = (const uint32_t*)srcV;
	volatile WORD* dest = (volatile WORD*)destV;
	volatile WORD* const dest_end = dest + (*(src32++) >> 8) / sizeof(WORD);

	const uint8_t* src8 = (const uint8_t*)src32;

	WORD regs[4] = {0};

	while (dest < dest_end) {
		uint8_t op = *(src8++);
		unsigned op_code = op >> 6;

		if (0 == op_code) {
			unsigned from = (op & 0x30) >> 4;
			unsigned to = (op & 0xC) >> 2;
			unsigned hi = (op & 0x2);
			unsigned low = (op & 0x1);
			unsigned hiValue;
			if (2 == sizeof(WORD)) {
				hiValue = (hi ? *(src8++) << 8 : 0);
			} else if (1 == sizeof(WORD)) {
				if (hi) {
					src8++;
				}
				hiValue = 0;
			} else {
				// I want "if this else branch is not constant-folded away"
				_Static_assert(2 == sizeof(WORD) || 1 == sizeof(WORD), "word size must be 1 or 2");
			}
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
					*(dest++) = regValue;
				}
			} else {
				signed delta = ((signed) op_code) - 2;

				for (; length > 0; --length) {
					*(dest++) = regValue;
					regValue += delta;
				}
				regs[regId] = regValue;
			}
		}
	}
}
