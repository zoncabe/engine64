#ifndef PLAYER_CONTROLS_H
#define PLAYER_CONTROLS_H

typedef struct Viewport  Viewport;
typedef struct Player    Player;
typedef struct Game      Game;
#include "../game/game_states.h"


#define PLAYER_STICK_DEADZONE       6
#define PLAYER_STICK_WALK_THRESHOLD 65


// function prototypes


void player_setActorControl(Player *player, Viewport *viewport);
void player_setControllerData(Player **players, Game *game);

#endif