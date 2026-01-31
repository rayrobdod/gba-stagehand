#include <algorithm>
#include <set>
#include "image.hpp"

template<class PALS>
uint16_t find_palette_superset(PALS haystack, std::set<rgba16_t> needle) {
	uint16_t retval;
	for (retval = 0; retval < haystack.size(); ++retval) {
		const auto check_pal = haystack[retval];

		if (std::includes(check_pal.begin(), check_pal.end(), needle.begin(), needle.end())) {
			break;
		}
	}
	if (retval >= haystack.size()) {
		std::string msg("Lost this sprite's palette");
		throw std::logic_error(msg);
	}
	return retval;
}
