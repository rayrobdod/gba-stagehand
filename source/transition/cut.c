#include "transition/cut.h"

#include <stdlib.h>
#include <string.h>
#include "management/vram_op_queue.h"

void transition_cut_initIn(union palette512 palette) {
	palette16_t* buffer = malloc(sizeof(rgb_t) * 512);
	if (buffer) {
		memcpy(buffer, palette.all, sizeof(rgb_t) * 512);
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
