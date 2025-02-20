#include <ostream>
#include <string>

enum sprite_size {
	SIZE_8x8 = 0,
	SIZE_16x16 = 4,
};

enum sprite_size sprite_size(unsigned width, unsigned height);
std::ostream& operator<<(std::ostream& os, enum sprite_size v);

struct sprite {
	std::string var_name;
	uint16_t paltag;
	uint16_t tiletag;
	enum sprite_size size;
};
