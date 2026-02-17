#include "scene/display_credits.h"

#include <stdint.h>
#include <stdlib.h>
#include "gba/bios.h"
#include "gba/palette.h"
#include "gba/screen.h"
#include "management/keyinput.h"
#include "management/vram_op_queue.h"
#include "scene/main_menu.h"
#include "transition/cut.h"
#include "utils/ansi_text_palette.h"
#include "graphics.h"
#include "graphics_types.h"
#include "resource_credits.h"
#include "main.h"
#include "mgba.h"
#include "text_printer.h"

static union palette512 InitFadeIn_credits(void);
static void MainCB_credits_printTitleInit(void);
static void MainCB_credits_printTitle(void);
static void MainCB_credits_hold(void);
static void MainCB_credits_resourceHeader(void);
static void MainCB_credits_resource(void);
static void MainCB_credits_scrollUntilBlank(void);
static void MainCB_credits_stall(void);

static const struct transitionSourceCallbacks transitionSourceCbs_credits = {
	.fadeOut = NULL,
	.cleanup = NULL,
};
const struct transitionTargetCallbacks transitionTargetCbs_credits = {
	.initFadeOut = NULL,
	.fadeOut = NULL,
	.initFadeIn = InitFadeIn_credits,
	.fadeIn = NULL,
	.target = MainCB_credits_printTitleInit,
};

enum {
	TEXT_LAYER = 1,
	TEXT_PALETTE = 0,
	TEXT_CHARBLOCK = 0,
	TEXT_SCREENBLOCK = 31,

	SCROLL_FREQUENCY = 2,
};

__attribute__((section(".sbss")))
static struct {
	struct text_print_step_state text_print_step_state;
	bool is_printing;
	uint8_t delay;
	uint8_t bgoffs_y;
	uint8_t print_y;
	struct {
		const struct resource_credit* current;
		enum {
			RESOURCEPART_TITLE,
			RESOURCEPART_RETRIEVEDFROM,
			RESOURCEPART_By,
			RESOURCEPART_AUTHOR,
			RESOURCEPART_AUTHORURL,
			RESOURCEPART_LICENSE,
			RESOURCEPART_DerivedFrom,
			RESOURCEPART_DERIVED_TITLE,
			RESOURCEPART_DERIVED_RETRIEVEDFROM,
			RESOURCEPART_DERIVED_By,
			RESOURCEPART_DERIVED_AUTHOR,
			RESOURCEPART_DERIVED_AUTHORURL,
			RESOURCEPART_DERIVED_LICENSE,
			RESOURCEPART_ADVANCE
		} part;
	} resource;
} view_model = {0};

static const struct shadow_tiles_window_allocate whole_screen_window = {
	.bg = TEXT_LAYER,
	.palette = TEXT_PALETTE,
	.x = 0,
	.y = 0,
	.width = 30,
	.height = 32,
};

static const font_colors_t colors_header = {
	ANSI_PALETTE_BLACK,
	ANSI_PALETTE_WHITE,
	ANSI_PALETTE_GREY,
	ANSI_PALETTE_BLUE,
	true
};
static const font_colors_t colors_normal = {
	ANSI_PALETTE_BLACK,
	ANSI_PALETTE_WHITE,
	ANSI_PALETTE_GREY,
	ANSI_PALETTE_BLACK,
	true
};
static const font_colors_t colors_url = {
	ANSI_PALETTE_BLACK,
	ANSI_PALETTE_BRIGHT_BLUE,
	ANSI_PALETTE_BLUE,
	ANSI_PALETTE_BLACK,
	true
};
static const coord16_t kerning_normal = {-1, -1};
static const coord16_t kerning_title = {1, -1};

static const text_print_overflow_t overflow_wordwrapx_aroundy = {
		TEXTPRINTOVERFLOWX_WORDWRAP, TEXTPRINTOVERFLOWY_WRAPAROUND};

