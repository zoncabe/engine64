#ifndef GAME_STATES_H
#define GAME_STATES_H

#define GAME_INITIAL_STATE GAME_STATE_MAIN_MENU

#define TRANSITION_SPEED_SLOW  10.0f
#define TRANSITION_SPEED_FAST  20.0f
#define GAME_STATE_MAX_CHILD   4

typedef struct GameContext GameContext;
typedef struct Game        Game;


typedef enum {
	GAME_STATE_INTRO      = 0,
	GAME_STATE_MAIN_MENU  = 1,
	GAME_STATE_GAMEPLAY   = 2,
	GAME_STATE_PAUSE      = 3,
	GAME_STATE_GAME_OVER  = 4,
	GAME_STATE_COUNT
} GameState;


void game_setState(Game *game, GameState new_state);
void game_updateState(GameContext *ctx);
void game_loadInitialState(void);


#endif
