#include "scene/dmg_music_using_notation.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "gba/bios.h"
#include "gba/palette.h"
#include "gba/screen.h"
#include "management/isr.h"
#include "management/keyinput.h"
#include "management/shadow_oam.h"
#include "management/shadow_vram.h"
#include "management/vram_op_queue.h"
#include "scene/main_menu.h"
#include "utils/arraycount.h"
#include "utils/one_transparent_tileset.h"
#include "dmg_music.h"
#include "graphics.h"
#include "graphics_types.h"
#include "main.h"
#include "mgba.h"
#include "text_printer.h"

enum {
	MEASURE_NUMBER_WINDOW_WIDTH = 3,
	CHANNEL_COUNT = 2,
};

enum note_length {
	NOTE_LENGTH_WHOLE = 32,
	NOTE_LENGTH_HALF = 16,
	NOTE_LENGTH_QUARTER = 8,
	NOTE_LENGTH_EIGHTH = 4,
	NOTE_LENGTH_SIXTEENTH = 2,
};

struct note {
	enum pitch pitch;
	enum note_length length;
};


static void MainCB_dmgMusicUsingNotation_main(void);
static void start_note_1(struct note);
static void start_note_2(struct note);
static void draw_measure(enum note_length measure_length, const struct note* const measure[CHANNEL_COUNT]);

static void (* const start_note[CHANNEL_COUNT])(struct note) = {
	start_note_1,
	start_note_2,
};

struct song {
	uint8_t tempo;
	enum note_length first_measure_length: 8;
	enum note_length measure_length: 8;
	uint8_t measure_count;
	// [[gnu::counted_by(measure_count)]]
	const struct note* measure[][CHANNEL_COUNT];
};

