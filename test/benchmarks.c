#include "benchmarks.h"

#include "gba/bios.h"
#include "gba/hw_reg.h"
#include "mgba.h"

static unsigned total;
static unsigned failed;
char fail_detail[256];

void benchmark_start() {
	reg_timer[2].counter = 0;
	reg_timer[3].counter = 0;
	reg_timer[2].control = (timer_control_t) {0};
	reg_timer[3].control = (timer_control_t) {0};
	reg_timer[3].control = (timer_control_t) {
		0,
		.cascade = true,
		.timer_enable = true,
	};
	reg_timer[2].control = (timer_control_t) {
		0,
		.timer_enable = true,
	};
}

uint32_t benchmark_stop() {
	reg_timer[2].control = (timer_control_t) {0};
	reg_timer[3].control = (timer_control_t) {0};
	return (reg_timer[3].counter << 16) | (reg_timer[2].counter);
}

void run_benchmark(void (*fn)(void), bool (*check)(void), const char* name) {
	setUp();
	VBlankIntrWait();
	benchmark_start();
	fn();
	uint32_t time = benchmark_stop();
	bool currentTestSucceeded = check();
	tearDown();
	if (currentTestSucceeded) {
		MgbaPrintf(MGBA_LOG_INFO, "%s: \033[44mBENCH\033[0m: %ld cycles = %ld.%03ld frames",
			name, time, time / CYCLES_PER_FRAME, (time * 1000 / CYCLES_PER_FRAME) % 1000);
	} else {
		++failed;
		MgbaPrintf(MGBA_LOG_INFO, "%s: \033[41mFAIL\033[0m: %s",
			name, fail_detail);
	}
	++total;
}

#include "management/vram_op_queue.h"
#include "main.h"

void run_transition_benchmark(
		const struct transition* transition,
		const struct transitionSourceCallbacks* source,
		const struct transitionTargetCallbacks* target,
		const char* name,
		enum run_transition_benchmark_verbosity verbosity) {
	uint32_t frameNo = 0;
	MgbaPrintf(MGBA_LOG_INFO, "%s: \033[44mBENCH\033[0m", name);

	uint32_t opsqueue_maxtime = 0;
	uint32_t opsqueue_maxframe = 0;
	uint32_t scene_maxtime = 0;
	uint32_t scene_maxframe = 0;
	uint32_t total_maxtime = 0;
	uint32_t total_maxframe = 0;

	StartTransition(transition, source, target);

	while (target->target != scene_onframe_callback) {
		VBlankIntrWait();

		benchmark_start();
		vram_op_queue_execute();
		uint32_t opsqueue_time = benchmark_stop();

		benchmark_start();
		scene_onframe_callback();
		uint32_t scene_time = benchmark_stop();

		if (opsqueue_maxtime < opsqueue_time) {
			opsqueue_maxtime = opsqueue_time;
			opsqueue_maxframe = frameNo;
		}

		if (scene_maxtime < scene_time) {
			scene_maxtime = scene_time;
			scene_maxframe = frameNo;
		}

		if (total_maxtime < scene_time + opsqueue_time) {
			total_maxtime = scene_time + opsqueue_time;
			total_maxframe = frameNo;
		}

		if (verbosity & RTBV_ALL_FRAMES) {
			const char* opsqueue_color = (opsqueue_time > CYCLES_PER_VBLANK ? "\033[43m" : "\033[0m");
			MgbaPrintf(MGBA_LOG_INFO, "    [%3ld] vram_ops: %s%8ld cycles = %2ld.%03ld frames\033[0m",
				frameNo, opsqueue_color, opsqueue_time, opsqueue_time / CYCLES_PER_FRAME, (opsqueue_time * 1000 / CYCLES_PER_FRAME) % 1000);

			const char* scene_color = ((scene_time + opsqueue_time) > (CYCLES_PER_FRAME) ? "\033[43m" : "\033[0m");
			MgbaPrintf(MGBA_LOG_INFO, "             scene: %s%8ld cycles = %2ld.%03ld frames\033[0m",
				scene_color, scene_time, scene_time / CYCLES_PER_FRAME, (scene_time * 1000 / CYCLES_PER_FRAME) % 1000);
		}

		++frameNo;
	}

	MgbaPrintf(MGBA_LOG_INFO, "    frames:        %3ld", frameNo);

	const char* opsqueue_color = (opsqueue_maxtime > CYCLES_PER_VBLANK ? "\033[43m" : "\033[0m");
	MgbaPrintf(MGBA_LOG_INFO, "    opsqueue_max: [%3ld] %s%8ld cycles = %2ld.%03ld frames\033[0m",
		opsqueue_maxframe, opsqueue_color, opsqueue_maxtime, opsqueue_maxtime / CYCLES_PER_FRAME, (opsqueue_maxtime * 1000 / CYCLES_PER_FRAME) % 1000);

	const char* scene_color = (scene_maxtime > CYCLES_PER_FRAME ? "\033[43m" : "\033[0m");
	MgbaPrintf(MGBA_LOG_INFO, "    scene_max:    [%3ld] %s%8ld cycles = %2ld.%03ld frames\033[0m",
		scene_maxframe, scene_color, scene_maxtime, scene_maxtime / CYCLES_PER_FRAME, (scene_maxtime * 1000 / CYCLES_PER_FRAME) % 1000);

	const char* total_color = (total_maxtime > CYCLES_PER_FRAME ? "\033[43m" : "\033[0m");
	MgbaPrintf(MGBA_LOG_INFO, "    total_max:    [%3ld] %s%8ld cycles = %2ld.%03ld frames\033[0m",
		total_maxframe, total_color, total_maxtime, total_maxtime / CYCLES_PER_FRAME, (total_maxtime * 1000 / CYCLES_PER_FRAME) % 1000);
}
