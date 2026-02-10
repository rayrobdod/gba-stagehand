#include "transition/cut.h"

#include <stdlib.h>
#include "gba/bios.h"
#include "management/vram_op_queue.h"

void transition_cut_initIn(const union palette512* palette) {
	palette16_t* buffer = malloc(sizeof(palette16_t) * 32);
	if (buffer) {
		CpuFastCopy(
			palette->all,
			buffer,
			sizeof(palette16_t) * 32 / sizeof(uint32_t));
		vram_op_queue_enqueue(&(struct vram_op) {
			.type = VRAM_QUEUE_OP_BG_PALETTES_FREE,
			.palettes_free = {
				.from = buffer,
				.to_palette = 0,
				.count = 32,
			},
		});
	}
};

const struct transition transition_cut = {
	.initFadeOut = NULL,
	.fadeOut = NULL,
	.initFadeIn = transition_cut_initIn,
	.fadeIn = NULL,
};