[[gnu::unused]]
static const struct song yankee_doodle = {
	.tempo = 120,
	.first_measure_length = NOTE_LENGTH_EIGHTH,
	.measure_length = NOTE_LENGTH_HALF,
	.measure_count = 18,
	.measure = {
		[0] = {
			[0] = (const struct note[]) {
				{PITCH_E4, NOTE_LENGTH_EIGHTH},
			},
			[1] = (const struct note[]) {
				{PITCH_E4, NOTE_LENGTH_EIGHTH},
			},
		},
		[1] = {
			[0] = (const struct note[]) {
				{PITCH_Cis4, NOTE_LENGTH_EIGHTH},
				{PITCH_Cis4, NOTE_LENGTH_EIGHTH},
				{PITCH_D4, NOTE_LENGTH_EIGHTH},
				{PITCH_E4, NOTE_LENGTH_EIGHTH},
			},
			[1] = (const struct note[]) {
				{PITCH_A4, NOTE_LENGTH_EIGHTH},
				{PITCH_A4, NOTE_LENGTH_EIGHTH},
				{PITCH_B4, NOTE_LENGTH_EIGHTH},
				{PITCH_Cis5, NOTE_LENGTH_EIGHTH},
			},
		},
		[2] = {
			[0] = (const struct note[]) {
				{PITCH_Cis4, NOTE_LENGTH_EIGHTH},
				{PITCH_E4, NOTE_LENGTH_EIGHTH},
				{PITCH_D4, NOTE_LENGTH_EIGHTH},
				{PITCH_D4, NOTE_LENGTH_EIGHTH},
			},
			[1] = (const struct note[]) {
				{PITCH_A4, NOTE_LENGTH_EIGHTH},
				{PITCH_Cis5, NOTE_LENGTH_EIGHTH},
				{PITCH_B4, NOTE_LENGTH_EIGHTH},
				{PITCH_G4, NOTE_LENGTH_EIGHTH},
			},
		},
		[3] = {
			[0] = (const struct note[]) {
				{PITCH_Cis4, NOTE_LENGTH_EIGHTH},
				{PITCH_Cis4, NOTE_LENGTH_EIGHTH},
				{PITCH_D4, NOTE_LENGTH_EIGHTH},
				{PITCH_E4, NOTE_LENGTH_EIGHTH},
			},
			[1] = (const struct note[]) {
				{PITCH_A4, NOTE_LENGTH_EIGHTH},
				{PITCH_A4, NOTE_LENGTH_EIGHTH},
				{PITCH_B4, NOTE_LENGTH_EIGHTH},
				{PITCH_Cis5, NOTE_LENGTH_EIGHTH},
			},
		},
		[4] = {
			[0] = (const struct note[]) {
				{PITCH_Cis4, NOTE_LENGTH_QUARTER},
				{PITCH_D4, NOTE_LENGTH_EIGHTH},
				{PITCH_D4, NOTE_LENGTH_EIGHTH},
			},
			[1] = (const struct note[]) {
				{PITCH_A4, NOTE_LENGTH_QUARTER},
				{PITCH_Gis4, NOTE_LENGTH_EIGHTH},
				{PITCH_E4, NOTE_LENGTH_EIGHTH},
			},
		},
		[5] = {
			[0] = (const struct note[]) {
				{PITCH_Cis4, NOTE_LENGTH_EIGHTH},
				{PITCH_Cis4, NOTE_LENGTH_EIGHTH},
				{PITCH_D4, NOTE_LENGTH_EIGHTH},
				{PITCH_E4, NOTE_LENGTH_EIGHTH},
			},
			[1] = (const struct note[]) {
				{PITCH_A4, NOTE_LENGTH_EIGHTH},
				{PITCH_A4, NOTE_LENGTH_EIGHTH},
				{PITCH_B4, NOTE_LENGTH_EIGHTH},
				{PITCH_Cis5, NOTE_LENGTH_EIGHTH},
			},
		},
		[6] = {
			[0] = (const struct note[]) {
				{PITCH_F4, NOTE_LENGTH_EIGHTH},
				{PITCH_F4, NOTE_LENGTH_EIGHTH},
				{PITCH_F4, NOTE_LENGTH_EIGHTH},
				{PITCH_F4, NOTE_LENGTH_EIGHTH},
			},
			[1] = (const struct note[]) {
				{PITCH_D5, NOTE_LENGTH_EIGHTH},
				{PITCH_Cis5, NOTE_LENGTH_EIGHTH},
				{PITCH_B4, NOTE_LENGTH_EIGHTH},
				{PITCH_A4, NOTE_LENGTH_EIGHTH},
			},
		},
		[7] = {
			[0] = (const struct note[]) {
				{PITCH_E4, NOTE_LENGTH_EIGHTH},
				{PITCH_E4, NOTE_LENGTH_EIGHTH},
				{PITCH_D4, NOTE_LENGTH_EIGHTH},
				{PITCH_D4, NOTE_LENGTH_EIGHTH},
			},
			[1] = (const struct note[]) {
				{PITCH_Gis4, NOTE_LENGTH_EIGHTH},
				{PITCH_E4, NOTE_LENGTH_EIGHTH},
				{PITCH_Fis4, NOTE_LENGTH_EIGHTH},
				{PITCH_Gis4, NOTE_LENGTH_EIGHTH},
			},
		},
		[8] = {
			[0] = (const struct note[]) {
				{PITCH_Cis4, NOTE_LENGTH_QUARTER},
				{PITCH_Cis4, NOTE_LENGTH_EIGHTH},
				{PITCH_REST, NOTE_LENGTH_EIGHTH},
			},
			[1] = (const struct note[]) {
				{PITCH_A4, NOTE_LENGTH_QUARTER},
				{PITCH_A4, NOTE_LENGTH_EIGHTH},
				{PITCH_REST, NOTE_LENGTH_EIGHTH},
			},
		},

		[9] = {
			[0] = (const struct note[]) {
				{PITCH_D4, NOTE_LENGTH_EIGHTH + NOTE_LENGTH_SIXTEENTH},
				{PITCH_E4, NOTE_LENGTH_SIXTEENTH},
				{PITCH_D4, NOTE_LENGTH_EIGHTH},
				{PITCH_Cis4, NOTE_LENGTH_EIGHTH},
			},
			[1] = (const struct note[]) {
				{PITCH_Fis4, NOTE_LENGTH_EIGHTH + NOTE_LENGTH_SIXTEENTH},
				{PITCH_Gis4, NOTE_LENGTH_SIXTEENTH},
				{PITCH_Fis4, NOTE_LENGTH_EIGHTH},
				{PITCH_E4, NOTE_LENGTH_EIGHTH},
			},
		},
		[10] = {
			[0] = (const struct note[]) {
				{PITCH_D4, NOTE_LENGTH_EIGHTH},
				{PITCH_E4, NOTE_LENGTH_EIGHTH},
				{PITCH_Fis4, NOTE_LENGTH_QUARTER},
			},
			[1] = (const struct note[]) {
				{PITCH_Fis4, NOTE_LENGTH_EIGHTH},
				{PITCH_Gis4, NOTE_LENGTH_EIGHTH},
				{PITCH_A4, NOTE_LENGTH_QUARTER},
			},
		},
		[11] = {
			[0] = (const struct note[]) {
				{PITCH_Cis4, NOTE_LENGTH_EIGHTH + NOTE_LENGTH_SIXTEENTH},
				{PITCH_D4, NOTE_LENGTH_SIXTEENTH},
				{PITCH_Cis4, NOTE_LENGTH_EIGHTH},
				{PITCH_B3, NOTE_LENGTH_EIGHTH},
			},
			[1] = (const struct note[]) {
				{PITCH_E4, NOTE_LENGTH_EIGHTH + NOTE_LENGTH_SIXTEENTH},
				{PITCH_Fis4, NOTE_LENGTH_SIXTEENTH},
				{PITCH_E4, NOTE_LENGTH_EIGHTH},
				{PITCH_D4, NOTE_LENGTH_EIGHTH},
			},
		},
		[12] = {
			[0] = (const struct note[]) {
				{PITCH_A3, NOTE_LENGTH_EIGHTH},
				{PITCH_B3, NOTE_LENGTH_EIGHTH},
				{PITCH_Cis4, NOTE_LENGTH_EIGHTH},
				{PITCH_REST, NOTE_LENGTH_EIGHTH},
			},
			[1] = (const struct note[]) {
				{PITCH_Cis4, NOTE_LENGTH_EIGHTH},
				{PITCH_D4, NOTE_LENGTH_EIGHTH},
				{PITCH_E4, NOTE_LENGTH_EIGHTH},
				{PITCH_REST, NOTE_LENGTH_EIGHTH},
			},
		},
		[13] = {
			[0] = (const struct note[]) {
				{PITCH_D4, NOTE_LENGTH_EIGHTH + NOTE_LENGTH_SIXTEENTH},
				{PITCH_E4, NOTE_LENGTH_SIXTEENTH},
				{PITCH_D4, NOTE_LENGTH_EIGHTH},
				{PITCH_Cis4, NOTE_LENGTH_EIGHTH},
			},
			[1] = (const struct note[]) {
				{PITCH_Fis4, NOTE_LENGTH_EIGHTH + NOTE_LENGTH_SIXTEENTH},
				{PITCH_Gis4, NOTE_LENGTH_SIXTEENTH},
				{PITCH_Fis4, NOTE_LENGTH_EIGHTH},
				{PITCH_E4, NOTE_LENGTH_EIGHTH},
			},
		},
		[14] = {
			[0] = (const struct note[]) {
				{PITCH_D4, NOTE_LENGTH_EIGHTH},
				{PITCH_E4, NOTE_LENGTH_EIGHTH},
				{PITCH_Fis4, NOTE_LENGTH_EIGHTH},
				{PITCH_D4, NOTE_LENGTH_EIGHTH},
			},
			[1] = (const struct note[]) {
				{PITCH_Fis4, NOTE_LENGTH_EIGHTH},
				{PITCH_Gis4, NOTE_LENGTH_EIGHTH},
				{PITCH_A4, NOTE_LENGTH_EIGHTH},
				{PITCH_Fis4, NOTE_LENGTH_EIGHTH},
			},
		},
		[15] = {
			[0] = (const struct note[]) {
				{PITCH_Cis4, NOTE_LENGTH_EIGHTH},
				{PITCH_Cis4, NOTE_LENGTH_EIGHTH},
				{PITCH_D4, NOTE_LENGTH_EIGHTH},
				{PITCH_D4, NOTE_LENGTH_EIGHTH},
			},
			[1] = (const struct note[]) {
				{PITCH_E4, NOTE_LENGTH_EIGHTH},
				{PITCH_A4, NOTE_LENGTH_EIGHTH},
				{PITCH_Gis4, NOTE_LENGTH_EIGHTH},
				{PITCH_B4, NOTE_LENGTH_EIGHTH},
			},
		},
		[16] = {
			[0] = (const struct note[]) {
				{PITCH_Cis4, NOTE_LENGTH_QUARTER},
				{PITCH_Cis4, NOTE_LENGTH_EIGHTH},
				{PITCH_REST, NOTE_LENGTH_EIGHTH},
			},
			[1] = (const struct note[]) {
				{PITCH_A4, NOTE_LENGTH_QUARTER},
				{PITCH_A4, NOTE_LENGTH_EIGHTH},
				{PITCH_REST, NOTE_LENGTH_EIGHTH},
			},
		},
		[17] = {
			[0] = (const struct note[]) {
				{PITCH_REST, NOTE_LENGTH_HALF},
			},
			[1] = (const struct note[]) {
				{PITCH_REST, NOTE_LENGTH_HALF},
			},
		},
	}
};

