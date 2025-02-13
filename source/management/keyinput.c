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
