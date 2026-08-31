#ifndef ENGINE64_GAME_H
#define ENGINE64_GAME_H

#include <stdbool.h>

#include "e64_game_states.h"
#include "control/e64_controller.h"
#include "player/e64_player.h"


typedef struct Scene3D         Scene3D;
typedef struct RenderContext RenderContext;
typedef struct Scene2D        Scene2D;
typedef struct Viewport      Viewport;


typedef struct Game {

	GameState state;

	/* Where it is heading. A state that asks to leave sets this and plays
	   its way out; the switch happens once the screen is done. Equal to
	   state while there is nowhere to go. */
	GameState next;

} Game;


typedef struct GameContext {

	Game          *game;
	Viewport      *viewport;
	Scene3D         *scene3d;
	Player        *player;
	Controller   **controller;

} GameContext;

typedef struct GameRenderDescriptor {

	const Scene3D *scene3d;
	const Scene2D *scene2d;

} GameRenderDescriptor;

Game *game_get(void);
GameContext game_getContext(void);
GameRenderDescriptor game_getRenderDescriptor(const GameContext *ctx);


void game_init(void);
void game_runStep(void);
void game_close(void);


#endif
