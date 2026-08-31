#ifndef ENGINE64_PLAYER_CONTROL_H
#define ENGINE64_PLAYER_CONTROL_H

#include "e64_character_control.h"

typedef struct Viewport Viewport;
typedef struct Player   Player;
typedef struct Game     Game;

/* Reads the buttons this player was seated with and turns them into its
   command for this frame, aimed by the camera. */
void player_setCharacterControl(PlayerID id, Viewport *viewport);
void player_setControllerData(Game *game);

#endif