static union palette512 InitFadeIn_credits(void) {
	view_model = (typeof(view_model)) {0};
	view_model.resource.current = resource_credits;

	union palette512 retval = {0};
	CpuFastCopy(ansi_text_palette, retval.background._4[TEXT_PALETTE], sizeof(palette16_t) / sizeof(uint32_t));

	vram_op_queue_enqueue(&(struct vram_op) {
		.type = VRAM_QUEUE_OP_DISABLE_ALL_OAM,
	});
	vram_op_queue_enqueue(&(struct vram_op) {
		.type = VRAM_QUEUE_OP_HWREG_DISPCNT,
		.dispcnt = {
			.value = {
				.mode = 0,
				.obj_character_mapping = OBJ_CHAR_MAP_1D,
				.enable_bg1 = true,
			}
		},
	});
	vram_op_queue_enqueue(&(struct vram_op) {
		.type = VRAM_QUEUE_OP_HWREG_BGOFSS,
		.bgofss = {
			.value = {{0},{0},{0},{0},},
		},
	});
	vram_op_queue_enqueue(&(struct vram_op) {
		.type = VRAM_QUEUE_OP_HWREG_BGCNT,
		.bgcnt = {
			.value = (bgcnt_t) {
				.priority = 0,
				.charblock = TEXT_CHARBLOCK,
				.screenblock = TEXT_SCREENBLOCK,
			},
			.to_index = TEXT_LAYER
		}
	});

	bg_tile_t* tilemap_buffer = malloc(32 * 32 * sizeof(bg_tile_t));
	if (tilemap_buffer) {
		for (unsigned y = 0; y < 32; y++)
		for (unsigned x = 0; x < 30; x++) {
			tilemap_buffer[y * 32 + x] = (bg_tile_t) {.tile = y * 30 + x, .hflip = false, .vflip = false, .palette = TEXT_PALETTE};
		}

		vram_op_queue_enqueue(&(struct vram_op){
			.type = VRAM_QUEUE_OP_BG_MAP_FREE,
			.map_free = {
				.from = tilemap_buffer,
				.to_block = TEXT_SCREENBLOCK,
				.to_tile = 0,
				.count = 32 * 32,
			}});
	}

	vram_op_queue_enqueue(&(struct vram_op){
		.type = VRAM_QUEUE_OP_BG_TILES_FILL,
		.tiles_fill = {
			.value = ANSI_PALETTE_BLACK,
			.to_block = TEXT_CHARBLOCK,
			.to_tile = 0,
			.count = 32 * 32,
		}});

	return retval;
}

static void exitCredits(void) {
	StartTransition(
		&transition_cut,
		&transitionSourceCbs_credits,
		&transitionTargetCbs_mainMenu);
}

static void MainCB_credits_printTitleInit(void) {
	if (! keyinput_get_new().b) {
		exitCredits();
	} else {
		const char* title = "TOTALLY COOL GAME";

		unsigned title_width = text_width(
			&bitmapfont,
			kerning_title,
			title);

		coord16_t title_position = {
			.x = (DISPLAY_WIDTH - title_width) / 2,
			.y = (DISPLAY_HEIGHT - bitmapfont.glyph_height) / 2,
		};

		text_print_step_init(
			&view_model.text_print_step_state,
			vram.bg_charblock[TEXT_CHARBLOCK],
			&whole_screen_window,
			&bitmapfont,
			title_position,
			kerning_title,
			TEXTPRINTOVERFLOW_CLIP,
			colors_header,
			title);

		view_model.delay = 8;
		scene_onframe_callback = &MainCB_credits_printTitle;
	}
}

static void MainCB_credits_printTitle(void) {
	if (! keyinput_get_new().b) {
		exitCredits();
	} else if (view_model.delay) {
		view_model.delay--;
	} else {
		view_model.delay = 8;

		enum text_print_step_retval progress = text_print_step(
			&view_model.text_print_step_state);

		if (TEXT_PRINT_STEP_STOP == progress) {
			view_model.print_y = DISPLAY_HEIGHT + 16;
			view_model.delay = 20;
			scene_onframe_callback = &MainCB_credits_hold;
		}
	}
}

