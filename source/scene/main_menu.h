#include "management/transition.h"
extern const struct transitionTargetCallbacks transitionTargetCbs_mainMenu;

#include "transition/cut.h"
[[maybe_unused]]
static void MainCB_mainMenu_init(void) {
	StartTransition(
		&transition_cut,
		&(struct transitionSourceCallbacks) {0},
		&transitionTargetCbs_mainMenu);
}
