#ifndef GUARD_GBA_HW_REG_H
#define GUARD_GBA_HW_REG_H

#include <stdbool.h>
#include <stdint.h>
#include "gba/shared.h"

typedef enum {
	OBJ_CHAR_MAP_2D = 0,
	OBJ_CHAR_MAP_1D = 1,
} obj_character_mapping_t;

typedef struct {
	uint8_t mode: 3;
	bool cbg_mode: 1;
	uint8_t display_frame: 1;
	bool hblank_interval_free: 1;
	obj_character_mapping_t obj_character_mapping: 1;
	bool force_blank: 1;
	bool enable_bg0: 1;
	bool enable_bg1: 1;
	bool enable_bg2: 1;
	bool enable_bg3: 1;
	bool enable_obj: 1;
	bool enable_win0: 1;
	bool enable_win1: 1;
	bool enable_win_obj: 1;
} dispcnt_t;

enum bgcnt_size {
	BGCNT_SIZE_TEXT_SMALL = 0,
	BGCNT_SIZE_TEXT_TALL = 1,
	BGCNT_SIZE_TEXT_WIDE = 2,
	BGCNT_SIZE_TEXT_BIG = 3,
	BGCNT_SIZE_AFFINE_128 = 0,
	BGCNT_SIZE_AFFINE_256 = 1,
	BGCNT_SIZE_AFFINE_512 = 2,
	BGCNT_SIZE_AFFINE_1024 = 3,
};

typedef struct {
	uint8_t priority: 2;
	uint8_t charblock : 2;
	uint8_t : 2;
	bool mosaic: 1;
	palette_mode_t palette_mode: 1;
	uint8_t screenblock: 5;
	bool overflow_wraparound: 1;
	enum bgcnt_size size: 2;
} bgcnt_t;

typedef struct {
	bool vblank_flag: 1;
	bool	hblank_flag: 1;
	bool vcounter_flag: 1;
	bool vblank_enable: 1;
	bool hblank_enable: 1;
	bool vcounter_enable: 1;
	uint8_t : 2;
	uint16_t vcounter: 8;
} dispstat_t;

typedef struct {
	uint8_t right;
	uint8_t left;
} window_horizontal_t;

typedef struct {
	uint8_t down;
	uint8_t up;
} window_vertical_t;

typedef struct {
	bool bg0: 1;
	bool bg1: 1;
	bool bg2: 1;
	bool bg3: 1;
	bool obj: 1;
	bool color_special: 1;
	bool : 1;
	bool : 1;
} window_enable_t;

typedef struct {
	window_enable_t win0;
	window_enable_t win1;
} window_enable_pair_t;

[[gnu::unused]]
static window_enable_t WIN_ENABLE_ALL = {true, true, true, true, true, true};

typedef struct bgofs {
	uint16_t h;
	uint16_t v;
} bgofs_t;

enum color_effect {
	COLOR_EFFECT_NONE = 0,
	COLOR_EFFECT_BLEND = 1,
	COLOR_EFFECT_BRIGHTEN = 2,
	COLOR_EFFECT_DARKEN = 3,
};

typedef struct {
	bool a_bg0 : 1;
	bool a_bg1 : 1;
	bool a_bg2 : 1;
	bool a_bg3 : 1;
	bool a_obj : 1;
	bool a_backdrop : 1;
	enum color_effect color_effect : 2;
	bool b_bg0 : 1;
	bool b_bg1 : 1;
	bool b_bg2 : 1;
	bool b_bg3 : 1;
	bool b_obj : 1;
	bool b_backdrop : 1;
} bldcnt_t;

typedef struct {
	uint16_t a : 5;
	uint16_t : 3;
	uint16_t b : 5;
	uint16_t : 3;
} bldalpha_t;

struct reg_lcd {
	dispcnt_t DISPCNT;	/* 4000000 */
	uint16_t : 16;	/* 4000002 */
	dispstat_t DISPSTAT;	/* 4000004 */
	uint16_t VCOUNT;	/* 4000006 */
	bgcnt_t BGCNT[4];	/* 4000008 */
	struct bgofs BGOFS[4];	/* 4000010 */
	uint16_t BG2PA;	/* 4000020 */
	uint16_t BG2PB;	/* 4000022 */
	uint16_t BG2PC;	/* 4000024 */
	uint16_t BG2PD;	/* 4000026 */
	uint32_t BG2X;	/* 4000028 */
	uint32_t BG2Y;	/* 400002C */
	uint16_t BG3PA;	/* 4000030 */
	uint16_t BG3PB;	/* 4000032 */
	uint16_t BG3PC;	/* 4000034 */
	uint16_t BG3PD;	/* 4000036 */
	uint32_t BG3X;	/* 4000038 */
	uint32_t BG3Y;	/* 400003C */
	window_horizontal_t WIN0H;	/* 4000040 */
	window_horizontal_t WIN1H;	/* 4000042 */
	window_vertical_t WIN0V;	/* 4000044 */
	window_vertical_t WIN1V;	/* 4000046 */
	window_enable_pair_t WININ;	/* 4000048 */
	window_enable_pair_t WINOUT;	/* 400004A */
	uint16_t MOSAIC;	/* 400004C */
	uint16_t : 16;	/* 400004E */
	bldcnt_t BLDCNT;	/* 4000050 */
	bldalpha_t BLDALPHA;	/* 4000052 */
	uint16_t BLDY;	/* 4000054 */
};

