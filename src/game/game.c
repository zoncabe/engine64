#include <stdint.h>
#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>

#include "../../include/physics/physics.h"
#include "../../include/control/control.h"
#include "../../include/actor/actor.h"
#include "../../include/light/lighting.h"
#include "../../include/camera/camera.h"
#include "../../include/viewport/viewport.h"
#include "../../include/time/time.h"
#include "../../include/scene/scenery.h"
#include "../../include/scene/scene.h"
#include "../../include/graphics/font.h"
#include "../../include/graphics/sprites.h"
#include "../../include/graphics/shapes.h"
#include "../../include/render/render.h"
#include "../../include/ui/ui.h"
#include "../../include/ui/menu.h"
#include "../../include/graphics/sprites.h"
#include "../../include/cutscene/intro.h"
#include "../../include/player/player.h"
#include "../../include/control/player_control.h"
#include "../../include/game/game.h"
#include "../../include/game/game_states.h"
#include "../../include/memory/memory.h"
#include "../../include/sound/sound.h"


static Game game;

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
	
	time_init();
	
	viewport_init();
	
	ui_init();

	sound_init();

	intro_init();

	menu_init();
	
	game.state = MAIN_MENU;
}

void game_close()
{
    t3d_destroy();
}


static RenderContext render_context;

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
