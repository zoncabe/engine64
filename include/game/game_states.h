#ifndef GAME_STATES_H
#define GAME_STATES_H

#define GAME_INITIAL_STATE         MAIN_MENU

#define TRANSITION_SPEED_SLOW      10.0f
#define TRANSITION_SPEED_FAST      20.0f
#define GAME_STATE_MAX_CHILD    4

typedef enum {
    INTRO      = 0,
    MAIN_MENU  = 1,
    GAMEPLAY   = 2,
    PAUSE      = 3,
    GAME_OVER  = 4,
    GAME_STATE_COUNT
} GameState;


// function prototypes

typedef struct GameContext GameContext;
typedef struct Game        Game;

void game_setState(Game *game, GameState new_state);
void game_updateState(GameContext *ctx);
void game_loadInitialState(void);


#endif
