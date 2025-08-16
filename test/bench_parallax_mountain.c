#include "scene/parallax_mountain_dusk.h"

#include <stddef.h>
#include <stdio.h>
#include "management/isr.h"
#include "gba/bios.h"
#include "management/vram_op_queue.h"
#include "benchmarks.h"
#include "main.h"
#include "mgba.h"

static unsigned total;
static unsigned failed;

MainCallback scene_onframe_callback;

void MainCB_mainMenu_init(void) {
	asm(
		"movs	r0,	#1\n\t"
		"swi	#0x00"
	);
}

void setUp(void){}
void tearDown(void){}


void run_parallaxMountainDusk_init_benchmark() {
	setUp();
	VBlankIntrWait();
	benchmark_start();
	MainCB_parallaxMountainDusk_init();
	uint32_t time = benchmark_stop();
	{
		MgbaPrintf(MGBA_LOG_INFO, "MainCB_parallaxMountainDusk_init: \033[44mBENCH\033[0m: %8ld cycles = %2ld.%03ld frames",
			time, time / CYCLES_PER_FRAME, (time * 1000 / CYCLES_PER_FRAME) % 1000);
	}

	VBlankIntrWait();
	{
		benchmark_start();
		vram_op_queue_execute();
		time = benchmark_stop();
		{
			MgbaPrintf(MGBA_LOG_INFO, "   - vram_op_queue_execute: \033[44mBENCH\033[0m: %8ld cycles = %2ld.%03ld frames",
				time, time / CYCLES_PER_FRAME, (time * 1000 / CYCLES_PER_FRAME) % 1000);
		}
	}
	tearDown();
	++total;
}


int main() {
	total = 0;
	failed = 0;

	isr_switchboard_init();
	isr_enable(II_VBLANK);
	MgbaOpen();

	run_parallaxMountainDusk_init_benchmark();

	MgbaPrintf(MGBA_LOG_INFO, "Total: %d; Failing: %d", total, failed);
	return failed != 0;
}
