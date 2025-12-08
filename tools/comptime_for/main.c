#include <stdio.h>

#define GUARD "FOR_COMPTIME_H"
#define LOOP_BODY "X"

int main([[gnu::unused]] int argc, [[gnu::unused]] char* argv[]) {
	printf("#ifndef " GUARD "\n");
	printf("#define " GUARD "\n");
	printf("\n");

	printf("#define FOR_1_TO_1(" LOOP_BODY ")    " LOOP_BODY "(1)\n");
	for (unsigned i = 2; i < 128; i++) {
		char current[40];
		char prev[40];

		snprintf(current, sizeof(current), "FOR_1_TO_%d(" LOOP_BODY ")", i);
		snprintf(prev, sizeof(prev), "FOR_1_TO_%d(" LOOP_BODY ")", i - 1);

		printf("#define %-16s %-16s " LOOP_BODY "(%d)\n", current, prev, i);
	}

	printf("\n");
	printf("#endif // " GUARD "\n");
	return 0;
}
