#include "variable_name_for_image.hpp"

#include <algorithm>

std::string variable_name_for_image(input_path_and_data input) {
	std::string retval;
	auto properties = input_properties(input.second);
	auto manual = properties.find(std::string("Variable"));
	if (manual != properties.end()) {
		retval = manual->second;
	} else {
		std::filesystem::path p = input.first.parent_path();
		for (auto segment : p) {
			retval += segment;
			retval += "_";
		}
		retval += input.first.stem().string();
	}
	std::replace(retval.begin(), retval.end(), '-', '_');
	return retval;
}
