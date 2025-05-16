#include <cstdint>
#include <ostream>
#include <string>

enum sprite_size {
	SIZE_8x8 = 0,
	SIZE_16x16 = 4,
	SIZE_32x32 = 8,
	SIZE_64x64 = 12,
	SIZE_16x8 = 1,
	SIZE_32x8 = 5,
	SIZE_32x16 = 9,
	SIZE_64x32 = 13,
	SIZE_8x16 = 2,
	SIZE_8x32 = 6,
	SIZE_16x32 = 10,
	SIZE_32x64 = 14,
};

enum sprite_size sprite_size(unsigned width, unsigned height);
std::ostream& operator<<(std::ostream& os, enum sprite_size v);

struct sprite {
	std::string var_name;
	uint16_t paltag;
	uint16_t tiletag;
	enum sprite_size size;
};