static const struct song* const song = &yankee_doodle;


static const struct shadow_vram_init brick_break_reg = {
	.enable_bg = {true, false, false, true},
	.enable_obj = true,
	.bgcnt = {
		[0] = {
			.priority = 3,
			.charblock = 0,
			.screenblock = 31,
		},
		[3] = {
			.priority = 2,
			.charblock = 0,
			.screenblock = 30,
		},
	}
};
static const palette16_t text_pal = {
	{10, 10, 10},
	{30, 10, 10},
	{10, 30, 10},
	{30, 30, 0},
	{10, 10, 30},
	{30, 10, 30},
	{10, 30, 30},
	{31, 31, 31},
	{0, 0, 0},
	{20, 0, 0},
	{0, 20, 0},
	{15, 15, 0},
	{0, 0, 20},
	{20, 0, 20},
	{0, 20, 20},
	{20, 20, 20},
};

static const struct shadow_tiles_window_allocate measure_number_window_template = {
	.bg = 3,
	.palette = 15,
	.x = 2,
	.y = 2,
	.width = MEASURE_NUMBER_WINDOW_WIDTH,
	.height = 2,
};


// model
static uint16_t current_measure = 0;

static struct channel_data {
	uint8_t current_note_index;
	uint8_t current_note_frames_left;
	uint8_t measure_note_length_so_far;
} channel_data[CHANNEL_COUNT] = {0};

