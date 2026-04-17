#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

#include "game_states.h"
#include "control/controller.h"
#include "player/player.h"


typedef struct Scene         Scene;
typedef struct RenderContext RenderContext;
typedef struct Viewport      Viewport;


typedef enum {

	TRANSITION_FADE,

} GameTransitionType;

typedef enum {

	TRANSITION_PHASE_START,
	TRANSITION_PHASE_LOAD,
	TRANSITION_PHASE_FINISH,

} GameTransitionPhase;

typedef struct GameTransition {

	GameTransitionType  type;
	GameTransitionPhase phase;
	float               progress;
	float               speed;
	bool                is_active;
	bool                is_overlay;

} GameTransition;

typedef struct Game {

	GameState  state;
	GameState  previous_state;
	GameState  target_state;
	bool       state_changed;
	GameTransition transition;

} Game;


typedef struct GameContext {

	Game          *game;
	Viewport      *viewport;
	Scene         *scene;
	Player        *player;
	Controller   **controller;

} GameContext;

typedef struct GameRenderDescriptor {

	const Scene *scene;
	const RenderContext *screen;
	
} GameRenderDescriptor;

Game *game_get(void);
GameContext game_getContext(void);
GameRenderDescriptor game_getRenderDescriptor(const GameContext *ctx);


void game_init(void);
void game_runFrame(void);
void game_close(void);


#endif
