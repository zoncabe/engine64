#ifndef MAIN_MENU_UI_H
#define MAIN_MENU_UI_H

#include <stdbool.h>

#include "screen/screen.h"


extern Screen main_menu_screen;


void main_menu_ui_init(void);
void main_menu_ui_update(void);
void main_menu_ui_startExit(void (*on_finish)(void *ctx), void *ctx);
void main_menu_ui_startEnter(void);
bool main_menu_ui_isTransitioning(void);

#endif
