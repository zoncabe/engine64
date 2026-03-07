#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>

#include "include/time/time.h"
#include "include/physics/physics.h"

#include "include/control/control.h"

#include "include/sound/sound.h"

#include "include/light/lighting.h"
#include "include/camera/camera.h"
#include "include/viewport/viewport.h"

#include "include/actor/actor.h"

#include "include/player/player.h"
#include "include/control/player_control.h"
#include "include/player/player_animation.h"

#include "include/scene/scenery.h"
#include "include/scene/scene.h"

#include "include/graphics/font.h"
#include "include/ui/ui.h"
#include "include/ui/menu.h"

#include "include/game/game.h"
#include "include/game/game_states.h"

#include "include/render/render.h"

#include "include/memory/memory.h"


int main()
{
	debug_init_isviewer();
	debug_init_usblog();
	
	game_init();
    
	demoScene_init();

	for(;;)
	{

		time_update();
		//timer.delta *= 1.8;

		player_setControllerData();
		
		game_updateState();
		
		//sound_setMixer();
		//sound_play();

		render();

		//sound_play();
	}

	game_close();

	return 0;
}
