#include "options.h"

#include "utils/arraycount.h"
#include "graphics.h"

__attribute__((section(".sbss")))
struct options options = {0};


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
