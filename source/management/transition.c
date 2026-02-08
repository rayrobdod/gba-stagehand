#include "management/transition.h"

__attribute__((section(".sbss")))
struct {
	struct transition transition;
	struct transitionSourceCallbacks source;
	struct transitionTargetCallbacks target;
} active_transition;

static void MainCB_transition_fadeOut(void);
static void MainCB_transition_init(void);
static void MainCB_transition_fadeIn(void);

void StartTransition(
		const struct transition* main,
		const struct transitionSourceCallbacks* source,
		const struct transitionTargetCallbacks* target) {
	active_transition.transition = *main;
	active_transition.source = *source;
	active_transition.target = *target;

	if (active_transition.transition.initFadeOut)
		active_transition.transition.initFadeOut();
	if (active_transition.target.initFadeOut)
		active_transition.target.initFadeOut();

	scene_onframe_callback = &MainCB_transition_fadeOut;
}

static enum progress transition_progress_and(enum progress lhs, enum progress (*cb)(void)) {
	if (cb)
		return (COMPLETE == cb() && COMPLETE == lhs ? COMPLETE : ONGOING);
	else
		return lhs;
}

static void MainCB_transition_fadeOut(void) {
	enum progress progress = COMPLETE;

	if (active_transition.source.fadeOut)
		active_transition.source.fadeOut();

	progress = transition_progress_and(progress, active_transition.transition.fadeOut);
	progress = transition_progress_and(progress, active_transition.target.fadeOut);

	if (COMPLETE == progress)
		scene_onframe_callback = &MainCB_transition_init;
}

static void MainCB_transition_init(void) {
	if (active_transition.source.cleanup)
		active_transition.source.cleanup();

	union palette512 palette = active_transition.target.initFadeIn();
	active_transition.transition.initFadeIn(&palette);
	scene_onframe_callback = &MainCB_transition_fadeIn;
}

static void MainCB_transition_fadeIn(void) {
	enum progress progress = COMPLETE;
	progress = transition_progress_and(progress, active_transition.transition.fadeIn);

	if (active_transition.target.fadeIn)
		active_transition.target.fadeIn();

	if (COMPLETE == progress)
		scene_onframe_callback = active_transition.target.target;
}
