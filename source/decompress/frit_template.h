__attribute__((optimize("-O3")))
void FUNCTION_NAME(const struct CompressedData* srcV, volatile void* destV) {
	volatile WORD* dest = (volatile WORD*)destV;
	volatile WORD* const dest_end = dest + (srcV->size / sizeof(WORD));

	const uint8_t* src8 = srcV->data;

	// zero-initializing a `uint_16 x[4]` array translates into a memset call.
	// assigning each item to zero translates into *two* `strs` instructions
	// The latter is 200 cycles faster. out of tens of thousands of cycles, but still.
	WORD regs[4];
	regs[0] = 0;
	regs[1] = 0;
	regs[2] = 0;
	regs[3] = 0;

	while (dest < dest_end) {
		uint8_t op = *(src8++);
		unsigned op_code = op >> 6;

		if (0 == op_code) {
			unsigned from = op >> 4;
			unsigned to = bit_mask_and_shift(op, 2, 2);
			unsigned hi = (op & 0x2);
			unsigned low = (op & 0x1);
			unsigned hiValue = _Generic(
				(WORD) 1,
				uint16_t :
					(hi ? *(src8++) << 8 : 0),
				uint8_t :
					(hi ? src8++, 0 : 0)
			);
			unsigned lowValue = (low ? *(src8++) : 0);
			unsigned operand = hiValue | lowValue;

			regs[to] = regs[from] ^ operand;
		} else {
			unsigned regId = bit_mask_and_shift(op, 2, 4);
			unsigned length = (op & 0x0F) + 1;
			if (length == 0x10) {
				length = *(src8++) + 31;
			}
			unsigned regValue = regs[regId];

			if (2 == op_code) {
				// Did try 32-bit writes, but I guess the branching to align is slower than just doing more 16-bit writes

				// Benchmarks are happier if this loop ends with `length > 0` guaranteed
				for (; length > 4; length -= 4) {
					*(dest++) = regValue;
					*(dest++) = regValue;
					*(dest++) = regValue;
					*(dest++) = regValue;
				}
				for (; length > 0; --length) {
					*(dest++) = regValue;
				}
			} else if (1 == op_code) {
				signed delta = ((signed) op_code) - 2;

				// Benchmarks are happier if this loop ends with `length > 0` guaranteed
				for (; length > 4; length -= 4) {
					*(dest+0) = regValue;
					*(dest+1) = regValue + delta;
					*(dest+2) = regValue + delta * 2;
					*(dest+3) = regValue + delta * 3;
					dest += 4;
					regValue += delta * 4;
				}
				for (; length > 0; --length) {
					*(dest++) = regValue;
					regValue += delta;
				}
				regs[regId] = regValue;
			} else {
				signed delta = ((signed) op_code) - 2;
				// looking at assembly indicates that gcc does know that `op_code == 3` and therefore `delta == 1` here
				// Being able to `+ 3` or `- 3` instead of `+ 3 * delta` seems to be a significant speedup

				// Benchmarks are happier if this loop ends with `length > 0` guaranteed
				for (; length > 4; length -= 4) {
					*(dest+0) = regValue;
					*(dest+1) = regValue + delta;
					*(dest+2) = regValue + delta * 2;
					*(dest+3) = regValue + delta * 3;
					dest += 4;
					regValue += delta * 4;
				}
				for (; length > 0; --length) {
					*(dest++) = regValue;
					regValue += delta;
				}
				regs[regId] = regValue;
			}
		}
	}
}

bool FUNCTION_NAME_SUSPEND(struct suspended_decompression* state) {
	volatile WORD* dest = (volatile WORD*)state->dest;
	volatile WORD* const dest_end = (volatile WORD*)state->dest_end;
	const uint8_t* src = state->src;

	while (dest < dest_end && (reg_lcd.VCOUNT < (DISPLAY_HEIGHT - 10) || reg_lcd.VCOUNT > DISPLAY_HEIGHT)) {
		uint8_t op = *(src++);
		unsigned op_code = op >> 6;

		if (0 == op_code) {
			unsigned from = op >> 4;
			unsigned to = bit_mask_and_shift(op, 2, 2);
			unsigned hi = (op & 0x2);
			unsigned low = (op & 0x1);
			unsigned hiValue = _Generic(
				(WORD) 1,
				uint16_t :
					(hi ? *(src++) << 8 : 0),
				uint8_t :
					(hi ? src++, 0 : 0)
			);
			unsigned lowValue = (low ? *(src++) : 0);
			unsigned operand = hiValue | lowValue;

			state->regs[to] = state->regs[from] ^ operand;
		} else {
			unsigned regId = bit_mask_and_shift(op, 2, 4);
			unsigned length = (op & 0x0F) + 1;
			if (length == 0x10) {
				length = *(src++) + 31;
			}
			unsigned regValue = state->regs[regId];

			if (2 == op_code) {
				for (unsigned i = 0; i < length; i++) {
					*(dest++) = regValue;
				}
			} else if (1 == op_code) {
				signed delta = ((signed) op_code) - 2;

				for (unsigned i = 0; i < length; i++) {
					*(dest++) = regValue;
					regValue += delta;
				}
				state->regs[regId] = regValue;
			} else {
				signed delta = ((signed) op_code) - 2;
				// looking at assembly indicates that gcc does know that `op_code == 3` and therefore `delta == 1` here
				// Being able to `+ 3` or `- 3` instead of `+ 3 * delta` seems to be a significant speedup

				for (unsigned i = 0; i < length; i++) {
					*(dest++) = regValue;
					regValue += delta;
				}
				state->regs[regId] = regValue;
			}
		}
	}

	state->dest = (volatile uint8_t*)dest;
	state->src = src;
	return state->dest >= state->dest_end;
}