// viewmodel
static struct {
	window_id_t measure_number_window;
	[[gnu::aligned(4)]]
	tile_4bpp_t measure_number_shadow_tiles[2 * MEASURE_NUMBER_WINDOW_WIDTH];
}* view_model = NULL;

void MainCB_dmgMusicUsingNotation_init(void) {
	reg_sound._1 = (struct reg_sound_1) {0};
	reg_sound._2 = (struct reg_sound_2) {0};

	reg_sound.master.X = (reg_sound_control_master_t) {
		.status_2 = true,
		.enable = true,
	};
	reg_sound.master.L = (reg_sound_control_dmg_t) {
		.left_volume = 7,
		.right_volume = 7,
		.left_1 = true,
		.left_2 = true,
		.right_1 = true,
		.right_2 = true,
	};
	reg_sound.master.H = (reg_sound_control_direct_t) {0};

	reg_sound._2.L = (reg_sound_tone_envelope_t) {
		.length = 0,
		.pattern_duty = WAVE_DUTY_50,
		.step_time = 0,
		.direction = ENVELOPE_INCREASE,
		.volume = 15,
	};
	reg_sound._1.L = (reg_sound_tone_sweep_t) {0};
	reg_sound._1.H = (reg_sound_tone_envelope_t) {
		.length = 0,
		.pattern_duty = WAVE_DUTY_50,
		.step_time = 0,
		.direction = ENVELOPE_INCREASE,
		.volume = 15,
	};

	current_measure = -1;
	channel_data[0] = (struct channel_data) {
		.current_note_index = 0,
		.current_note_frames_left = 0,
		.measure_note_length_so_far = 0xFF,
	};
	channel_data[1] = (struct channel_data) {
		.current_note_index = 0,
		.current_note_frames_left = 0,
		.measure_note_length_so_far = 0xFF,
	};


	view_model = calloc(sizeof(view_model[0]), 1);

	shadow_vram_init(&brick_break_reg);

	vram_op_queue_enqueue((struct vram_op) {
			.type = VRAM_QUEUE_OP_DISABLE_ALL_OAM,
	});
	shadow_tiles_load_background(
		&music_sheet_background,
		(struct shadow_tiles_load_background){.bg = 0});

	vram_op_queue_enqueue((struct vram_op) {
			.type = VRAM_QUEUE_OP_OAM_TILES_COMPRESSED,
			.tiles_compressed = {
				.from = music_sheet_notes.tileset,
				.to_block = 0,
				.to_tile = 0,
			}
	});
	vram_op_queue_enqueue((struct vram_op) {
			.type = VRAM_QUEUE_OP_OAM_PALETTES,
			.palettes = {
				.from = music_sheet_notes.palette,
				.to_palette = 0,
				.count = 1,
			}
	});

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_PALETTES,
		.palettes = {
			.from = &text_pal,
			.to_palette = 15,
			.count = 1,
		},
	});

	bg_tile_t zero_tile_ref = (bg_tile_t) {.tile = shadow_tiles_load_tileset(&one_transparent_tileset, (struct shadow_tiles_load_tileset) {3})};

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_MAP_FILL,
		.map_fill = {
			.value = zero_tile_ref,
			.to_block = 30,
			.to_tile = 0,
			.count = 32 * 20,
		},
	});

	view_model->measure_number_window = shadow_tiles_window_allocate(&measure_number_window_template);
	shadow_tiles_window_queue_map(view_model->measure_number_window);

	scene_onframe_callback = &MainCB_dmgMusicUsingNotation_main;
}