static void MainCB_credits_hold(void) {
	if (! keyinput_get_new().b) {
		exitCredits();
	} else if (view_model.delay) {
		view_model.delay--;
	} else {
		scene_onframe_callback = &MainCB_credits_resourceHeader;
	}
}

static void MainCB_credits_resourceHeader(void) {
	if (! keyinput_get_new().b) {
		exitCredits();
	} else {
		const char* title = "RESOURCES";

		unsigned title_width = text_width(
			&bitmapfont,
			kerning_title,
			title);

		coord16_t title_position = {
			.x = (DISPLAY_WIDTH - title_width) / 2,
			.y = view_model.print_y,
		};
		view_model.print_y += bitmapfont.glyph_height;

		text_print_immediate(
			vram.bg_charblock[TEXT_CHARBLOCK],
			&whole_screen_window,
			&bitmapfont,
			title_position,
			kerning_title,
			colors_header,
			title);

		text_clear_immediate(
			vram.bg_charblock[TEXT_CHARBLOCK],
			&whole_screen_window,
			(coord16_t) {title_position.x, view_model.print_y - 1},
			(coord16_t) {title_position.x + title_width - 1, view_model.print_y},
			ANSI_PALETTE_GREY);

		view_model.print_y += 8;

		scene_onframe_callback = &MainCB_credits_resource;
	}
}

