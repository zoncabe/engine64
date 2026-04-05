#ifndef PLAYER_CONTROLS_H
#define PLAYER_CONTROLS_H

#include "controller.h"

typedef struct Viewport Viewport;
typedef struct Player   Player;
typedef struct Game     Game;

#define PLAYER_STICK_DEADZONE       6
#define PLAYER_STICK_WALK_THRESHOLD 65

void player_setActorControl(Player *player, const ControllerActions *actions, Viewport *viewport);
void player_setControllerData(Player **players, Game *game);

#endif
