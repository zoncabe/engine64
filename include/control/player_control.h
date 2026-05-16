#ifndef PLAYER_CONTROLS_H
#define PLAYER_CONTROLS_H

#include "controller.h"

typedef struct Viewport Viewport;
typedef struct Player   Player;
typedef struct Game     Game;

void player_setActorControl(Player *player, const ControllerActions *actions, Viewport *viewport);
void player_setControllerData(Player *player, Game *game);

#endif