static void MainCB_credits_resource(void) {
	if (! keyinput_get_new().b) {
		exitCredits();
	} else {
		if (view_model.delay) {
			view_model.delay--;
		} else {
			view_model.delay = SCROLL_FREQUENCY;
			view_model.bgoffs_y += 1;

			unsigned row_to_clear = (256 + view_model.bgoffs_y - 2) % 256;
			text_clear_immediate(
				vram.bg_charblock[TEXT_CHARBLOCK],
				&whole_screen_window,
				(coord16_t) {0, row_to_clear},
				(coord16_t) {DISPLAY_WIDTH, row_to_clear + 1},
				ANSI_PALETTE_BLACK);

			vram_op_queue_enqueue(&(struct vram_op) {
				.type = VRAM_QUEUE_OP_UINT16,
				.uint16 = {
					.value = view_model.bgoffs_y,
					.to = &reg_lcd.BGOFS[TEXT_LAYER].v,
				},
			});
		}

		if (view_model.is_printing) {
			int steps = 0;
			enum text_print_step_retval progress = TEXT_PRINT_STEP_CONTINUE;
			while (steps < 4 && progress != TEXT_PRINT_STEP_STOP) {
				steps++;
				progress = text_print_step(
					&view_model.text_print_step_state);
			}

			if (TEXT_PRINT_STEP_STOP == progress) {
				view_model.is_printing = false;
				view_model.print_y = view_model.text_print_step_state.current_point.y + bitmapfont.glyph_height;
			}

		} else {
			bool should_start_printing =
				(view_model.print_y > view_model.bgoffs_y ? 0 : 256) +
				view_model.print_y - view_model.bgoffs_y <
				DISPLAY_HEIGHT + 16;

			if (should_start_printing) {
				switch (view_model.resource.part) {
				case RESOURCEPART_TITLE:
					text_print_step_init(
						&view_model.text_print_step_state,
						vram.bg_charblock[TEXT_CHARBLOCK],
						&whole_screen_window,
						&bitmapfont,
						(coord16_t) {8, view_model.print_y},
						kerning_title,
						overflow_wordwrapx_aroundy,
						colors_header,
						view_model.resource.current->primary.title);

					view_model.is_printing = true;
					view_model.resource.part += 1;
					break;
				case RESOURCEPART_RETRIEVEDFROM:
					text_print_step_init(
						&view_model.text_print_step_state,
						vram.bg_charblock[TEXT_CHARBLOCK],
						&whole_screen_window,
						&bitmapfont,
						(coord16_t) {14, view_model.print_y},
						kerning_normal,
						overflow_wordwrapx_aroundy,
						colors_url,
						view_model.resource.current->primary.retrieved_from);

					view_model.is_printing = true;
					view_model.resource.part += 1;
					break;
				case RESOURCEPART_By:
					text_print_step_init(
						&view_model.text_print_step_state,
						vram.bg_charblock[TEXT_CHARBLOCK],
						&whole_screen_window,
						&bitmapfont,
						(coord16_t) {14, view_model.print_y},
						kerning_normal,
						TEXTPRINTOVERFLOW_WRAPAROUND,
						colors_normal,
						"By: ");

					view_model.is_printing = true;
					view_model.resource.part += 1;
					break;
				case RESOURCEPART_AUTHOR:
					view_model.print_y -= bitmapfont.glyph_height;
					text_print_step_init(
						&view_model.text_print_step_state,
						vram.bg_charblock[TEXT_CHARBLOCK],
						&whole_screen_window,
						&bitmapfont,
						(coord16_t) {32, view_model.print_y},
						kerning_normal,
						overflow_wordwrapx_aroundy,
						colors_normal,
						view_model.resource.current->primary.author);

					view_model.is_printing = true;
					view_model.resource.part += 1;
					break;
				case RESOURCEPART_AUTHORURL:
					text_print_step_init(
						&view_model.text_print_step_state,
						vram.bg_charblock[TEXT_CHARBLOCK],
						&whole_screen_window,
						&bitmapfont,
						(coord16_t) {32, view_model.print_y},
						kerning_normal,
						overflow_wordwrapx_aroundy,
						colors_url,
						view_model.resource.current->primary.author_url);

					view_model.is_printing = true;
					view_model.resource.part += 1;
					break;
				case RESOURCEPART_LICENSE:
					text_print_step_init(
						&view_model.text_print_step_state,
						vram.bg_charblock[TEXT_CHARBLOCK],
						&whole_screen_window,
						&bitmapfont,
						(coord16_t) {14, view_model.print_y},
						kerning_normal,
						overflow_wordwrapx_aroundy,
						colors_normal,
						view_model.resource.current->primary.licensed_under);

					view_model.is_printing = true;
					if (0 == view_model.resource.current->derived_from.title[0]) {
						view_model.resource.part = RESOURCEPART_ADVANCE;
					} else {
						view_model.resource.part += 1;
					}
					break;
				case RESOURCEPART_DerivedFrom:
					text_print_step_init(
						&view_model.text_print_step_state,
						vram.bg_charblock[TEXT_CHARBLOCK],
						&whole_screen_window,
						&bitmapfont,
						(coord16_t) {14, view_model.print_y},
						kerning_normal,
						TEXTPRINTOVERFLOW_WRAPAROUND,
						colors_normal,
						"Derived From: ");

					view_model.is_printing = true;
					view_model.resource.part += 1;
					break;
				case RESOURCEPART_DERIVED_TITLE:
					text_print_step_init(
						&view_model.text_print_step_state,
						vram.bg_charblock[TEXT_CHARBLOCK],
						&whole_screen_window,
						&bitmapfont,
						(coord16_t) {22, view_model.print_y},
						kerning_title,
						overflow_wordwrapx_aroundy,
						colors_header,
						view_model.resource.current->derived_from.title);

					view_model.is_printing = true;
					view_model.resource.part += 1;
					break;
				case RESOURCEPART_DERIVED_RETRIEVEDFROM:
					text_print_step_init(
						&view_model.text_print_step_state,
						vram.bg_charblock[TEXT_CHARBLOCK],
						&whole_screen_window,
						&bitmapfont,
						(coord16_t) {28, view_model.print_y},
						kerning_normal,
						overflow_wordwrapx_aroundy,
						colors_url,
						view_model.resource.current->derived_from.retrieved_from);

					view_model.is_printing = true;
					view_model.resource.part += 1;
					break;
				case RESOURCEPART_DERIVED_By:
					text_print_step_init(
						&view_model.text_print_step_state,
						vram.bg_charblock[TEXT_CHARBLOCK],
						&whole_screen_window,
						&bitmapfont,
						(coord16_t) {28, view_model.print_y},
						kerning_normal,
						TEXTPRINTOVERFLOW_WRAPAROUND,
						colors_normal,
						"By: ");

					view_model.is_printing = true;
					view_model.resource.part += 1;
					break;
				case RESOURCEPART_DERIVED_AUTHOR:
					view_model.print_y -= bitmapfont.glyph_height;
					text_print_step_init(
						&view_model.text_print_step_state,
						vram.bg_charblock[TEXT_CHARBLOCK],
						&whole_screen_window,
						&bitmapfont,
						(coord16_t) {46, view_model.print_y},
						kerning_normal,
						overflow_wordwrapx_aroundy,
						colors_normal,
						view_model.resource.current->derived_from.author);

					view_model.is_printing = true;
					view_model.resource.part += 1;
					break;
				case RESOURCEPART_DERIVED_AUTHORURL:
					text_print_step_init(
						&view_model.text_print_step_state,
						vram.bg_charblock[TEXT_CHARBLOCK],
						&whole_screen_window,
						&bitmapfont,
						(coord16_t) {46, view_model.print_y},
						kerning_normal,
						overflow_wordwrapx_aroundy,
						colors_url,
						view_model.resource.current->derived_from.author_url);

					view_model.is_printing = true;
					view_model.resource.part += 1;
					break;
				case RESOURCEPART_DERIVED_LICENSE:
					text_print_step_init(
						&view_model.text_print_step_state,
						vram.bg_charblock[TEXT_CHARBLOCK],
						&whole_screen_window,
						&bitmapfont,
						(coord16_t) {28, view_model.print_y},
						kerning_normal,
						overflow_wordwrapx_aroundy,
						colors_normal,
						view_model.resource.current->derived_from.licensed_under);

					view_model.is_printing = true;
					if (0 == view_model.resource.current->derived_from.title[0]) {
						view_model.resource.part = RESOURCEPART_ADVANCE;
					} else {
						view_model.resource.part += 1;
					}
					break;
				case RESOURCEPART_ADVANCE:
					view_model.resource.current += 1;
					view_model.print_y += bitmapfont.glyph_height;
					if (view_model.resource.current == resource_credits_end) {
						scene_onframe_callback = &MainCB_credits_scrollUntilBlank;
					} else {
						view_model.resource.part = RESOURCEPART_TITLE;
					}
					break;
				}
			}
		}
	}
}

static void MainCB_credits_scrollUntilBlank(void) {
	if (! keyinput_get_new().b) {
		exitCredits();
	} else if (view_model.delay) {
		view_model.delay--;
	} else {
		view_model.delay = SCROLL_FREQUENCY;
		view_model.bgoffs_y += 1;

		unsigned row_to_clear = (256 + view_model.bgoffs_y - 2) % 256;
		text_clear_immediate(
			vram.bg_charblock[TEXT_CHARBLOCK],
			&whole_screen_window,
			(coord16_t) {0, row_to_clear},
			(coord16_t) {DISPLAY_WIDTH, row_to_clear + 1},
			ANSI_PALETTE_BLACK);

		vram_op_queue_enqueue(&(struct vram_op) {
			.type = VRAM_QUEUE_OP_UINT16,
			.uint16 = {
				.value = view_model.bgoffs_y,
				.to = &reg_lcd.BGOFS[TEXT_LAYER].v,
			},
		});

		if (view_model.print_y == view_model.bgoffs_y) {
			scene_onframe_callback = &MainCB_credits_stall;
		}
	}
}

[[maybe_unused]]
static void MainCB_credits_stall(void) {
	if (! keyinput_get_new().b) {
		exitCredits();
	}
}
