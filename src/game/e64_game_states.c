/*
	State machinery only: the table itself is the game's, handed over at
	game_start. The engine loads, unloads and switches whatever it is given.
*/
#include <assert.h>
#include <libdragon.h>

#include "resources/e64_resources.h"
#include "time/e64_time.h"
#include "scene3d/e64_scene3d.h"
#include "scene2d/e64_scene2d.h"
#include "player/e64_player.h"
#include "game/e64_game.h"
#include "game/e64_game_states.h"


static const GameStateDef *game_state;
static uint8_t             game_state_count;


const GameStateDef *gameState_get(GameState id)
{
	assert(id < game_state_count);
	return &game_state[id];
}

static void gameState_load(GameState id)
{
	resources_load(&game_state[id].resources);
	if (game_state[id].scene3d) {
		scene3d_load(game_state[id].scene3d);
		if (game_state[id].bindCharacter) game_state[id].bindCharacter();
	}
	if (game_state[id].scene2d) scene2d_load(game_state[id].scene2d);
	if (game_state[id].onEnter) game_state[id].onEnter();
}

static void gameState_unload(GameState id)
{
	if (game_state[id].onExit) game_state[id].onExit();
	if (game_state[id].scene2d) scene2d_unload();
	if (game_state[id].scene3d) {
		player_init();
		scene3d_unload();
	}
	resources_unload(&game_state[id].resources);
}

static bool gameState_isOverlayPair(GameState prev, GameState next)
{
	return game_state[next].overlay_of == prev || game_state[prev].overlay_of == next;
}

/* Asking to leave is not leaving: the state names where it goes, and the
   switch happens as soon as it lets go. */
void game_setState(Game *game, GameState new_state)
{
	assert(new_state < game_state_count);
	game->next = new_state;
}

static void gameState_settle(Game *game)
{
	if (game->next == game->state) return;

	const GameStateDef *leaving = &game_state[game->state];
	if (leaving->canLeave && !leaving->canLeave()) return;

	GameState prev      = game->state;
	GameState new_state = game->next;

	/* An overlay rides its base: the 3D world stays untouched and dropping
	   back does not re-enter the base. Only the 2D scene changes hands, and
	   it comes back the way its definition declares it. */
	if (gameState_isOverlayPair(prev, new_state)) {
		game->state = new_state;
		if (game_state[new_state].scene2d) scene2d_load(game_state[new_state].scene2d);
		if (game_state[new_state].overlay_of == prev && game_state[new_state].onEnter)
			game_state[new_state].onEnter();
		return;
	}

	rspq_wait();
	gameState_unload(prev);
	/* An abandoned overlay takes its base state down with it. */
	if (game_state[prev].overlay_of != GAME_STATE_NONE)
		gameState_unload(game_state[prev].overlay_of);
	game->state = new_state;
	gameState_load(new_state);

	/* The load blocked for as long as it took: none of it counts as a
	   played frame, or the enter animations would swallow it as one. */
	time_reset();
}

void game_start(const GameStateDef *states, uint8_t count, GameState initial)
{
	assert(states && initial < count);

	game_state       = states;
	game_state_count = count;

	Game *game = game_get();
	game->state = initial;
	game->next  = initial;

	gameState_load(initial);
	time_reset();
}

void game_updateState(GameContext *ctx)
{
	game_state[ctx->game->state].update(ctx);
	gameState_settle(ctx->game);
}

/* What the state draws: its own scenes. An overlay draws the 3D world of
   the state it rides on, which is the one still loaded. */
GameRenderDescriptor game_getRenderDescriptor(const GameContext *ctx)
{
	const GameStateDef *def = &game_state[ctx->game->state];

	bool on_3d = def->scene3d || def->overlay_of != GAME_STATE_NONE;

	return (GameRenderDescriptor){
		.scene3d = on_3d ? ctx->scene3d : NULL,
		.scene2d = def->scene2d ? scene2d_get() : NULL,
	};
}
