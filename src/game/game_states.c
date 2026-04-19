#include "resources/resources.h"
#include "time/time.h"
#include "scene/scene.h"
#include "render/render.h"
#include "cutscene/intro.h"
#include "screen/screen.h"
#include "screen/screen_animation.h"
#include "ui/main_menu_ui.h"
#include "ui/pause_ui.h"
#include "menu/menu.h"
#include "ui/settings_ui.h"
#include "player/player.h"
#include "control/player_control.h"
#include "game/game.h"
#include "viewport/viewport.h"
#include "game/game_states.h"


typedef struct {

	void (*update)(GameContext *);
	void (*setDescriptor)(const GameContext *, GameRenderDescriptor *);
	void (*bindActor)(void);
	void (*onEnter)(void);
	GameState          parent;
	GameState          child[GAME_STATE_MAX_CHILD];
	uint8_t            child_count;
	SceneID            scene_id;
	GameTransitionType gameTransition_type;
	float              gameTransition_speed;
	bool               is_overlay;

} GameStateDef;

// workaround until physics happen ////////////////////////////////////////////////////////////////
static void playerCollision_collideWithRoom(Player *player) {
	if (player->entity->transform.position.x > 2450) player->entity->transform.position.x = 2450;
	if (player->entity->transform.position.x < -2450) player->entity->transform.position.x = -2450;
	if (player->entity->transform.position.y > 2450) player->entity->transform.position.y = 2450;
	if (player->entity->transform.position.y < -2450) player->entity->transform.position.y = -2450;
	if (player->entity->transform.position.z < 0) player->entity->transform.position.z = 0;
}//////////////////////////////////////////////////////////////////////////////////////////////////

static void gameState_bindGameplayActor(void)
{
	player_setEntity(&player_get()[0], scene_getActor(0));
}

static void gameState_updateIntro(GameContext *ctx)
{
	if (ctx->game->transition.is_active) return;
	screenAnimationPlayer_update(&intro_animation_player, time_get()->delta);
	if (!intro_animation_player.is_active) {
		game_setState(ctx->game, GAME_STATE_MAIN_MENU);
		ctx->game->transition.progress = 1.0f;
	}
}

static void gameState_updateMainMenu(GameContext *ctx)
{
	(void)ctx;
	if (menuStack_current() != NULL) {
		settings_ui_update();
		return;
	}
	main_menu_update();
}

static void gameState_updateGameplay(GameContext *ctx)
{
	Game *game = ctx->game;

	Controller *control = controller_get();
	for (uint8_t i = 0; i < PLAYER_COUNT; i++)
		player_setActorControl(&ctx->player[i], &control[i].actions, ctx->viewport);
	player_update(ctx->viewport->fb_index);
	playerCollision_collideWithRoom(&ctx->player[0]);
	viewport_updateCamera(&control[0].actions, &ctx->player[0].entity->transform.position);
	(void)game;
}

static void gameState_updatePause(GameContext *ctx)
{
	player_setMatrix(ctx->viewport->fb_index);

	if (menuStack_current() != NULL) {
		settings_ui_update();
		return;
	}

	pause_update();
}

static void gameState_updateGameOver(GameContext *ctx)
{
	(void)ctx;
}

static void gameState_setIntroDescriptor(const GameContext *ctx, GameRenderDescriptor *descriptor)
{
	(void)ctx;
	descriptor->scene  = NULL;
	descriptor->screen = &intro_screen;
}

static void gameState_setMainMenuDescriptor(const GameContext *ctx, GameRenderDescriptor *descriptor)
{
	(void)ctx;
	descriptor->scene  = NULL;
	Screen *top = menuStack_current();
	descriptor->screen = top ? top : &main_menu_screen;
}

static void gameState_setGameplayDescriptor(const GameContext *ctx, GameRenderDescriptor *descriptor)
{
	descriptor->scene  = ctx->scene;
	descriptor->screen = NULL;
}

static void gameState_setPauseDescriptor(const GameContext *ctx, GameRenderDescriptor *descriptor)
{
	descriptor->scene  = ctx->scene;
	Screen *top = menuStack_current();
	descriptor->screen = top ? top : &pause_screen;
}

static void gameState_setGameOverDescriptor(const GameContext *ctx, GameRenderDescriptor *descriptor)
{
	(void)ctx;
	descriptor->scene  = NULL;
	descriptor->screen = NULL;
}

