#include "gba/shared.h"

void mix_rgb_many_1(
	  volatile rgb_t* dest
	, const rgb_t* from
	, const unsigned count
	, const rgb_t to
	, const unsigned proportion
);


void fade_to_initialize(rgb_t);
void fade_from_initialize(const rgb_t* target, unsigned count);
bool fade_step(void);
