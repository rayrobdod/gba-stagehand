#include "mix_rgb.h"

#include "gba/hw_reg.h"
#include "gba/palette.h"
#include "utils/saturating_add.h"
#include "mgba.h"

void mix_rgb_many_1(
	  volatile rgb_t* dest
	, const rgb_t* from
	, const unsigned count
	, const rgb_t to
	, const unsigned proportion
) {
	// the compiled version does calculate `to.r * proportion` only once.
	// No local variables needed.
	for (unsigned i = 0; i < count; ++i) {
		dest[i] = (rgb_t){
			.r = (to.r * proportion + from[i].r * (16 - proportion)) / 16,
			.g = (to.g * proportion + from[i].g * (16 - proportion)) / 16,
			.b = (to.b * proportion + from[i].b * (16 - proportion)) / 16,
		};
	}
}

[[gnu::section(".sbss")]]
static struct {
	rgb_t from[512];
	rgb_t to;
	uint8_t step;
	int8_t delta;
} fade_data = {0};

static const struct reg_dma dma_fade_to_init = {
	.source = (void*) hw_palette.all,
	.destination = fade_data.from,
	.word_count = 512,
	.control = {
		.dest_control = DMA_ADDR_INCREMENT,
		.src_control = DMA_ADDR_INCREMENT,
		.repeat = false,
		.word_size = WORDSIZE_16BIT,
		.start = DMA_START_IMMEDIATELY,
		.irq = false,
		.enable = true,
	},
};

void fade_to_initialize(rgb_t to) {
	reg_dma[3] = dma_fade_to_init;
	fade_data.to = to;
	fade_data.step = 0;
	fade_data.delta = 2;
}

void fade_from_initialize(const rgb_t* target, unsigned count) {
	reg_dma[3].source = target;
	reg_dma[3].destination = fade_data.from;
	reg_dma[3].word_count = count;
	reg_dma[3].control = (dma_control_t){
		.dest_control = DMA_ADDR_INCREMENT,
		.src_control = DMA_ADDR_INCREMENT,
		.repeat = false,
		.word_size = WORDSIZE_16BIT,
		.start = DMA_START_IMMEDIATELY,
		.irq = false,
		.enable = true,
	},
	fade_data.to = hw_palette.all[0];
	fade_data.step = 16;
	fade_data.delta = -2;
}

bool fade_step() {
	fade_data.step = saturating_add(fade_data.step, 0, 16, fade_data.delta);
	mix_rgb_many_1(
		  hw_palette.all
		, fade_data.from
		, 512
		, fade_data.to
		, fade_data.step
	);
	return fade_data.step == 0 || fade_data.step == 16;
}
