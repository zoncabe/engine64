#ifndef MENU_CONTROL_H
#define MENU_CONTROL_H

#include "controller.h"

typedef struct Player Player;
typedef struct Game   Game;


typedef struct MenuControlBinding {

	ButtonID confirm;
	ButtonID cancel;
	ButtonID pause;
	ButtonID up;
	ButtonID down;
	ButtonID left;
	ButtonID right;
	ButtonID tab_left;
	ButtonID tab_right;

} MenuControlBinding;


typedef struct MenuControls {

	bool confirm;
	bool cancel;
	bool pause;
	bool up;
	bool down;
	bool up_held;
	bool down_held;
	bool left;
	bool right;
	bool tab_left;
	bool tab_right;

} MenuControls;


void menuControls_map(MenuControls *controls, const Controller *pad, const MenuControlBinding *binding);
void menuControl_update(Player *player, Game *game);

#endif
