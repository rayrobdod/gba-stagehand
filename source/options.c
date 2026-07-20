#include "options.h"

#include "utils/arraycount.h"
#include "graphics.h"
#include "memcpy_sram.h"

__attribute__((section(".sbss")))
struct options options = {0};

__attribute__((section(".sram.options")))
static volatile struct options_versioned {
	uint8_t version;
	union {
		uint8_t generic[0x20-1];
		struct options v0;
	};
} options_sram = {0};


static const struct tileset* const options_frames[OPTIONS_FRAMES_COUNT] = {
	&dialog_frames_1,
	&dialog_frames_2,
	&dialog_frames_pcwindow,
	&dialog_frames_whitehot,
};


const struct tileset *options_frame_get_n(unsigned index) {
	if (index >= arraycount(options_frames)) {
		index = 0;
	}
	return options_frames[index];
}
const struct tileset *options_frame_get(void) {
	return options_frame_get_n(options.frame_index);
}

void options_commit_to_save(void) {
	struct options_versioned tmp = {
		.version = 0,
		.v0 = options,
	};
	memcpy_to_sram(&options_sram, &tmp, sizeof(struct options_versioned));
}

void options_load_from_save(void) {
	struct options_versioned tmp;
	memcpy_from_sram(&tmp, &options_sram, sizeof(struct options_versioned));
	switch (tmp.version) {
	case 0:
		options = tmp.v0;
		break;
	default:
		options = (struct options) {0};
		break;
	}
}
