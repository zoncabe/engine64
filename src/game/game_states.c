#include "resources/resources.h"
#include "time/time.h"
#include "scene/scene.h"
#include "render/render.h"
#include "cutscene/intro.h"
#include "screen/screen.h"
#include "screen/screen_animation.h"
#include "ui/main_menu_ui.h"
#include "ui/pause_ui.h"
#include "ui/gameplay_ui.h"
#include "menu/menu.h"
#include "ui/settings_ui.h"
#include "player/player.h"
#include "control/player_control.h"
#include "game/game.h"
#include "viewport/viewport.h"
#include "physics/world/physics_scene.h"
#include "physics/math/math_common.h"
#include "game/game_states.h"


extern PhysicsScene *physics_getScene(void);


typedef struct {

	void (*update)(GameContext *);
	void (*setDescriptor)(const GameContext *, GameRenderDescriptor *);
	void (*bindActor)(void);
	void (*onEnter)(void);
	SceneID            scene_id;

} GameStateDef;

static void gameState_bindGameplayActor(void)
{
	player_setEntity(&player_get()[0], scene_getActor(0));
}

static void gameState_updateIntro(GameContext *ctx)
{
	screenAnimationPlayer_update(&intro_animation_player, time_get()->delta);
	if (!intro_animation_player.is_active)
		game_setState(ctx->game, GAME_STATE_MAIN_MENU);
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
	for (int i = 0; i < PLAYER_COUNT; i++)
		player_setActorControl(&ctx->player[i], &control[i].actions, ctx->viewport);
	player_update();

	Scene *scene = scene_get();
	float dt = time_get()->delta;

	physics_step(physics_getScene());

	uint8_t fb_index = ctx->viewport->fb_index;
	for (int i = 0; i < scene->entity_count; i++) {
		Entity *entity = scene->entity[i];
		if (!entity->body || (entity->body->flags & BODY_FLAG_STATIC)) continue;
		entity_syncTransformToBody(entity);
		entity_setMatrix(entity, fb_index);
	}

	viewport_updateCamera(&control[0].actions, &ctx->player[0].entity->transform.position);
	gameplay_update();
	(void)game;
}

static void gameState_updatePause(GameContext *ctx)
{
	(void)ctx;
	Scene *scene = scene_get();
	for (int i = 0; i < scene->entity_count; i++) {
		Entity *entity = scene->entity[i];
		if (entity->type != ENTITY_ACTOR) continue;
		for (int fb = 0; fb < FB_COUNT; fb++)
			entity_setMatrix(entity, fb);
	}

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
	descriptor->screen = &gameplay_screen;
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
		.update        = gameState_updateIntro,
		.setDescriptor = gameState_setIntroDescriptor,
		.bindActor     = NULL,
		.onEnter       = intro_init,
		.scene_id      = SCENE_COUNT,
	},

	[GAME_STATE_MAIN_MENU] = {
		.update        = gameState_updateMainMenu,
		.setDescriptor = gameState_setMainMenuDescriptor,
		.bindActor     = NULL,
		.onEnter       = main_menu_startEnter,
		.scene_id      = SCENE_COUNT,
	},

	[GAME_STATE_GAMEPLAY] = {
		.update        = gameState_updateGameplay,
		.setDescriptor = gameState_setGameplayDescriptor,
		.bindActor     = gameState_bindGameplayActor,
		.onEnter       = gameplay_startEnter,
		.scene_id      = SCENE_DEMO,
	},

	[GAME_STATE_PAUSE] = {
		.update        = gameState_updatePause,
		.setDescriptor = gameState_setPauseDescriptor,
		.bindActor     = NULL,
		.onEnter       = pause_startEnter,
		.scene_id      = SCENE_COUNT,
	},

	[GAME_STATE_GAME_OVER] = {
		.update        = gameState_updateGameOver,
		.setDescriptor = gameState_setGameOverDescriptor,
		.bindActor     = NULL,
		.onEnter       = NULL,
		.scene_id      = SCENE_COUNT,
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

static bool gameState_preservesScene(GameState prev, GameState next)
{
	if (prev == GAME_STATE_GAMEPLAY && next == GAME_STATE_PAUSE)    return true;
	if (prev == GAME_STATE_PAUSE    && next == GAME_STATE_GAMEPLAY) return true;
	return false;
}

void game_setState(Game *game, GameState new_state)
{
	if (game->state == new_state) return;

	GameState prev = game->state;
	bool preserves = gameState_preservesScene(prev, new_state);

	if (preserves) {
		// Pause is overlay on top of gameplay: keep gameplay resources loaded.
		// Going INTO pause: load pause resources additively + run pause onEnter.
		// Going OUT of pause: unload pause resources, leave gameplay onEnter alone (no re-fade).
		if (new_state == GAME_STATE_PAUSE) {
			resources_load(resources_forState(GAME_STATE_PAUSE));
			game->state = new_state;
			if (game_state[GAME_STATE_PAUSE].onEnter) game_state[GAME_STATE_PAUSE].onEnter();
		} else {
			resources_unload(resources_forState(GAME_STATE_PAUSE));
			game->state = new_state;
		}
		return;
	}

	rspq_wait();
	// If leaving pause toward a non-preserved state, also tear down the gameplay scene underneath.
	if (prev == GAME_STATE_PAUSE) {
		resources_unload(resources_forState(GAME_STATE_PAUSE));
		gameState_unload(GAME_STATE_GAMEPLAY);
	} else {
		gameState_unload(prev);
	}
	game->state = new_state;
	gameState_load(new_state);
}

void game_loadInitialState(void)
{
	gameState_load(GAME_INITIAL_STATE);
}

void game_updateState(GameContext *ctx)
{
	game_state[ctx->game->state].update(ctx);
}

GameRenderDescriptor game_getRenderDescriptor(const GameContext *ctx)
{
	GameRenderDescriptor descriptor = {0};
	game_state[ctx->game->state].setDescriptor(ctx, &descriptor);
	return descriptor;
}