static void MainCB_dmgMusicUsingNotation_main(void) {
	if (! keyinput_get_down().b) {
		if (view_model) {
			free(view_model);
			view_model = NULL;
		}

		reg_sound.master.X = (reg_sound_control_master_t) {0};
		scene_onframe_callback = &MainCB_mainMenu_init;
		return;
	}

	enum note_length current_measure_length = (current_measure == 0 ? song->first_measure_length : song->measure_length);

	if (channel_data[0].current_note_frames_left == 0
			&& channel_data[1].current_note_frames_left == 0
			&& channel_data[0].measure_note_length_so_far >= current_measure_length
			&& channel_data[1].measure_note_length_so_far >= current_measure_length
	) {
		// next measure
		current_measure++;
		if (current_measure >= song->measure_count) {
			current_measure = 0;
		}

		current_measure_length = (current_measure == 0 ? song->first_measure_length : song->measure_length);

		for (unsigned i = 0; i < CHANNEL_COUNT; i++) {
			(*start_note[i])(song->measure[current_measure][i][0]);

			channel_data[i].current_note_index = 0;
			channel_data[i].measure_note_length_so_far = song->measure[current_measure][i][0].length;
			channel_data[i].current_note_frames_left = song->measure[current_measure][i][0].length * 3600 / song->tempo / NOTE_LENGTH_QUARTER;
		}

		draw_measure(current_measure_length, song->measure[current_measure]);

	} else {
		for (unsigned i = 0; i < CHANNEL_COUNT; i++) {
			if (channel_data[i].current_note_frames_left == 0) {
				if (channel_data[i].measure_note_length_so_far < current_measure_length) {
					channel_data[i].current_note_index++;
					const uint8_t current_note_index = channel_data[i].current_note_index;

					(*start_note[i])(song->measure[current_measure][i][current_note_index]);
					channel_data[i].measure_note_length_so_far += song->measure[current_measure][i][current_note_index].length;
					channel_data[i].current_note_frames_left = song->measure[current_measure][i][current_note_index].length * 3600 / song->tempo / NOTE_LENGTH_QUARTER;
				}
			} else {
				channel_data[i].current_note_frames_left--;
			}
		}
	}
}

