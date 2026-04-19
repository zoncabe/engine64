#include <libdragon.h>

#include "screen/screen.h"
#include "menu/menu.h"


static MenuStack menuStack;


int8_t        menuStack_getIndex(void)    { return menuStack.index; }
const int8_t *menuStack_getIndexPtr(void) { return &menuStack.index; }
void          menuStack_setIndex(int8_t index) { menuStack.index = index; }

int8_t menuStack_getItemCount(void)
{
	if (menuStack.top == 0) return 0;
	return menuStack.frame[menuStack.top - 1].item_count;
}

void menuStack_moveIndex(int8_t delta, int8_t max)
{
	menuStack.index += delta;
	if (menuStack.index < 0)   menuStack.index = max;
	if (menuStack.index > max) menuStack.index = 0;
}

void menuStack_init(void)
{
	menuStack.index = 0;
	menuStack.top   = 0;
}

void menuStack_open(Screen *screen, int8_t item_count)
{
	if (menuStack.top >= MENU_STACK_MAX) return;

	if (menuStack.top > 0)
		menuStack.frame[menuStack.top - 1].index = menuStack.index;

	menuStack.frame[menuStack.top++] = (MenuStackFrame){
		.screen     = screen,
		.index      = 0,
		.item_count = item_count,
	};
	menuStack.index = 0;
}

void menuStack_back(void)
{
	if (menuStack.top == 0) return;
	menuStack.top--;
	if (menuStack.top > 0)
		menuStack.index = menuStack.frame[menuStack.top - 1].index;
	else
		menuStack.index = 0;
}

Screen *menuStack_current(void)
{
	if (menuStack.top == 0) return NULL;
	return menuStack.frame[menuStack.top - 1].screen;
}
