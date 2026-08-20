#ifndef CREDITS_UI_H
#define CREDITS_UI_H

#include <stdbool.h>

#include "screen/screen.h"


extern Screen credits_screen;


void credits_ui_update(void);
void credits_ui_setScrollVelocity(float velocity);
void credits_ui_startEnter(void);
void credits_ui_startExit(void (*on_finish)(void *ctx), void *ctx);
bool credits_ui_isTransitioning(void);

#endif
