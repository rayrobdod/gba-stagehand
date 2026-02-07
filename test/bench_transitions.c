#include "scene/dmg_music_using_notation.h"

#include <stddef.h>
#include <stdio.h>
#include "management/isr.h"
#include "management/keyinput.h"
#include "gba/bios.h"
#include "management/vram_op_queue.h"
#include "transition/palette_fade.h"
#include "transition/star_iris.h"
#include "benchmarks.h"
#include "main.h"
#include "mgba.h"

static unsigned total;
static unsigned failed;

MainCallback scene_onframe_callback;


union palette512 InitFadeIn_null(void) {
	union palette512 retval = {0};
	return retval;
}
void MainCB_null(void){}
const struct transitionTargetCallbacks transitionTargetCbs_null = {
	NULL,
	NULL,
	InitFadeIn_null,
	NULL,
	MainCB_null,
};
const struct transitionSourceCallbacks transitionSourceCbs_null = {
	NULL,
	NULL,
};

void setUp(void){}
void tearDown(void){}


static void bench_transition(const char* name, const struct transition* transition) {
	uint32_t frameNo = 0;
	MgbaPrintf(MGBA_LOG_INFO, "Transition %s: \033[44mBENCH\033[0m", name);

	uint32_t opsqueue_maxtime = 0;
	uint32_t opsqueue_maxframe = 0;
	uint32_t scene_maxtime = 0;
	uint32_t scene_maxframe = 0;
	uint32_t total_maxtime = 0;
	uint32_t total_maxframe = 0;

	StartTransition(
		transition,
		&transitionSourceCbs_null,
		&transitionTargetCbs_null);

	while (MainCB_null != scene_onframe_callback) {
		VBlankIntrWait();

		benchmark_start();
		vram_op_queue_execute();
		uint32_t opsqueue_time = benchmark_stop();

		if (opsqueue_maxtime < opsqueue_time) {
			opsqueue_maxtime = opsqueue_time;
			opsqueue_maxframe = frameNo;
		}

		benchmark_start();
		scene_onframe_callback();
		uint32_t scene_time = benchmark_stop();

		if (scene_maxtime < scene_time) {
			scene_maxtime = scene_time;
			scene_maxframe = frameNo;
		}

		if (total_maxtime < scene_time + opsqueue_time) {
			total_maxtime = scene_time + opsqueue_time;
			total_maxframe = frameNo;
		}

		if (false)
		{
			const char* opsqueue_color = (opsqueue_time > CYCLES_PER_VBLANK ? "\033[43m" : "\033[0m");
			MgbaPrintf(MGBA_LOG_INFO, "    [%3ld] vram_ops: %s%8ld cycles = %2ld.%03ld frames\033[0m",
				frameNo, opsqueue_color, opsqueue_time, opsqueue_time / CYCLES_PER_FRAME, (opsqueue_time * 1000 / CYCLES_PER_FRAME) % 1000);

			const char* scene_color = ((scene_time + opsqueue_time) > (CYCLES_PER_FRAME) ? "\033[43m" : "\033[0m");
			MgbaPrintf(MGBA_LOG_INFO, "             scene: %s%8ld cycles = %2ld.%03ld frames\033[0m",
				scene_color, scene_time, scene_time / CYCLES_PER_FRAME, (scene_time * 1000 / CYCLES_PER_FRAME) % 1000);
		}

		++frameNo;
	}

	MgbaPrintf(MGBA_LOG_INFO, "frames:        %3ld", frameNo);

	const char* opsqueue_color = (opsqueue_maxtime > CYCLES_PER_VBLANK ? "\033[43m" : "\033[0m");
	MgbaPrintf(MGBA_LOG_INFO, "opsqueue_max: [%3ld] %s%8ld cycles = %2ld.%03ld frames\033[0m",
		opsqueue_maxframe, opsqueue_color, opsqueue_maxtime, opsqueue_maxtime / CYCLES_PER_FRAME, (opsqueue_maxtime * 1000 / CYCLES_PER_FRAME) % 1000);

	const char* scene_color = (scene_maxtime > CYCLES_PER_FRAME ? "\033[43m" : "\033[0m");
	MgbaPrintf(MGBA_LOG_INFO, "scene_max:    [%3ld] %s%8ld cycles = %2ld.%03ld frames\033[0m",
		scene_maxframe, scene_color, scene_maxtime, scene_maxtime / CYCLES_PER_FRAME, (scene_maxtime * 1000 / CYCLES_PER_FRAME) % 1000);

	const char* total_color = (total_maxtime > CYCLES_PER_FRAME ? "\033[43m" : "\033[0m");
	MgbaPrintf(MGBA_LOG_INFO, "total_max:    [%3ld] %s%8ld cycles = %2ld.%03ld frames\033[0m",
		total_maxframe, total_color, total_maxtime, total_maxtime / CYCLES_PER_FRAME, (total_maxtime * 1000 / CYCLES_PER_FRAME) % 1000);
}


int main() {
	total = 0;
	failed = 0;

	reg_interrupt.WAITCNT = common_waitcnt;
	isr_switchboard_init();
	isr_enable(II_VBLANK);
	MgbaOpen();

	bench_transition("starIris", &transition_starIris);
	bench_transition("paletteFade_coral", &transition_paletteFade_coral);

	return failed != 0;
}
