#ifndef GAMEPLAY_UI_H
#define GAMEPLAY_UI_H

#include <stdbool.h>

#include "screen/screen.h"


extern Screen gameplay_screen;


void gameplay_ui_init(void);
void gameplay_update(void);
void gameplay_startEnter(void);
void gameplay_startExit(void (*on_finish)(void *ctx), void *ctx);
bool gameplay_isTransitioning(void);

#endif
