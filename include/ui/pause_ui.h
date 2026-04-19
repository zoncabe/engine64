#ifndef PAUSE_UI_H
#define PAUSE_UI_H

#include <stdbool.h>

#include "screen/screen.h"


extern Screen pause_screen;


void pause_ui_init(void);
void pause_update(void);
void pause_startEnter(void);
void pause_startExit(void (*on_finish)(void *ctx), void *ctx);
bool pause_isTransitioning(void);

#endif
