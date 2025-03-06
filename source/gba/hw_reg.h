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

typedef struct {
	uint8_t priority: 2;
	uint8_t charblock : 2;
	uint8_t unused: 2;
	bool mosaic: 1;
	palette_mode_t palette_mode: 1;
	uint8_t screenblock: 5;
	bool overflow_wraparound: 1;
	uint8_t size: 2;
} bgcnt_t;

typedef struct {
	bool vblank_flag: 1;
	bool	hblank_flag: 1;
	bool vcounter_flag: 1;
	bool vblank_enable: 1;
	bool hblank_enable: 1;
	bool vcounter_enable: 1;
	uint8_t _unused: 2;
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
	bool _unused6: 1;
	bool _unused7: 1;
} window_enable_t;

typedef struct {
	window_enable_t win0;
	window_enable_t win1;
} window_enable_pair_t;

__attribute__((unused))
static window_enable_t WIN_ENABLE_ALL = {true, true, true, true, true, true};

typedef struct {
	dispcnt_t DISPCNT;	/* 4000000 */
	uint16_t _unused0;	/* 4000002 */
	dispstat_t DISPSTAT;	/* 4000004 */
	uint16_t VCOUNT;	/* 4000006 */
	bgcnt_t BG0CNT;	/* 4000008 */
	bgcnt_t BG1CNT;	/* 400000A */
	bgcnt_t BG2CNT;	/* 400000C */
	bgcnt_t BG3CNT;	/* 400000E */
	uint16_t BG0HOFS;	/* 4000010 */
	uint16_t BG0VOFS;	/* 4000012 */
	uint16_t BG1HOFS;	/* 4000014 */
	uint16_t BG1VOFS;	/* 4000016 */
	uint16_t BG2HOFS;	/* 4000018 */
	uint16_t BG2VOFS;	/* 400001A */
	uint16_t BG3HOFS;	/* 400001C */
	uint16_t BG3VOFS;	/* 400001E */
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
	uint16_t _unused2;	/* 400004E */
	uint16_t BLDCNT;	/* 4000050 */
	uint16_t BLDALPHA;	/* 4000052 */
	uint16_t BLDY;	/* 4000054 */
} reg_lcd_t;

extern volatile reg_lcd_t reg_lcd;

#if 0
Sound Registers

  uint16_t SOUND1CNT_L;	/* 4000060 */
  uint16_t SOUND1CNT_H;	/* 4000062 */
  uint16_t SOUND1CNT_X;	/* 4000064 */
  uint16_t _unused;	/* 4000066 */
  uint16_t SOUND2CNT_L;	/* 4000068 */
  uint16_t _unused;	/* 400006A */
  uint16_t SOUND2CNT_H;	/* 400006C */
  uint16_t _unused;	/* 400006E */
  uint16_t SOUND3CNT_L;	/* 4000070 */
  uint16_t SOUND3CNT_H;	/* 4000072 */
  uint16_t SOUND3CNT_X;	/* 4000074 */
  uint16_t _unused;	/* 4000076 */
  uint16_t SOUND4CNT_L;	/* 4000078 */
  uint16_t _unused;	/* 400007A */
  uint16_t SOUND4CNT_H;	/* 400007C */
  uint16_t _unused;	/* 400007E */
  uint16_t SOUNDCNT_L;	/* 4000080 */
  uint16_t SOUNDCNT_H;	/* 4000082 */
  uint16_t SOUNDCNT_X;	/* 4000084 */
  uint16_t _unused;	/* 4000086 */
  uint16_t SOUNDBIAS;	/* 4000088 */
  400008Ah  ..   -    -         Not used
  4000090h 2x10h R/W  WAVE_RAM  Channel 3 Wave Pattern RAM (2 banks!!)
  uint32_t FIFO_A;	/* 40000A0 */
  uint32_t FIFO_B;	/* 40000A4 */
  uint16_t _unused;	/* 40000A8 */
#endif

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

#if 0
Timer Registers

  uint16_t TM0CNT_L;	/* 4000100 */
  uint16_t TM0CNT_H;	/* 4000102 */
  uint16_t TM1CNT_L;	/* 4000104 */
  uint16_t TM1CNT_H;	/* 4000106 */
  uint16_t TM2CNT_L;	/* 4000108 */
  uint16_t TM2CNT_H;	/* 400010A */
  uint16_t TM3CNT_L;	/* 400010C */
  uint16_t TM3CNT_H;	/* 400010E */
  uint16_t _unused;	/* 4000110 */

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
	unsigned char _unused : 4;
	bool enable : 1;
	keypad_condition_t condition : 1;
} keypad_control_t;

typedef struct {
	keypad_t KEYINPUT;	/* 4000130 */
	keypad_control_t KEYCNT;	/* 4000132 */
} reg_keypad_t;

extern volatile reg_keypad_t reg_keypad;

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

typedef struct {
  interrupt_flag_t IE;	/* 4000200 */
  interrupt_flag_t IF;	/* 4000202 */
  uint16_t WAITCNT;	/* 4000204 */
  uint16_t _unused3;	/* 4000206 */
  uint16_t IME;	/* 4000208 */
} reg_interrupt_t;

extern volatile reg_interrupt_t reg_interrupt;

#endif        //  #ifndef HW_REG_H
