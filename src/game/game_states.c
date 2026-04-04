
#include "../../include/time/time.h"
#include "../../include/scene/scene.h"
#include "../../include/scene/scenery.h"
#include "../../include/render/render.h"
#include "../../include/cutscene/intro.h"
#include "../../include/ui/ui.h"
#include "../../include/player/player.h"
#include "../../include/control/player_control.h"
#include "../../include/game/game.h"
#include "../../include/viewport/viewport.h"
#include "../../include/game/game_states.h"


typedef struct {

    void (*update)(GameContext *);
    void (*setDescriptor)(const GameContext *, GameRenderDescriptor *);
    GameTransitionType gameTransition_type;
    float gameTransition_speed;
    bool is_overlay;

} GameStateDef;

void gameState_updateIntro(GameContext *ctx);
void gameState_updateMainMenu(GameContext *ctx);
void gameState_updateGameplay(GameContext *ctx);
void gameState_updatePause(GameContext *ctx);
void gameState_updateGameOver(GameContext *ctx);

void gameState_setIntroDescriptor(const GameContext *ctx, GameRenderDescriptor *descriptor);
void gameState_setMainMenuDescriptor(const GameContext *ctx, GameRenderDescriptor *descriptor);
void gameState_setGameplayDescriptor(const GameContext *ctx, GameRenderDescriptor *descriptor);
void gameState_setPauseDescriptor(const GameContext *ctx, GameRenderDescriptor *descriptor);
void gameState_setGameOverDescriptor(const GameContext *ctx, GameRenderDescriptor *descriptor);


static const GameStateDef game_state[GAME_STATE_COUNT] = {

    [INTRO]     = { gameState_updateIntro,    gameState_setIntroDescriptor,    TRANSITION_FADE, TRANSITION_SPEED_SLOW, false },
    [MAIN_MENU] = { gameState_updateMainMenu, gameState_setMainMenuDescriptor, TRANSITION_FADE, TRANSITION_SPEED_SLOW, false },
    [GAMEPLAY]  = { gameState_updateGameplay, gameState_setGameplayDescriptor, TRANSITION_FADE, TRANSITION_SPEED_SLOW, false },
    [PAUSE]     = { gameState_updatePause,    gameState_setPauseDescriptor,    TRANSITION_FADE, TRANSITION_SPEED_FAST, true  },
    [GAME_OVER] = { gameState_updateGameOver, gameState_setGameOverDescriptor, TRANSITION_FADE, TRANSITION_SPEED_SLOW, false },

};

static void gameTransition_start(Game *game, GameTransitionType type, float speed, bool is_overlay, bool reversed)
{
    game->transition.type       = type;
    game->transition.speed      = speed;
    game->transition.active     = true;
    game->transition.is_overlay = is_overlay;
    if (reversed) {
        game->transition.progress = 1.0f;
        game->transition.phase    = 1;
        game->state               = game->target_state;
    } else {
        game->transition.progress = 0.0f;
        game->transition.phase    = 0;
    }
}


static void gameTransition_update(Game *game)
{
    if (!game->transition.active) return;
    if (game->transition.phase == 0) {
        game->transition.progress += game->transition.speed * time_get()->delta;
        if (game->transition.progress >= 1.0f) {
            game->transition.progress = 1.0f;
            game->state = game->target_state;
            if (game->transition.is_overlay) {
                game->transition.active = false;
            } else {
                game->transition.phase = 1;
            }
        }
    } else {
        game->transition.progress -= game->transition.speed * time_get()->delta;
        if (game->transition.progress <= 0.0f) {
            game->transition.progress = 0.0f;
            game->transition.active = false;
        }
    }
}


static void playerCollision_collideWithRoom(Player *player) {
    if (player->entity->transform.position.x > 2450) player->entity->transform.position.x = 2450;
    if (player->entity->transform.position.x < -2450) player->entity->transform.position.x = -2450;
    if (player->entity->transform.position.y > 2450) player->entity->transform.position.y = 2450;
    if (player->entity->transform.position.y < -2450) player->entity->transform.position.y = -2450;
    if (player->entity->transform.position.z < 0) player->entity->transform.position.z = 0;
}


