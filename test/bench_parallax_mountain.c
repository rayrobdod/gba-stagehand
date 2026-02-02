#include "scene/parallax_mountain_dusk.h"

#include <stddef.h>
#include <stdio.h>
#include "management/isr.h"
#include "management/keyinput.h"
#include "gba/bios.h"
#include "management/vram_op_queue.h"
#include "benchmarks.h"
#include "main.h"
#include "mgba.h"

static unsigned total;
static unsigned failed;

MainCallback scene_onframe_callback;
void MainCB_parallaxMountainDusk_main(void);

void MainCB_mainMenu_init(void) {
	MgbaPrintf(MGBA_LOG_INFO, "ENTER: MainCB_mainMenu_init");
	asm(
		"movs	r0,	#1\n\t"
		"swi	#0x00"
	);
}

void setUp(void){}
void tearDown(void){}

void fadeCb(void) {}


static void run_parallaxMountainDusk_initialization_benchmark() {
	unsigned frameNo = 0;
	MgbaPrintf(MGBA_LOG_INFO, "parallaxMountainDusk: \033[44mBENCH\033[0m");

	ChangeScene_parallaxMountainDusk(fadeCb);

	while (MainCB_parallaxMountainDusk_main != scene_onframe_callback) {
		VBlankIntrWait();

		benchmark_start();
		vram_op_queue_execute();
		uint32_t opsqueue_time = benchmark_stop();

		benchmark_start();
		scene_onframe_callback();
		uint32_t scene_time = benchmark_stop();

		const char* opsqueue_color = (opsqueue_time > CYCLES_PER_VBLANK ? "\033[43m" : "\033[0m");
		MgbaPrintf(MGBA_LOG_INFO, "    [%3d] vram_ops: %s%8ld cycles = %2ld.%03ld frames\033[0m",
			frameNo, opsqueue_color, opsqueue_time, opsqueue_time / CYCLES_PER_FRAME, (opsqueue_time * 1000 / CYCLES_PER_FRAME) % 1000);

		const char* scene_color = ((scene_time + opsqueue_time) > (CYCLES_PER_FRAME) ? "\033[43m" : "\033[0m");
		MgbaPrintf(MGBA_LOG_INFO, "             scene: %s%8ld cycles = %2ld.%03ld frames\033[0m",
			scene_color, scene_time, scene_time / CYCLES_PER_FRAME, (scene_time * 1000 / CYCLES_PER_FRAME) % 1000);

		++frameNo;
	}
}


int main() {
	total = 0;
	failed = 0;

	reg_interrupt.WAITCNT = common_waitcnt;
	isr_switchboard_init();
	isr_enable(II_VBLANK);
	MgbaOpen();

	run_parallaxMountainDusk_initialization_benchmark();

	return failed != 0;
}
