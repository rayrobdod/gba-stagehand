#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "mode_raw.h"
#include "mode_tileset.h"

#define arraycount(a) (sizeof(a) / sizeof(a[0]))

const struct {
	const char* id;
	int (*runner)(int argc, char** argv);
} modes[] = {
	{"raw", mode_raw},
	{"tileset", mode_tileset},
};


int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("TODO \n");
		return 0;
	}

	char* mode_id = argv[1];

	for (size_t i = 0; i < arraycount(modes); i++) {
		if (0 == strcmp(mode_id, modes[i].id)) {
			return modes[i].runner(argc - 2, argv + 2);
		}
	}

	printf("unknown mode: %s\n", mode_id);
	return -1;
}