static void start_note_1(struct note note) {
	if (note.pitch != PITCH_REST) {
		reg_sound._1.H = (reg_sound_tone_envelope_t) {
			.length = 0,
			.pattern_duty = WAVE_DUTY_50,
			.step_time = 4,
			.direction = ENVELOPE_DECREASE,
			.volume = 15,
		};
		reg_sound._1.X = (reg_sound_frequency_control_t) {
			.frequency = pitch_frequencies[note.pitch],
			.restart = true,
		};
	} else {
		reg_sound._1.X = (reg_sound_frequency_control_t) {
			.stop = true,
		};
	}
}

static void start_note_2(struct note note) {
	if (note.pitch != PITCH_REST) {
		reg_sound._2.H = (reg_sound_frequency_control_t) {
			.frequency = pitch_frequencies[note.pitch],
			.restart = true,
		};
	} else {
		reg_sound._2.H = (reg_sound_frequency_control_t) {
			.stop = true,
		};
	}
}

enum note_symbol_fragments {
	HOLLOW_HEAD,
	FILLED_HEAD,
	STEM,
	FLAG,
	FLAG2,
	DOT,
	DOT2,
	WHOLE_REST,
	HALF_REST,
	QUARTER_REST,
	EIGHTH_REST,
	TERMINATOR = 0xFF
};

static const struct {
	int8_t dx;
	int8_t dy;
	uint8_t tile_num;
	enum oam_shape shape : 4;
	uint8_t size : 4;
} note_symbol_fragments[] = {
	[HOLLOW_HEAD] = {
		.dx = -4,
		.dy = -4,
		.tile_num = 1,
		.shape = OAM_SHAPE_SQUARE,
		.size = 0,
	},
	[FILLED_HEAD] = {
		.dx = -4,
		.dy = -4,
		.tile_num = 2,
		.shape = OAM_SHAPE_SQUARE,
		.size = 0,
	},
	[DOT] = {
		.dx = 2,
		.dy = -2,
		.tile_num = 7,
		.shape = OAM_SHAPE_SQUARE,
		.size = 0,
	},
	[STEM] = {
		.dx = -1,
		.dy = -28,
		.tile_num = 3,
		.shape = OAM_SHAPE_VERTICAL,
		.size = 1,
	},
	[FLAG] = {
		.dx = 2,
		.dy = -24,
		.tile_num = 8,
		.shape = OAM_SHAPE_SQUARE,
		.size = 0,
	},
	[FLAG2] = {
		.dx = 2,
		.dy = -16,
		.tile_num = 8,
		.shape = OAM_SHAPE_SQUARE,
		.size = 0,
	},
	[WHOLE_REST] = {
		.dx = -8,
		.dy = -2,
		.tile_num = 9,
		.shape = OAM_SHAPE_HORIZONTAL,
		.size = 0,
	},
	[HALF_REST] = {
		.dx = -8,
		.dy = -6,
		.tile_num = 9,
		.shape = OAM_SHAPE_HORIZONTAL,
		.size = 0,
	},
	[QUARTER_REST] = {
		.dx = -4,
		.dy = -8,
		.tile_num = 11,
		.shape = OAM_SHAPE_VERTICAL,
		.size = 0,
	},
	[EIGHTH_REST] = {
		.dx = -4,
		.dy = -4,
		.tile_num = 13,
		.shape = OAM_SHAPE_SQUARE,
		.size = 0,
	},
};

typedef enum note_symbol_fragments symfrag;
static const enum note_symbol_fragments* note_symbols[] = {
	[NOTE_LENGTH_WHOLE] = (symfrag[]) {HOLLOW_HEAD, TERMINATOR},
	[NOTE_LENGTH_HALF] = (symfrag[]) {HOLLOW_HEAD, STEM, TERMINATOR},
	[NOTE_LENGTH_QUARTER] = (symfrag[]) {FILLED_HEAD, STEM, TERMINATOR},
	[NOTE_LENGTH_EIGHTH + NOTE_LENGTH_SIXTEENTH] = (symfrag[]) {FILLED_HEAD, DOT, STEM, FLAG, TERMINATOR},
	[NOTE_LENGTH_EIGHTH] = (symfrag[]) {FILLED_HEAD, STEM, FLAG, TERMINATOR},
	[NOTE_LENGTH_SIXTEENTH] = (symfrag[]) {FILLED_HEAD, STEM, FLAG, FLAG2, TERMINATOR},
};
static const enum note_symbol_fragments* rest_symbols[] = {
	[NOTE_LENGTH_WHOLE] = (symfrag[]) {WHOLE_REST, TERMINATOR},
	[NOTE_LENGTH_HALF] = (symfrag[]) {HALF_REST, TERMINATOR},
	[NOTE_LENGTH_QUARTER] = (symfrag[]) {QUARTER_REST, TERMINATOR},
	[NOTE_LENGTH_EIGHTH] = (symfrag[]) {EIGHTH_REST, TERMINATOR},
};

