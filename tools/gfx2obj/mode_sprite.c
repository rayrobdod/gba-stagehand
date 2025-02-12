#include "mode_sprite.h"

#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "object.h"
#include "parse_arguments.h"
#include "util.h"


struct TilesetArguments {
	const char* in_palettes_file;
	const char* in_tiles_file;
	const char* out_object_file;
	const char* out_header_file;
	const char* variable_name;
	unsigned paltag;
	unsigned tiletag;
	unsigned size;
};

static void set_in_palettes_file(void* accumulator, const char* arg) {
	((struct TilesetArguments*) accumulator)->in_palettes_file = arg;
}
static void set_in_tiles_file(void* accumulator, const char* arg) {
	((struct TilesetArguments*) accumulator)->in_tiles_file = arg;
}
static void set_paltag(void* accumulator, const char* arg) {
	long argl = strtol(arg, NULL, 0);
	((struct TilesetArguments*) accumulator)->paltag = argl;
}
static void set_size(void* accumulator, const char* arg) {
	if (0 == strcmp(arg, "8x8")) {
		((struct TilesetArguments*) accumulator)->size = 0;
	} else if (0 == strcmp(arg, "16x16")) {
		((struct TilesetArguments*) accumulator)->size = (1 << 2);
	} else {
		fprintf(stderr, "Illegal `--size` parameter %s\n", arg);
		exit(1);
	}
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
	.flags = NULL,
	.arguments = (struct Argument[]) {
		{"--in_palettes", set_in_palettes_file},
		{"--paltag", set_paltag},
		{"--in_tiles", set_in_tiles_file},
		{"--size", set_size},
		{"--out_object", set_out_object_file},
		{"--out_header", set_out_header_file},
		{"--variable_name", set_variable_name},
		{0},
	}
};

int mode_sprite(int argc, char* argv[]) {
	struct TilesetArguments args = {.size = 0xFF};
	parseArguments(argc, argv, &args, &argsTemplate);

	if (! args.variable_name) {
		fprintf(stderr, "`--variable_name` required\n");
		exit(1);
	}
	if (! args.in_palettes_file) {
		fprintf(stderr, "`--in_palettes` required\n");
		exit(1);
	}
	if (! args.in_tiles_file) {
		fprintf(stderr, "`--in_tiles` required\n");
		exit(1);
	}
	if (args.size == 0xFF) {
		fprintf(stderr, "`--size` required\n");
		exit(1);
	}

	if (args.out_header_file) {
		FILE* f = fopen(args.out_header_file, "w");
		if (!f) {
			fprintf(stderr, "Could not open file %s\n", args.out_header_file);
			exit(1);
		}
		fprintf(f, "extern const struct shadow_oam_template %s;\n", args.variable_name);
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
			args.in_tiles_file,
			(struct variable_template) { tiles_name, STB_LOCAL });

		uint32_t root_data[4] = {
			0,
			0,
			args.paltag | (args.tiletag << 16),
			args.size,
		};
		object_push_bytes_section(
			object,
			root_data, sizeof(root_data),
			(struct variable_template) { args.variable_name, STB_GLOBAL });

		struct relocation_template relocations[2] = {
			[0] = {
				.offset = 0,
				.type = R_ARM_ABS32,
				.symbol_name = pal_name,
			},
			[1] = {
				.offset = 4,
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
