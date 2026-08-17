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
#include "menu/settings.h"
#include "menu/menu.h"
#include "ui/main_menu_ui.h"
#include "ui/pause_ui.h"
#include "ui/gameplay_ui.h"
#include "particles/particles.h"


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

	viewport_init();

	particles_init();

	player_init();

	sound_init();

	settings_init();

	menuStack_init();
	main_menu_ui_init();
	pause_ui_init();
	gameplay_ui_init();

	game.state = GAME_INITIAL_STATE;
	game_loadInitialState();
}

void game_runStep(void)
{
	time_update();

	GameContext ctx = game_getContext();

	player_setControllerData(ctx.player, ctx.game);

	game_updateState(&ctx);

	Vector3 ear = ctx.player[0].entity ? ctx.player[0].entity->transform.position
	                                   : ctx.viewport->camera.position;
	Vector3 right = camera_getRight(&ctx.viewport->camera);
	sound_setListener(&ear, &right);
	sound_update();

	GameRenderDescriptor desc = game_getRenderDescriptor(&ctx);
	render_setContext(&render_context, desc.scene, ctx.viewport->fb_index, desc.screen);
	render(&render_context, &ctx.viewport->fb_index);

	sound_poll();
}

void game_close()
{
	t3d_destroy();
}
