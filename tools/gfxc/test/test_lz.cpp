#include <algorithm>
#include <cstdio>
#include "compression/lz.hpp"

static std::vector<uint8_t>::size_type hd_columns = 16;

int main(int argc, char** argv) {
	std::vector<uint8_t> src{0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3};
	auto res = compressLz(src);
	for (unsigned i = 0; i < res.size(); i += hd_columns) {
		printf("%06X | ", i);
		for (unsigned j = 0; j < std::min(hd_columns, res.size() - i); j++) {
			printf("%02X ", res[i + j]);
		}
		printf("\n");
	}
	return 0;
}

#include "compression/lz.cpp"
