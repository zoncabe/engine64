#ifndef GAMEPLAY_UI_H
#define GAMEPLAY_UI_H

#include <stdbool.h>

#include "screen/screen.h"


extern Screen gameplay_screen;


void gameplay_ui_init(void);
void gameplay_ui_update(void);
void gameplay_ui_startEnter(void);
void gameplay_ui_startExit(void (*on_finish)(void *ctx), void *ctx);

/* The state's onExit: releases what this UI holds from the scene. */
void gameplay_ui_exit(void);
bool gameplay_ui_isTransitioning(void);

#endif