extern volatile struct reg_lcd reg_lcd;

// Sound Registers

enum sweep_direction {
	SWEEP_INCREASE = 0,
	SWEEP_DECREASE = 1,
};

typedef struct {
	uint16_t shift : 3;
	enum sweep_direction direction : 1;
	uint16_t time : 3;
	uint16_t : 9;
} reg_sound_tone_sweep_t;

enum envelope_direction {
	ENVELOPE_DECREASE = 0,
	ENVELOPE_INCREASE = 1,
};

enum pattern_duty {
	WAVE_DUTY_12 = 0,
	WAVE_DUTY_25 = 1,
	WAVE_DUTY_50 = 2,
	WAVE_DUTY_75 = 3,
};

typedef struct {
	uint16_t length : 6;
	enum pattern_duty pattern_duty : 2;
	uint16_t step_time : 3;
	enum envelope_direction direction : 1;
	uint16_t volume : 4;
} reg_sound_tone_envelope_t;

typedef struct {
	uint16_t frequency : 11;
	uint16_t : 3;
	bool stop : 1;
	bool restart : 1;
} reg_sound_frequency_control_t;

struct reg_sound_1 {
	reg_sound_tone_sweep_t L;
	reg_sound_tone_envelope_t H;
	reg_sound_frequency_control_t X;
	uint16_t : 16;
};

struct reg_sound_2 {
	reg_sound_tone_envelope_t L;
	uint16_t : 16;
	reg_sound_frequency_control_t H;
	uint16_t : 16;
};

typedef struct {
	uint16_t : 5;
	uint16_t dimension : 1;
	uint16_t bank_no : 1;
	bool enabled : 1;
	uint16_t : 8;
} reg_sound_wave_ram_select_t;

enum wave_volume {
	WAVE_VOLUME_MUTE = 0,
	WAVE_VOLUME_100 = 2,
	WAVE_VOLUME_50 = 4,
	WAVE_VOLUME_25 = 6,
	WAVE_VOLUME_75 = 1,
	WAVE_VOLUME_75_a = 3,
	WAVE_VOLUME_75_b = 5,
	WAVE_VOLUME_75_c = 7,
};

typedef struct {
	uint16_t length : 8;
	uint16_t : 5;
	enum wave_volume volume : 3;
} reg_sound_wave_ram_volume_t;

struct reg_sound_3 {
	reg_sound_wave_ram_select_t L;
	reg_sound_wave_ram_volume_t H;
	reg_sound_frequency_control_t X;
	uint16_t : 16;
};

typedef struct {
	uint16_t length : 6;
	uint16_t : 2;
	uint16_t step_time : 3;
	enum envelope_direction direction : 1;
	uint16_t volume : 4;
} reg_sound_noise_envelope_t;

enum noise_counter_width {
	NOISE_COUNTER_WIDTH_15 = 0,
	NOISE_COUNTER_WIDTH_7 = 1,
};

typedef struct {
	uint16_t ratio : 3;
	enum noise_counter_width counter_width : 1;
	uint16_t shift_frequency : 4;
	uint16_t : 6;
	bool stop : 1;
	bool restart : 1;
} reg_sound_noise_frequency_control_t;

struct reg_sound_4 {
	reg_sound_noise_envelope_t L;
	uint16_t : 16;
	reg_sound_noise_frequency_control_t H;
	uint16_t : 16;
};

typedef struct {
	uint16_t left_volume : 3;
	uint16_t : 1;
	uint16_t right_volume : 3;
	uint16_t : 1;
	bool left_1 : 1;
	bool left_2 : 1;
	bool left_3 : 1;
	bool left_4 : 1;
	bool right_1 : 1;
	bool right_2 : 1;
	bool right_3 : 1;
	bool right_4 : 1;
} reg_sound_control_dmg_t;

enum sound_ratio {
	SOUND_RATIO_25 = 0,
	SOUND_RATIO_50 = 1,
	SOUND_RATIO_100 = 2,
};

typedef struct {
	enum sound_ratio ratio : 2;
	uint16_t ratio_a : 1;
	uint16_t ratio_b : 1;
	uint16_t : 4;
	bool a_right : 1;
	bool a_left : 1;
	bool a_timer : 1;
	bool a_reset : 1;
	bool b_right : 1;
	bool b_left : 1;
	bool b_timer : 1;
	bool b_reset : 1;
} reg_sound_control_direct_t;

typedef struct {
	bool status_1 : 1;
	bool status_2 : 1;
	bool status_3 : 1;
	bool status_4 : 1;
	uint16_t : 3;
	bool enable : 1;
	uint16_t : 8;
} reg_sound_control_master_t;

[[gnu::unused]]
static reg_sound_control_master_t REG_SOUND_ENABLE = {.enable = true};

