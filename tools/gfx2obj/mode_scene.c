#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gba/hw_reg.h"
#include "gba/hw_reg_cast.h"
#include "mode_sprite.h"
#include "object.h"
#include "parse_arguments.h"
#include "util.h"

struct SceneArguments {
	const char* in_palettes_file;
	const char* in_tiles_file;
	const char* in_map0_file;
	const char* out_object_file;
	const char* out_header_file;
	const char* variable_name;
};

static void set_in_palettes_file(void* accumulator, const char* arg) {
	((struct SceneArguments*)accumulator)->in_palettes_file = arg;
}
static void set_in_tiles_file(void* accumulator, const char* arg) {
	((struct SceneArguments*)accumulator)->in_tiles_file = arg;
}
static void set_in_map0_file(void* accumulator, const char* arg) {
	((struct SceneArguments*)accumulator)->in_map0_file = arg;
}
static void set_out_object_file(void* accumulator, const char* arg) {
	((struct SceneArguments*)accumulator)->out_object_file = arg;
}
static void set_out_header_file(void* accumulator, const char* arg) {
	((struct SceneArguments*)accumulator)->out_header_file = arg;
}
static void set_variable_name(void* accumulator, const char* arg) {
	((struct SceneArguments*)accumulator)->variable_name = arg;
}

static const struct ArgumentsAndFlags argsTemplate = {
	.flags = NULL,
	.arguments = (struct Argument[]){
		{"--in_palettes", set_in_palettes_file},
		{"--in_tiles", set_in_tiles_file},
		{"--in_map0", set_in_map0_file},
		{"--out_object", set_out_object_file},
		{"--out_header", set_out_header_file},
		{"--variable_name", set_variable_name},
		{0},
	}};

int mode_scene(int argc, char* argv[]) {
	struct SceneArguments args = {0};
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
	if (! args.in_map0_file) {
		fprintf(stderr, "`--in_map0` required\n");
		exit(1);
	}

	if (args.out_header_file) {
		FILE* f = fopen(args.out_header_file, "w");
		if (! f) {
			fprintf(stderr, "Could not open file %s\n", args.out_header_file);
			exit(1);
		}
		fprintf(f, "extern const struct scene_graphics %s;\n", args.variable_name);
		fclose(f);
	}
	if (args.out_object_file) {
		struct Object* object = object_start(args.out_object_file);

		char* pal_name = strdupcat(args.variable_name, ".pal");
		object_push_file_copy_section(
			object,
			args.in_palettes_file,
			(struct variable_template){pal_name, STB_LOCAL});

		char* tiles_name = strdupcat(args.variable_name, ".tiles");
		object_push_file_copy_section(
			object,
			args.in_tiles_file,
			(struct variable_template){tiles_name, STB_LOCAL});

		char* map0_name = strdupcat(args.variable_name, ".map0");
		object_push_file_copy_section(
			object,
			args.in_map0_file,
			(struct variable_template){map0_name, STB_LOCAL});

		uint16_t bgcnt0 = bgcnt2uint((bgcnt_t){
			.priority = 0,
			.charblock = 0,
			.palette_mode = PAL_MODE_4BPP,
			.screenblock = 31,
		});

		uint32_t root_data[6] = {
			file_size(args.in_palettes_file) / (16 * 2),
			0,
			file_size(args.in_tiles_file) / (8 * 4),
			0,
			(bgcnt0 << 16) | (file_size(args.in_map0_file) / 2),
			0,
		};
		object_push_bytes_section(
			object,
			root_data,
			sizeof(root_data),
			(struct variable_template){args.variable_name, STB_GLOBAL});

		struct relocation_template relocations[3] = {
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
			[2] = {
				.offset = 20,
				.type = R_ARM_ABS32,
				.symbol_name = map0_name,
			},
		};

		char* root_section_name = strdupcat(".rodata.", args.variable_name);
		push_relocation_section(
			object,
			root_section_name,
			relocations,
			arraycount(relocations));

		free(pal_name);
		free(tiles_name);
		free(root_section_name);
		object_finish(object);
	}

	return 0;
}
