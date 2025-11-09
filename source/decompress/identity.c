#include "decompress/identity.h"

#include <stdint.h>
#include "decompress/type.h"
#include "gba/hw_reg.h"
#include "gba/screen.h"
#include "utils/minmax.h"

void IdentityUnComp_FastCopy(
	const uint8_t* src,
	volatile uint8_t* dest,
	unsigned length);

bool IdentityUnCompSuspendable(struct suspended_decompression* state) {
	const int scanlines_left = (
		reg_lcd.VCOUNT < DISPLAY_HEIGHT ?
		DISPLAY_HEIGHT - reg_lcd.VCOUNT :
		VCOUNT_MAX + DISPLAY_HEIGHT - reg_lcd.VCOUNT) - 1;
	// 68330 bytes per frame ÷ 227 scanlines per frame ≈ 301 bytes per scanline
	// To keep dest word-aligned, use multiple of 4 as factor here
	const int max_bytes = scanlines_left * 300;

	const int segment_length = min(max_bytes, state->dest_end - state->dest);

	const bool is_complete = segment_length != max_bytes;

	if (segment_length > 0)
		IdentityUnComp_FastCopy(state->src, state->dest, segment_length);

	state->src += segment_length;
	state->dest += segment_length;
	return is_complete;
}
