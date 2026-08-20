#ifndef PAUSE_UI_H
#define PAUSE_UI_H

#include <stdbool.h>

#include "screen/screen.h"


extern Screen pause_screen;


void pause_ui_init(void);
void pause_ui_update(void);
void pause_ui_startEnter(void);
void pause_ui_startExit(void (*on_finish)(void *ctx), void *ctx);
void pause_ui_startQuit(void (*on_finish)(void *ctx), void *ctx);
bool pause_ui_isTransitioning(void);

#endif
