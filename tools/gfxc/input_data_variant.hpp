#ifndef INPUT_DATA_VARIANT_HPP
#define INPUT_DATA_VARIANT_HPP

#include <variant>
#include <filesystem>
#include <map>
#include <utility>
#include "image.hpp"
#include "input_tilemap.hpp"

typedef std::variant<bufferedimage, input_tile16x3map> input_data;
typedef std::pair<std::filesystem::path, input_data> input_path_and_data;

static inline const std::map<std::string, std::string>& input_properties(
		input_data input) {
	return std::visit(
		[](auto&& arg) -> const std::map<std::string, std::string>& {
			return arg.properties();
		},
		input);
}

#endif        //  #ifndef INPUT_DATA_VARIANT_HPP
