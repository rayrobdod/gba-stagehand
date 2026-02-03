#include "transition/palette_fade.h"
#include "mix_rgb.h"

static void transition_paletteFade_initOut_black(void) {
	fade_to_initialize(rgb(0, 0, 0));
};
static void transition_paletteFade_initOut__21_13_17(void) {
	fade_to_initialize(rgb(21, 13, 17));
};
static void transition_paletteFade_initOut_coral(void) {
	fade_to_initialize(rgb(31, 16, 16));
};
static void transition_paletteFade_initOut_deepskyblue(void) {
	fade_to_initialize(rgb(0, 23, 31));
};
static void transition_paletteFade_initOut_dodgerblue(void) {
	fade_to_initialize(rgb(4, 18, 31));
};
static enum progress transition_paletteFade(void) {
	if (fade_step()) {
		return COMPLETE;
	} else {
		return ONGOING;
	}
};
static void transition_paletteFade_initIn(union palette512 palette) {
	fade_from_initialize(palette.all, 512);
};

const struct transition transition_paletteFade_black = {
	.initFadeOut = transition_paletteFade_initOut_black,
	.fadeOut = transition_paletteFade,
	.initFadeIn = transition_paletteFade_initIn,
	.fadeIn = transition_paletteFade,
};
const struct transition transition_paletteFade__21_13_17 = {
	.initFadeOut = transition_paletteFade_initOut__21_13_17,
	.fadeOut = transition_paletteFade,
	.initFadeIn = transition_paletteFade_initIn,
	.fadeIn = transition_paletteFade,
};
const struct transition transition_paletteFade_coral = {
	.initFadeOut = transition_paletteFade_initOut_coral,
	.fadeOut = transition_paletteFade,
	.initFadeIn = transition_paletteFade_initIn,
	.fadeIn = transition_paletteFade,
};
const struct transition transition_paletteFade_deepskyblue = {
	.initFadeOut = transition_paletteFade_initOut_deepskyblue,
	.fadeOut = transition_paletteFade,
	.initFadeIn = transition_paletteFade_initIn,
	.fadeIn = transition_paletteFade,
};
const struct transition transition_paletteFade_dodgerblue = {
	.initFadeOut = transition_paletteFade_initOut_dodgerblue,
	.fadeOut = transition_paletteFade,
	.initFadeIn = transition_paletteFade_initIn,
	.fadeIn = transition_paletteFade,
};
