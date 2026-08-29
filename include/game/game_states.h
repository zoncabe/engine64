#ifndef GAME_STATES_H
#define GAME_STATES_H

#include <stdint.h>
#include "resources/resources.h"


typedef struct Game                 Game;
typedef struct GameContext          GameContext;
typedef struct GameRenderDescriptor GameRenderDescriptor;
typedef struct Scene3DDef           Scene3DDef;
typedef struct Scene2DDef           Scene2DDef;
typedef struct Player               Player;

/* Index into the state table the game hands to game_start. */
typedef uint8_t GameState;

/* No state: the overlay_of sentinel. A field left out of a designated
   initializer is 0, which is a valid state, so every table entry must set
   overlay_of explicitly. */
#define GAME_STATE_NONE 0xFF


typedef struct GameStateDef {

	void (*update)(GameContext *);
	void (*bindCharacter)(void);
	void (*onEnter)(void);
	void (*onExit)(void);

	/* Holds the switch back while it answers false, so a state that plays
	   its way out is seen through. NULL leaves the moment it is asked. */
	bool (*canLeave)(void);

	/* Per-state input handling (menus, pause); NULL for none. The pad is
	   already polled: the game reads it with controller_get. */
	void (*control)(Player *, Game *);

	/* Loaded on enter, freed on exit. An overlay rides its base's set. */
	ResourceSet resources;

	/* The scenes this state runs on, either or both: the 3D world and the
	   2D one drawn over it. */
	Scene3DDef          *scene3d;
	const Scene2DDef    *scene2d;

	/* The state this one rides on top of; GAME_STATE_NONE for none.
	   Switching between an overlay and its base leaves the base untouched. */
	GameState          overlay_of;

} GameStateDef;


/* Hands the engine the game's state table and loads the initial state.
   Runs after game_init and the game's own inits, before the first runStep. */
void game_start(const GameStateDef *states, uint8_t count, GameState initial);

const GameStateDef *gameState_get(GameState id);

void game_setState(Game *game, GameState new_state);
void game_updateState(GameContext *ctx);


#endif
