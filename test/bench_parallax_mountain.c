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

	MainCB_parallaxMountainDusk(fadeCb);

	while (MainCB_parallaxMountainDusk_main != scene_onframe_callback) {
		setUp();
		VBlankIntrWait();
		benchmark_start();
		scene_onframe_callback();
		uint32_t time = benchmark_stop();
		{
			const char* color = (time > (CYCLES_PER_FRAME * 3 / 4) ? "\033[43m" : "\033[0m");
			MgbaPrintf(MGBA_LOG_INFO, "  [%3d]:      %s%8ld cycles = %2ld.%03ld frames\033[0m",
				frameNo, color, time, time / CYCLES_PER_FRAME, (time * 1000 / CYCLES_PER_FRAME) % 1000);
		}

		//MgbaPrintf(MGBA_LOG_INFO, "    cb: %p", scene_onframe_callback);

		VBlankIntrWait();
		{
			benchmark_start();
			vram_op_queue_execute();
			time = benchmark_stop();
			{
				const char* color = (time > (CYCLES_PER_FRAME * 3 / 4) ? "\033[43m" : "\033[0m");
				MgbaPrintf(MGBA_LOG_INFO, "    vram_ops: %s%8ld cycles = %2ld.%03ld frames\033[0m",
					color, time, time / CYCLES_PER_FRAME, (time * 1000 / CYCLES_PER_FRAME) % 1000);
			}
		}
		tearDown();
		++frameNo;
	}
}


int main() {
	total = 0;
	failed = 0;

	isr_switchboard_init();
	isr_enable(II_VBLANK);
	MgbaOpen();

	run_parallaxMountainDusk_initialization_benchmark();

	return failed != 0;
}
