#include "variable_name_for_image.hpp"

std::string variable_name_for_image(std::pair<std::filesystem::path, bufferedimage> image) {
	std::string retval;
	auto manual = image.second.text().find(std::string("Variable"));
	if (manual != image.second.text().end()) {
		retval = manual->second;
	} else {
		std::filesystem::path p = image.first.parent_path();
		for (auto segment : p) {
			retval += segment;
			retval += "_";
		}
		retval += image.first.stem().string();
	}
	std::replace(retval.begin(), retval.end(), '-', '_');
	return retval;
}