void gameState_updateIntro(GameContext *ctx)
{
    Game *game = ctx->game;

    game->intro_counter += time_get()->delta;
    intro_animate(game->intro_counter);

    if (game->intro_counter >= INTRO_SCENE_LENGTH) {
        game_setState(game, MAIN_MENU);
        game->intro_counter = 0.0f;
        game->transition.progress = 1.0f;
    }
}


void gameState_updateMainMenu(GameContext *ctx)
{
    (void)ctx;
    mainMenu_animate();
}


void gameState_updateGameplay(GameContext *ctx)
{
    Game *game = ctx->game;

    bool show_pause = (game->previous_state == PAUSE || game->target_state == PAUSE)
                      && game->transition.progress > 0.0f;

    for (uint8_t i = 0; i < PLAYER_COUNT; i++)
        player_setActorControl(ctx->player[i], ctx->viewport);
    player_update(ctx->viewport->fb_index);
    playerCollision_collideWithRoom(ctx->player[0]);
    viewport_setOrbitalCamera(&ctx->player[0]->control, &ctx->player[0]->entity->transform.position);

    if (show_pause) pause_animate(game);
}


void gameState_updatePause(GameContext *ctx)
{
    Game *game       = ctx->game;
    uint8_t fb_index = ctx->viewport->fb_index;

    for (uint8_t i = 0; i < PLAYER_COUNT; i++)
        renderMesh_buildMatrix(ctx->player[i]->entity->mesh, &ctx->player[i]->entity->transform, fb_index);

    if (!game->transition.active || game->transition.is_overlay)
        pause_animate(game);
}


void gameState_updateGameOver(GameContext *ctx)
{
    (void)ctx;
}


void gameState_setIntroDescriptor(const GameContext *ctx, GameRenderDescriptor *descriptor)
{
    (void)ctx;
    descriptor->scene  = NULL;
    descriptor->screen = intro_getRenderContext();
}

void gameState_setMainMenuDescriptor(const GameContext *ctx, GameRenderDescriptor *descriptor)
{
    (void)ctx;
    descriptor->scene  = NULL;
    descriptor->screen = mainMenu_getRenderContext();
}

void gameState_setGameplayDescriptor(const GameContext *ctx, GameRenderDescriptor *descriptor)
{
    const Game *game = ctx->game;

    bool show_pause = (game->previous_state == PAUSE || game->target_state == PAUSE)
                      && game->transition.progress > 0.0f;

    descriptor->scene  = ctx->scene;
    descriptor->screen = show_pause ? pause_getRenderContext() : NULL;
}

void gameState_setPauseDescriptor(const GameContext *ctx, GameRenderDescriptor *descriptor)
{
    descriptor->scene  = ctx->scene;
    descriptor->screen = pause_getRenderContext();
}

void gameState_setGameOverDescriptor(const GameContext *ctx, GameRenderDescriptor *descriptor)
{
    (void)ctx;
    descriptor->scene  = NULL;
    descriptor->screen = NULL;
}


void game_setState(Game *game, GameState new_state)
{
    if (game->state == new_state) return;
    game->target_state  = new_state;
    game->state_changed = true;
}

static void gameTransition_setStateChange(Game *game)
{
    if (!game->state_changed) return;
    game->state_changed = false;

    const GameStateDef *def = &game_state[game->target_state];
    bool is_overlay = game_state[game->state].is_overlay;
    bool reverse = is_overlay && (game->previous_state == game->target_state);

    game->previous_state = game->state;

    gameTransition_start(game, def->gameTransition_type, def->gameTransition_speed, reverse ? is_overlay : def->is_overlay, reverse);
}

void game_updateState(GameContext *ctx)
{
    gameTransition_update(ctx->game);
    gameTransition_setStateChange(ctx->game);
    game_state[ctx->game->state].update(ctx);
}

GameRenderDescriptor game_getRenderDescriptor(const GameContext *ctx)
{
    GameRenderDescriptor descriptor = {0};
    game_state[ctx->game->state].setDescriptor(ctx, &descriptor);
    return descriptor;
}