static unsigned draw_note(unsigned sprite_start, struct note note, unsigned x) {
	unsigned sprite_i = sprite_start;

	const uint8_t* my_note_symbols = (note.pitch == PITCH_REST ? rest_symbols : note_symbols )[note.length];
	unsigned y = staff_positions[note.pitch];

	if (NULL == my_note_symbols) {
		vram_op_queue_enqueue((struct vram_op) {
			.type = VRAM_QUEUE_OP_OAM_ENTRY,
			.oam = {
				.to_index = sprite_i++,
				.value = {
					.y = y,
					.x = x,
					.disabled = false,
					.mode = OAM_MODE_NORMAL,
					.mosaic = false,
					.palette_mode = PAL_MODE_4BPP,
					.shape = OAM_SHAPE_SQUARE,
					.size = 0,
					.tile_num = 0,
					.priority = 0,
					.palette_num = 0,
				}
			}
		});
	} else {
		unsigned part_i = 0;
		while (my_note_symbols[part_i] != TERMINATOR) {
			vram_op_queue_enqueue((struct vram_op) {
				.type = VRAM_QUEUE_OP_OAM_ENTRY,
				.oam = {
					.to_index = sprite_i++,
					.value = {
						.y = y + note_symbol_fragments[my_note_symbols[part_i]].dy,
						.x = x + note_symbol_fragments[my_note_symbols[part_i]].dx,
						.disabled = false,
						.mode = OAM_MODE_NORMAL,
						.mosaic = false,
						.palette_mode = PAL_MODE_4BPP,
						.shape = note_symbol_fragments[my_note_symbols[part_i]].shape,
						.size = note_symbol_fragments[my_note_symbols[part_i]].size,
						.tile_num = note_symbol_fragments[my_note_symbols[part_i]].tile_num,
						.priority = 0,
						.palette_num = 0,
					}
				}
			});
			part_i++;
		}
	}

	return sprite_i;
}

static void draw_measure(enum note_length measure_length, const struct note* const measure[CHANNEL_COUNT]) {
	vram_op_queue_enqueue((struct vram_op) {
			.type = VRAM_QUEUE_OP_DISABLE_ALL_OAM,
	});

	char measure_number_str[8];
	snprintf(measure_number_str, 8, "%d", current_measure);
	uint32_t zero = 0;
	CpuFastSet(
		&zero,
		view_model->measure_number_shadow_tiles,
		(struct CpuFastSet){
			.word_count = sizeof(view_model->measure_number_shadow_tiles) / sizeof(uint32_t),
			.mode = CPU_SET_FILL,
		});

	uint16_t text_x = MEASURE_NUMBER_WINDOW_WIDTH * 8 - 2 - text_width(
		&breakout_set_font,
		(coord16_t) {.x = 0, .y = 0},
		measure_number_str);

	text_print_immediate(
		view_model->measure_number_shadow_tiles,
		&measure_number_window_template,
		&breakout_set_font,
		(coord16_t) {.x = text_x, .y = 6},
		(coord16_t) {.x = 0, .y = 0},
		(font_colors_t) {0, 7, 15, 8, false},
		measure_number_str);

	shadow_tiles_window_queue_tiles(view_model->measure_number_window, view_model->measure_number_shadow_tiles);

	unsigned sprite_i = 0;

	for (unsigned channel_i = 0; channel_i < CHANNEL_COUNT; channel_i++) {
		unsigned x = 48;
		unsigned note_i = 0;
		unsigned measure_length_i = 0;
		while (measure_length_i < measure_length) {
			struct note note = measure[channel_i][note_i];
			sprite_i = draw_note(sprite_i, note, x);
			x += 5 * note.length;
			measure_length_i += note.length;
			note_i++;
		}
	}
}
