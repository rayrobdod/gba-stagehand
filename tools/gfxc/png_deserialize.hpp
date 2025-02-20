#ifndef PNG_DESERIALIZE_HPP
#define PNG_DESERIALIZE_HPP

#include <string>
#include "image.hpp"

struct bufferedimage png_deserialize(
	const std::string& file_name);

#endif //  #ifndef PNG_DESERIALIZE_HPP
