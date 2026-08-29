#ifndef PLAYER_CONTROLS_H
#define PLAYER_CONTROLS_H

#include "character_control.h"

typedef struct Viewport Viewport;
typedef struct Player   Player;
typedef struct Game     Game;

void player_setCharacterControl(Player *player, const CharacterControls *controls, Viewport *viewport);
void player_setControllerData(Player *player, Game *game);

#endif
