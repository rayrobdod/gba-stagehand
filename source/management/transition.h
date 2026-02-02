#ifndef TRANSITION_H
#define TRANSITION_H

#include "main.h"
#include "gba/palette.h"

enum progress {
	ONGOING,
	COMPLETE,
};

struct transition {
	void (*initFadeOut)(void);
	enum progress (*fadeOut)(void);
	void (*initFadeIn)(union palette512);
	enum progress (*fadeIn)(void);
};

struct transitionTargetCallbacks {
	void (*initFadeOut)(void);
	enum progress (*fadeOut)(void);
	union palette512 (*initFadeIn)(void);
	void (*fadeIn)(void);
	MainCallback target;
};

struct transitionSourceCallbacks {
	void (*fadeOut)(void);
};

void StartTransition(
	const struct transition*,
	const struct transitionSourceCallbacks*,
	const struct transitionTargetCallbacks*);

#endif        //  #ifndef TRANSITION_H
