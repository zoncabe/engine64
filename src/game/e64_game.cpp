#include "viewport/e64_viewport.h"
#include "time/e64_time.h"
#include "scene3d/e64_scene3d.h"
#include "render/e64_render.h"
#include "player/e64_player.h"
#include "controller/e64_controller.h"
#include "game/e64_game.h"
#include "game/e64_game_states.h"
#include "sound/e64_sound.h"
#include "menu/e64_settings.h"
#include "menu/e64_menu.h"
#include "particles/e64_particles.h"

namespace e64 {

namespace game {

static Game gg;


Game *get(void) { return &gg; }

void init()
{
	asset_init_compression(2);

	dfs_init(DFS_DEFAULT_LOCATION);

	srand(getentropy32());

	rdpq_init();

	joypad_init();

	controller::start();

	time::init();

	viewport::get()->init();

	particles::init();

	player::init();

	settings::init();

	menu::init();

	/* The game's own inits and game::state::start (state table, initial
	   state) run after this, from the game's main. */
}

void runStep(void)
{
	sound::poll();

	time::update();

	controller::poll();

	state::update();

	sound::update();

	render::draw();
}

void close()
{
	t3d_destroy();
}

}

}
