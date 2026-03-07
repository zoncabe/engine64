#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>

#include "../../include/time/time.h"
#include "../../include/physics/physics.h"
#include "../../include/control/control.h"
#include "../../include/light/lighting.h"
#include "../../include/camera/camera.h"
#include "../../include/viewport/viewport.h"
#include "../../include/actor/actor.h"
#include "../../include/player/player.h"
#include "../../include/control/player_control.h"
#include "../../include/player/player_animation.h"
#include "../../include/scene/scenery.h"
#include "../../include/ui/ui.h"
#include "../../include/ui/menu.h"
#include "../../include/game/game.h"
#include "../../include/game/game_states.h"
#include "../../include/render/render.h"
#include "../../include/assets/male_muscled.h"
#include "../../include/memory/memory.h"
#include "../../include/graphics/color.h"



void demoScene_init()
{	
	light_initAmbient((color_t){60, 60, 60, 0xFF});
	light_initDirectional(&light.directional[0], (T3DVec3){{1.0f, -1.0f, 1.0f}}, (color_t){210, 210, 210, 0xFF});
	
	//light_initPoint(&light.point[0], (T3DVec3){{0.0f, 1300.0f, 655.0f}}, (color_t){255, 255, 255, 0xFF}, 1000.0f);
	//light_initPoint(&light.point[1], (T3DVec3){{0.0f, -1300.0f, 655.0f}}, (color_t){255, 255, 255, 0xFF}, 1000.0f);

    player[0] = player_create(male_muscled_model, &male_muscled_motion_settings, &male_muscled_animation_settings);
	player[0]->actor.body.position =(Vector3){-210.0f, -210.0f, 0.0f};
	player[0]->actor.body.rotation.z = 200;
	
	player[1] = player_create(gorilla_model, &male_muscled_motion_settings, &male_muscled_animation_settings);
	player[1]->actor.body.position =(Vector3){210.0f, 500.0f, 0.0f};
	player[1]->actor.body.rotation.z = 20;
	

	scenery[0] = scenery_create("rom:/models/room.t3dm");
	scenery[0]->position.z = -2;
	scenery_update(scenery[0]);

	scenery[1] = scenery_create("rom:/models/axis.t3dm");
	scenery[1]->position.z = -2;
	scenery[1]->scale = (Vector3){1.5f, 1.5f, 1.5f};
	scenery_update(scenery[1]);

}