struct reg_sound_master {
	reg_sound_control_dmg_t L;
	reg_sound_control_direct_t H;
	reg_sound_control_master_t X;
	uint16_t : 16;
};

typedef struct {
	uint16_t : 1;
	uint16_t bias_level : 9;
	uint16_t : 4;
	uint16_t amplitude_resolution : 2;
} reg_sound_bias_t;

struct reg_sound_bias {
	reg_sound_bias_t bias;
	uint16_t : 16;
	uint16_t : 16;
	uint16_t : 16;
};

struct reg_sound {
	struct reg_sound_1 _1;
	struct reg_sound_2 _2;
	struct reg_sound_3 _3;
	struct reg_sound_4 _4;
	struct reg_sound_master master;
	struct reg_sound_bias bias;
	uint16_t wave_ram[8];
	uint32_t channel_A;
	uint32_t channel_B;
};

extern volatile struct reg_sound reg_sound;

// DMA Registers

typedef enum {
	DMA_ADDR_INCREMENT = 0,
	DMA_ADDR_DECREMENT = 1,
	DMA_ADDR_FIXED = 2,
	DMA_ADDR_RELOAD = 3,
} dma_addr_control_t;

typedef enum {
	DMA_START_IMMEDIATELY = 0,
	DMA_START_VBLANK = 1,
	DMA_START_HBLANK = 2,
	DMA_START_SPECIAL = 3,
} dma_start_t;

typedef struct __attribute((packed)) {
	short : 5;
	dma_addr_control_t dest_control : 2;
	dma_addr_control_t src_control : 2;
	bool repeat : 1;
	enum WordSize word_size : 1;
	bool video_capture_mode : 1;
	dma_start_t start : 2;
	bool irq : 1;
	bool enable : 1;
} dma_control_t;

struct reg_dma {
	const void* source;
	volatile void* destination;
	uint16_t word_count;
	dma_control_t control;
};

extern volatile struct reg_dma reg_dma[4];

// Timer Registers

enum timer_prescaler {
	TIMER_PRESCALER_1 = 0,
	TIMER_PRESCALER_64 = 1,
	TIMER_PRESCALER_256 = 2,
	TIMER_PRESCALER_1024 = 3,
};

typedef struct {
	enum timer_prescaler prescaler: 2;
	bool cascade: 1;
	uint16_t : 3;
	bool irq_enable : 1;
	bool timer_enable : 1;
	uint16_t : 8;
} timer_control_t;

struct reg_timer {
	uint16_t counter;
	timer_control_t control;
};

extern volatile struct reg_timer reg_timer[4]; /* 4000100 */

#if 0
Serial Communication (1)

  uint32_t SIODATA32;	/* 4000120 */
  uint16_t SIOMULTI0;	/* 4000120 */
  uint16_t SIOMULTI1;	/* 4000122 */
  uint16_t SIOMULTI2;	/* 4000124 */
  uint16_t SIOMULTI3;	/* 4000126 */
  uint16_t SIOCNT;	/* 4000128 */
  uint16_t SIOMLT_SEND;	/* 400012A */
  uint16_t SIODATA8;	/* 400012A */
  uint32_t _unused;	/* 400012C */
#endif

typedef struct {
	bool a : 1;
	bool b : 1;
	bool select : 1;
	bool start : 1;
	bool right : 1;
	bool left : 1;
	bool up : 1;
	bool down : 1;
	bool r : 1;
	bool l : 1;
} keypad_t;

typedef enum {
	KEYPAD_CONDITION_OR = 0,
	KEYPAD_CONDITION_AND = 1,
} keypad_condition_t;

typedef struct {
	bool a : 1;
	bool b : 1;
	bool select : 1;
	bool start : 1;
	bool right : 1;
	bool left : 1;
	bool up : 1;
	bool down : 1;
	bool r : 1;
	bool l : 1;
	unsigned char  : 4;
	bool enable : 1;
	keypad_condition_t condition : 1;
} keypad_control_t;

struct reg_keypad {
	keypad_t KEYINPUT;	/* 4000130 */
	keypad_control_t KEYCNT;	/* 4000132 */
};

extern volatile struct reg_keypad reg_keypad;

#if 0

Serial Communication (2)

  uint16_t RCNT;	/* 4000134 */
  uint16_t IR;	/* 4000136 */
  uint16_t _unused;	/* 4000138 */
  uint16_t JOYCNT;	/* 4000140 */
  uint16_t _unused;	/* 4000142 */
  uint32_t JOY_RECV;	/* 4000150 */
  uint32_t JOY_TRANS;	/* 4000154 */
  uint16_t JOYSTAT;	/* 4000158 */
#endif

struct reg_interrupt {
  interrupt_flag_t IE;	/* 4000200 */
  interrupt_flag_t IF;	/* 4000202 */
  uint16_t WAITCNT;	/* 4000204 */
  uint16_t : 16;	/* 4000206 */
  uint16_t IME;	/* 4000208 */
};

extern volatile struct reg_interrupt reg_interrupt;

#endif        //  #ifndef HW_REG_H
