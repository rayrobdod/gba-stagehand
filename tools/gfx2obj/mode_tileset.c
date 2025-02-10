#include "mode_tileset.h"

#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "object.h"
#include "parse_arguments.h"
#include "util.h"


struct TilesetArguments {
	const char* in_palettes_file;
	const char* in_tileset_file;
	const char* out_object_file;
	const char* out_header_file;
	const char* variable_name;
};

static void set_in_palettes_file(void* accumulator, const char* arg) {
	((struct TilesetArguments*) accumulator)->in_palettes_file = arg;
}
static void set_in_tileset_file(void* accumulator, const char* arg) {
	((struct TilesetArguments*) accumulator)->in_tileset_file = arg;
}
static void set_out_object_file(void* accumulator, const char* arg) {
	((struct TilesetArguments*) accumulator)->out_object_file = arg;
}
static void set_out_header_file(void* accumulator, const char* arg) {
	((struct TilesetArguments*) accumulator)->out_header_file = arg;
}
static void set_variable_name(void* accumulator, const char* arg) {
	((struct TilesetArguments*) accumulator)->variable_name = arg;
}

static const struct ArgumentsAndFlags argsTemplate = {
	.flags_count = 0,
	.flags = NULL,
	.args_count = 5,
	.arguments = (struct Argument[]) {
		{"--in_palettes", set_in_palettes_file},
		{"--in_tileset", set_in_tileset_file},
		{"--out_object", set_out_object_file},
		{"--out_header", set_out_header_file},
		{"--variable_name", set_variable_name},
	}
};

int mode_tileset(int argc, char* argv[]) {
	struct TilesetArguments args = {0};
	parseArguments(argc, argv, &args, &argsTemplate);

	if (! args.variable_name) {
		fprintf(stderr, "`--variable_name` required\n");
		exit(1);
	}
	if (! args.in_palettes_file) {
		fprintf(stderr, "`--in_palettes` required\n");
		exit(1);
	}
	if (! args.in_tileset_file) {
		fprintf(stderr, "`--in_tileset` required\n");
		exit(1);
	}

	if (args.out_header_file) {
		FILE* f = fopen(args.out_header_file, "w");
		if (!f) {
			fprintf(stderr, "Could not open file %s\n", args.out_header_file);
			exit(1);
		}
		fprintf(f, "extern struct tileset_graphics %s;\n", args.variable_name);
		fclose(f);
	}
	if (args.out_object_file) {
		struct Object* object = object_start(args.out_object_file);

		char* pal_name = strdupcat(args.variable_name, ".pal");
		object_push_file_copy_section(
			object,
			args.in_palettes_file,
			(struct variable_template) { pal_name, STB_LOCAL });

		char* tiles_name = strdupcat(args.variable_name, ".tiles");
		object_push_file_copy_section(
			object,
			args.in_tileset_file,
			(struct variable_template) { tiles_name, STB_LOCAL });

		uint32_t root_data[4] = {
			file_size(args.in_palettes_file) / (16 * 2),
			0,
			file_size(args.in_tileset_file) / (8 * 4),
			0,
		};
		object_push_bytes_section(
			object,
			root_data, sizeof(root_data),
			(struct variable_template) { args.variable_name, STB_GLOBAL });

		struct relocation_template relocations[2] = {
			[0] = {
				.offset = 4,
				.type = R_ARM_ABS32,
				.symbol_name = pal_name,
			},
			[1] = {
				.offset = 12,
				.type = R_ARM_ABS32,
				.symbol_name = tiles_name,
			},
		};

		char* root_section_name = strdupcat(".rodata.", args.variable_name);
		push_relocation_section(
			object,
			root_section_name,
			relocations, 2);

		free(pal_name);
		free(tiles_name);
		free(root_section_name);
		object_finish(object);
	}

	return 0;
}
