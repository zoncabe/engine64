#ifndef MENU_CONTROL_H
#define MENU_CONTROL_H

#include "controller.h"

typedef struct Player Player;
typedef struct Game   Game;

void menuControl_update(Player *player, const ControllerActions *actions, Game *game);

#endif
