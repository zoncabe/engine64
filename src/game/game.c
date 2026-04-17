#include "viewport/viewport.h"
#include "time/time.h"
#include "scene/scene.h"
#include "render/render.h"
#include "player/player.h"
#include "control/controller.h"
#include "control/player_control.h"
#include "game/game.h"
#include "game/game_states.h"
#include "sound/sound.h"


static Game game;

static RenderContext render_context;


Game *game_get(void) { return &game; }

GameContext game_getContext(void)
{
	return (GameContext){
		.game     = &game,
		.viewport = viewport_get(),
		.scene    = scene_get(),
		.player   = player_get(),
	};
}

void game_init()
{
	asset_init_compression(2);

	dfs_init(DFS_DEFAULT_LOCATION);

	srand(getentropy32());
	
	rdpq_init();
	
	joypad_init();
	controller_start();
	
	time_init();
	//time_setScale(2.0f);

	viewport_init();
	
	sound_init();

	game.state = GAME_INITIAL_STATE;
	game_loadInitialState();
}

void game_runFrame(void)
{
	time_update();

	GameContext ctx = game_getContext();

	player_setControllerData(ctx.player, ctx.game);

	game_updateState(&ctx);

	GameRenderDescriptor desc = game_getRenderDescriptor(&ctx);
	render_setContext(&render_context, desc.scene, ctx.viewport->fb_index, &ctx.game->transition, desc.screen);
	render(&render_context, &ctx.viewport->fb_index);
}

void game_close()
{
	t3d_destroy();
}
