#include "management/keyinput.h"

struct {
	keypad_t down;
	keypad_t new;
} keyinputs = {0};

keypad_t keyinput_get_down(void) {
	return keyinputs.down;
}

keypad_t keyinput_get_new(void) {
	return keyinputs.new;
}

static int keyinput_horizontal(keypad_t k) {
	int retval = 0;
	if (!k.left) retval--;
	if (!k.right) retval++;
	return retval;
}
int keyinput_horizontal_new(void) {return keyinput_horizontal(keyinputs.new);}
int keyinput_horizontal_down(void) {return keyinput_horizontal(keyinputs.down);}

static int keyinput_vertical(keypad_t k) {
	int retval = 0;
	if (!k.up) retval--;
	if (!k.down) retval++;
	return retval;
}
int keyinput_vertical_new(void) {return keyinput_vertical(keyinputs.new);}
int keyinput_vertical_down(void) {return keyinput_vertical(keyinputs.down);}
