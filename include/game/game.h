#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

#include "game_states.h"
#include "control/controller.h"
#include "player/player.h"


typedef struct Scene         Scene;
typedef struct RenderContext RenderContext;
typedef struct Screen        Screen;
typedef struct Viewport      Viewport;


typedef struct Game {

	GameState state;

} Game;


typedef struct GameContext {

	Game          *game;
	Viewport      *viewport;
	Scene         *scene;
	Player        *player;
	Controller   **controller;

} GameContext;

typedef struct GameRenderDescriptor {

	const Scene  *scene;
	const Screen *screen;

} GameRenderDescriptor;

Game *game_get(void);
GameContext game_getContext(void);
GameRenderDescriptor game_getRenderDescriptor(const GameContext *ctx);


void game_init(void);
void game_runStep(void);
void game_close(void);


#endif