static const GameStateDef game_state[GAME_STATE_COUNT] = {

	[GAME_STATE_INTRO] = {
		.update               = gameState_updateIntro,
		.setDescriptor        = gameState_setIntroDescriptor,
		.bindActor            = NULL,
		.onEnter              = intro_init,
		.parent               = GAME_STATE_COUNT,
		.child_count          = 0,
		.scene_id             = SCENE_COUNT,
		.gameTransition_type  = TRANSITION_FADE,
		.gameTransition_speed = TRANSITION_SPEED_SLOW,
		.is_overlay           = false,
	},

	[GAME_STATE_MAIN_MENU] = {
		.update               = gameState_updateMainMenu,
		.setDescriptor        = gameState_setMainMenuDescriptor,
		.bindActor            = NULL,
		.parent               = GAME_STATE_COUNT,
		.child_count          = 0,
		.scene_id             = SCENE_COUNT,
		.gameTransition_type  = TRANSITION_FADE,
		.gameTransition_speed = TRANSITION_SPEED_SLOW,
		.is_overlay           = false,
	},

	[GAME_STATE_GAMEPLAY] = {
		.update               = gameState_updateGameplay,
		.setDescriptor        = gameState_setGameplayDescriptor,
		.bindActor            = gameState_bindGameplayActor,
		.parent               = GAME_STATE_COUNT,
		.child                = { GAME_STATE_PAUSE },
		.child_count          = 1,
		.scene_id             = SCENE_DEMO,
		.gameTransition_type  = TRANSITION_FADE,
		.gameTransition_speed = TRANSITION_SPEED_SLOW,
		.is_overlay           = false,
	},

	[GAME_STATE_PAUSE] = {
		.update               = gameState_updatePause,
		.setDescriptor        = gameState_setPauseDescriptor,
		.bindActor            = NULL,
		.parent               = GAME_STATE_GAMEPLAY,
		.child_count          = 0,
		.scene_id             = SCENE_COUNT,
		.gameTransition_type  = TRANSITION_FADE,
		.gameTransition_speed = TRANSITION_SPEED_FAST,
		.is_overlay           = true,
	},

	[GAME_STATE_GAME_OVER] = {
		.update               = gameState_updateGameOver,
		.setDescriptor        = gameState_setGameOverDescriptor,
		.bindActor            = NULL,
		.parent               = GAME_STATE_COUNT,
		.child_count          = 0,
		.scene_id             = SCENE_COUNT,
		.gameTransition_type  = TRANSITION_FADE,
		.gameTransition_speed = TRANSITION_SPEED_SLOW,
		.is_overlay           = false,
	},

};

static void gameState_load(GameState id)
{
	resources_load(resources_forState(id));
	if (game_state[id].scene_id != SCENE_COUNT) {
		scene_load(scene_getDef(game_state[id].scene_id));
		if (game_state[id].bindActor) game_state[id].bindActor();
	}
	if (game_state[id].onEnter) game_state[id].onEnter();
}

static void gameState_unload(GameState id)
{
	if (game_state[id].scene_id != SCENE_COUNT) {
		player_init();
		scene_unload();
	}
	resources_unload(resources_forState(id));
}

static void gameTransition_start(Game *game, GameTransitionType type, float speed, bool is_overlay, bool reversed)
{
	game->transition.type       = type;
	game->transition.speed      = speed;
	game->transition.is_active     = true;
	game->transition.is_overlay = is_overlay;
	if (reversed) {
		game->transition.progress = 1.0f;
		game->transition.phase    = TRANSITION_PHASE_FINISH;
		game->state               = game->target_state;
	} else {
		game->transition.progress = 0.0f;
		game->transition.phase    = TRANSITION_PHASE_START;
	}
}

static bool gameTransition_sharesContext(GameState a, GameState b)
{
	return game_state[b].parent == a || game_state[a].parent == b;
}

static void gameTransition_swapContext(Game *game)
{
	GameState prev = game->state;
	GameState next = game->target_state;

	if (gameTransition_sharesContext(prev, next)) return;

	rspq_wait();

	GameState prev_root = (game_state[prev].parent != GAME_STATE_COUNT) ? game_state[prev].parent : prev;

	for (uint8_t i = 0; i < game_state[prev_root].child_count; i++)
		gameState_unload(game_state[prev_root].child[i]);
	gameState_unload(prev_root);

	gameState_load(next);
	for (uint8_t i = 0; i < game_state[next].child_count; i++)
		gameState_load(game_state[next].child[i]);
}

static void gameTransition_update(Game *game)
{
	if (!game->transition.is_active) return;

	if (game->transition.phase == TRANSITION_PHASE_START) {
		game->transition.progress += game->transition.speed * time_get()->delta;
		if (game->transition.progress >= 1.0f) {
			game->transition.progress = 1.0f;
			game->transition.phase = TRANSITION_PHASE_LOAD;
		}
	} else if (game->transition.phase == TRANSITION_PHASE_LOAD) {
		gameTransition_swapContext(game);
		game->state = game->target_state;
		if (game->transition.is_overlay) {
			game->transition.is_active = false;
		} else {
			game->transition.phase = TRANSITION_PHASE_FINISH;
		}
	} else {
		game->transition.progress -= game->transition.speed * time_get()->delta;
		if (game->transition.progress <= 0.0f) {
			game->transition.progress = 0.0f;
			game->transition.is_active = false;
		}
	}
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

void game_loadInitialState(void)
{
	gameState_load(GAME_INITIAL_STATE);
	for (uint8_t i = 0; i < game_state[GAME_INITIAL_STATE].child_count; i++)
		gameState_load(game_state[GAME_INITIAL_STATE].child[i]);
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
