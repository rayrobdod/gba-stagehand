#include "mode_raw.h"

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include "parse_arguments.h"
#include "object.h"

struct RawArguments {
	const char* in_data_file;
	const char* out_object_file;
	const char* out_header_file;
	const char* variable_name;
};

static void set_in_data_file(void* accumulator, const char* arg) {
	((struct RawArguments*) accumulator)->in_data_file = arg;
}
static void set_out_object_file(void* accumulator, const char* arg) {
	((struct RawArguments*) accumulator)->out_object_file = arg;
}
static void set_out_header_file(void* accumulator, const char* arg) {
	((struct RawArguments*) accumulator)->out_header_file = arg;
}
static void set_variable_name(void* accumulator, const char* arg) {
	((struct RawArguments*) accumulator)->variable_name = arg;
}

const struct ArgumentsAndFlags argsTemplate = {
	.flags_count = 0,
	.flags = NULL,
	.args_count = 4,
	.arguments = (struct Argument[]) {
		{"--in_data", set_in_data_file},
		{"--out_object", set_out_object_file},
		{"--out_header", set_out_header_file},
		{"--variable_name", set_variable_name},
	}
};

int mode_raw(int argc, char* argv[]) {
	struct RawArguments args = {0};
	parseArguments(argc, argv, &args, &argsTemplate);

	struct stat st;
	stat(args.in_data_file, &st);
	unsigned in_data_file_size = st.st_size;

	if (args.out_header_file) {
		FILE* f = fopen(args.out_header_file, "w");
		if (!f) {
			fprintf(stderr, "Could not open file %s\n", args.out_header_file);
			exit(1);
		}
		fprintf(f, "extern uint8_t %s[%d];\n", args.variable_name, in_data_file_size);
		fclose(f);
	}
	if (args.out_object_file) {
		struct Object* object = object_start(args.out_object_file);

		object_push_file_copy_section(object, args.variable_name, args.in_data_file);

		object_finish(object);
	}

	return 0;
}
