#include "viewport/e64_viewport.h"
#include "time/e64_time.h"
#include "scene3d/e64_scene3d.h"
#include "render/e64_render.h"
#include "player/e64_player.h"
#include "control/e64_controller.h"
#include "control/e64_player_control.h"
#include "game/e64_game.h"
#include "game/e64_game_states.h"
#include "sound/e64_sound.h"
#include "menu/e64_settings.h"
#include "menu/e64_menu.h"
#include "particles/e64_particles.h"


static Game game;

static RenderContext render_context;


Game *game_get(void) { return &game; }

GameContext game_getContext(void)
{
	return (GameContext){
		.game     = &game,
		.viewport = viewport_get(),
		.scene3d  = scene3d_get(),
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

	viewport_init();

	particles_init();

	player_init();

	settings_init();

	menuStack_init();

	/* The game's own inits and game_start (state table, initial state)
	   run after this, from the game's main. */
}

void game_runStep(void)
{
	time_update();

	GameContext ctx = game_getContext();

	controller_poll();
	player_setControllerData(ctx.game);

	game_updateState(&ctx);

	sound_update();

	GameRenderDescriptor desc = game_getRenderDescriptor(&ctx);
	render_setContext(&render_context, desc.scene3d, ctx.viewport->fb_index, desc.scene2d);
	render(&render_context, &ctx.viewport->fb_index);

	sound_poll();
}

void game_close()
{
	t3d_destroy();
}
