#ifndef GAME_STATES_H
#define GAME_STATES_H

#define GAME_INITIAL_STATE GAME_STATE_MAIN_MENU

typedef struct GameContext GameContext;
typedef struct Game Game;


typedef enum {

	GAME_STATE_INTRO,
	GAME_STATE_MAIN_MENU,
	GAME_STATE_GAMEPLAY,
	GAME_STATE_PAUSE,
	GAME_STATE_GAME_OVER,
	GAME_STATE_COUNT
	
} GameState;


void game_setState(Game *game, GameState new_state);
void game_updateState(GameContext *ctx);
void game_loadInitialState(void);


#endif
