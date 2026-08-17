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
#include "ui/credits_ui.h"
#include "menu/menu.h"
#include "particles/particles.h"
#include "player/player.h"
#include "control/player_control.h"
#include "game/game.h"
#include "viewport/viewport.h"
#include "game/game_states.h"


typedef struct {

	void (*update)(GameContext *);
	void (*setDescriptor)(const GameContext *, GameRenderDescriptor *);
	void (*bindCharacter)(void);
	void (*onEnter)(void);
	SceneID            scene_id;

} GameStateDef;

static void gameState_bindGameplayCharacter(void)
{
	player_setCharacter(&player_get()[0], scene_getCharacter(0));
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
	main_menu_update();
}

static void gameState_updateCredits(GameContext *ctx)
{
	(void)ctx;
	credits_update();
}

static void gameState_updateGameplay(GameContext *ctx)
{
	Game *game = ctx->game;

	Controller *control = controller_get();
	for (int i = 0; i < PLAYER_COUNT; i++)
		player_setCharacterControl(&ctx->player[i], &control[i].actions, ctx->viewport);
	player_update();

	Scene *scene = scene_get();
	uint8_t fb_index = ctx->viewport->fb_index;

	physics_update(scene_getPhysics(), time_get()->delta);

	for (int i = 0; i < scene->character_count; i++) {
		Character *character = scene->character[i];
		characterPhysics_collide(character, scene_getPhysics());
		characterPhysics_syncBody(character);
		entity_setTransform(character->entity, &character->body);
		entity_setMatrix(character->entity, fb_index);
	}

	/* Simulated props are placed by the solver, so their matrix comes from the
	   body. Static ones keep the one scene_load wrote. */
	for (int i = 0; i < scene->entity_count; i++)
		entity_setMatrixFromBody(scene->entity[i], fb_index);

	particles_update(ctx, fb_index);

	viewport_updateCamera(&control[0].actions, &ctx->player[0].entity->transform.position);
	gameplay_update();
	(void)game;
}

static void gameState_updatePause(GameContext *ctx)
{
	(void)ctx;
	Scene *scene = scene_get();
	/* Matrices are per framebuffer and gameplay only writes the current one, so
	   the three hold three different instants. Frozen, that reads as a shake:
	   everything that moves has to fill all three. */
	for (int i = 0; i < scene->entity_count; i++) {
		Entity *entity = scene->entity[i];

		for (int fb = 0; fb < FB_COUNT; fb++) {
			if (entity->type == ENTITY_CHARACTER) entity_setMatrix(entity, fb);
			entity_setMatrixFromBody(entity, fb);
		}
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
	descriptor->screen = &main_menu_screen;
}

static void gameState_setCreditsDescriptor(const GameContext *ctx, GameRenderDescriptor *descriptor)
{
	(void)ctx;
	descriptor->scene  = NULL;
	descriptor->screen = &credits_screen;
}

static void gameState_setGameplayDescriptor(const GameContext *ctx, GameRenderDescriptor *descriptor)
{
	descriptor->scene  = ctx->scene;
	descriptor->screen = &gameplay_screen;
}

static void gameState_setPauseDescriptor(const GameContext *ctx, GameRenderDescriptor *descriptor)
{
	descriptor->scene  = ctx->scene;
	descriptor->screen = &pause_screen;
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
		.bindCharacter     = NULL,
		.onEnter       = intro_init,
		.scene_id      = SCENE_COUNT,
	},

	[GAME_STATE_MAIN_MENU] = {
		.update        = gameState_updateMainMenu,
		.setDescriptor = gameState_setMainMenuDescriptor,
		.bindCharacter     = NULL,
		.onEnter       = main_menu_startEnter,
		.scene_id      = SCENE_COUNT,
	},

	[GAME_STATE_CREDITS] = {
		.update        = gameState_updateCredits,
		.setDescriptor = gameState_setCreditsDescriptor,
		.bindCharacter     = NULL,
		.onEnter       = credits_startEnter,
		.scene_id      = SCENE_COUNT,
	},

	[GAME_STATE_GAMEPLAY] = {
		.update        = gameState_updateGameplay,
		.setDescriptor = gameState_setGameplayDescriptor,
		.bindCharacter     = gameState_bindGameplayCharacter,
		.onEnter       = gameplay_startEnter,
		.scene_id      = SCENE_DEMO,
	},

	[GAME_STATE_PAUSE] = {
		.update        = gameState_updatePause,
		.setDescriptor = gameState_setPauseDescriptor,
		.bindCharacter     = NULL,
		.onEnter       = pause_startEnter,
		.scene_id      = SCENE_COUNT,
	},

	[GAME_STATE_GAME_OVER] = {
		.update        = gameState_updateGameOver,
		.setDescriptor = gameState_setGameOverDescriptor,
		.bindCharacter     = NULL,
		.onEnter       = NULL,
		.scene_id      = SCENE_COUNT,
	},

};

static void gameState_load(GameState id)
{
	resources_load(resources_forState(id));
	if (game_state[id].scene_id != SCENE_COUNT) {
		scene_load(scene_getDef(game_state[id].scene_id));
		if (game_state[id].bindCharacter) game_state[id].bindCharacter();
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
		/* Pause overlays gameplay, so its resources stay loaded. Entering
		   adds the pause ones, leaving drops them without re-entering
		   gameplay, which would fade in again. */
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
	/* Leaving pause for a state that does not preserve the scene also tears
	   down the gameplay scene underneath. */
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
