#include <stdbool.h>

#ifndef GAME_H
#define GAME_H

#include "game_states.h"
#include "../control/controller.h"

typedef struct Scene         Scene;
typedef struct RenderContext RenderContext;
typedef struct Viewport      Viewport;
typedef struct Player        Player;

typedef enum {

    TRANSITION_FADE,

} GameTransitionType;

typedef struct GameTransition {

    GameTransitionType type;
    float          progress;
    float          speed;
    bool           active;
    uint8_t        phase;
    bool           is_overlay;

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
    Player       **player;
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
