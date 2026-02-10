#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdint.h>

#define OPTIONS_FRAMES_COUNT (4)

struct tileset;

struct options {
	uint8_t frame_index;
};

extern struct options options;

const struct tileset* options_frame_get(void);
const struct tileset* options_frame_get_n(unsigned);

#endif        //  #ifndef OPTIONS_H
