#ifndef SETTINGS_UI_H
#define SETTINGS_UI_H

#include <stdint.h>

#include "screen/screen.h"
#include "control/controller.h"


typedef enum {

	SETTINGS_CTX_FROM_MAIN_MENU,
	SETTINGS_CTX_FROM_PAUSE,
	SETTINGS_CTX_COUNT,

} SettingsContext;


extern Screen settings_screen;


void settings_ui_init(void);
void settings_ui_open(SettingsContext context, void (*on_close)(void));
void settings_ui_setOnClose(void (*on_close)(void));
void settings_ui_startExit(void);
bool settings_ui_isTransitioning(void);
void settings_ui_update(void);
void settings_ui_handleInput(const ControllerActions *actions);

#endif
