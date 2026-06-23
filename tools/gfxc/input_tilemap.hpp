#ifndef INPUT_TILEMAP_HPP
#define INPUT_TILEMAP_HPP

#include "image.hpp"

extern const std::initializer_list<std::string_view> walkaround_behavior_names;

struct input_tile16x3 {
	unsigned behavior_id;
	std::array<std::vector<rgba16_t>, 3> pixelss;
};

class input_tile16x3map {
public:
	struct sign {int x; int y; std::string message;};
private:
	const size_t _width;
	const size_t _height;
	const std::vector<input_tile16x3> _tiles;
	const std::vector<sign> _signs;
	const std::map<std::string, std::string> _properties;
public:
	input_tile16x3map(
		size_t width,
		size_t height,
		const std::vector<input_tile16x3>& tiles,
		const std::vector<sign>& _signs,
		const std::map<std::string, std::string>& properties);

	unsigned width() const;
	unsigned height() const;
	const input_tile16x3& tile(size_t x, size_t y) const;

	const std::vector<input_tile16x3>& tiles() const;
	const std::vector<sign>& signs() const;
	const std::map<std::string, std::string>& properties() const;
};

input_tile16x3map tilemap_deserialize(
	const std::string& file_name);


#endif        //  #ifndef INPUT_TILEMAP_HPP
