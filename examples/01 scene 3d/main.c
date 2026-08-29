/*
	The smallest world engine64 can put on screen: a room, a capsule standing in
	the middle of it, and a camera to look around.

	Everything here is content: the assets, where they are placed, and the one
	game state that draws them. The engine supplies the rest.
*/
#include <libdragon.h>

#include "game/game.h"
#include "scene3d/scene3d.h"
#include "entity/entity.h"
#include "viewport/viewport.h"
#include "control/controller.h"
#include "control/camera_control.h"
#include "time/time.h"


/* --- assets ----------------------------------------------------------------
	An asset is a model plus whatever its kind needs. Scenery with no collider
	is pure dressing: it is drawn and nothing else.
*/

static const Asset room    = { .type = ASSET_SCENERY, .model = "rom:/models/room.t3dm"    };
static const Asset capsule = { .type = ASSET_SCENERY, .model = "rom:/models/capsule.t3dm" };


/* --- the scene -------------------------------------------------------------
	What the world contains and where. The load instances these in order;
	not const because each placement receives the entity it produced.
*/

static Scene3DAsset scene_assets[] = {

	{ &room,    { 0.0f, 0.0f, 0.0f } },
	{ &capsule, { 0.0f, 0.0f, 0.0f } },
};

static const CameraDef camera = {

	.type = CAMERA_TYPE_SPRING_ARM,

	.field_of_view = 60.0f,
	.near_clipping = 100.0f,
	.far_clipping  = 4000.0f,

	.spring_arm = {
		.arm_length   = 600.0f,
		.side_offset  = 0.0f,
		.yaw          = -45.0f,
		.pitch        = 15.0f,
		.pivot_height = 100.0f,

		.settings = {
			.response_rate = { 10.0f, 10.0f },
			.max_velocity  = { 60.0f, 40.0f },
			.direction     = {  1.0f, -1.0f },
			.zoom_response_rate = 6.0f,
			.distance_speed = 400.0f,
			.fov_speed      =  30.0f,
			.max_pitch     =  80.0f,
			.min_pitch     = -50.0f,
		},
	},
};

static const LightDef light = {

	.ambient_color = { 60, 60, 70, 0xFF },
	.point = {
		[0] = { .position = {{ 0.0f, 0.0f, 700.0f }},
		        .color = { 255, 245, 220, 0xFF }, .size = 2500.0f },
	},
};

static const FogDef fog = { .enabled = false };

static Scene3DDef scene = {

	.light  = &light,
	.fog    = &fog,
	.camera = &camera,

	.asset       = scene_assets,
	.asset_count = sizeof(scene_assets) / sizeof(scene_assets[0]),
};


/* --- the state -------------------------------------------------------------
	One state, one scene. The camera orbits the middle of the room instead of
	following a character, since there is nobody to follow yet.
*/

enum { EXAMPLE_SCENE_STATE, STATE_COUNT };

static Vector3 room_center = { 0.0f, 0.0f, 100.0f };

/* Naming the buttons is the whole job: the engine reads this every frame and
   turns, pulls back and zooms the camera on its own. Anything left at
   BTN_NONE simply never happens. */
static const CameraControlBinding camera_binding = {

	.pan_left  = BTN_C_LEFT,
	.pan_right = BTN_C_RIGHT,
	.tilt_up   = BTN_C_UP,
	.tilt_down = BTN_C_DOWN,

	.distance_in  = BTN_L,
	.distance_out = BTN_R,

	.fov_in    = BTN_D_UP,
	.fov_out   = BTN_D_DOWN,
};

static void exampleGameState_update(GameContext *ctx)
{
	(void)ctx;

	Scene3D *scene3d = scene3d_get();

	/* Model matrices are kept per framebuffer. Nothing moves here, so they
	   could be written once, but filling them every frame is what a scene
	   with anything moving in it has to do anyway. */
	for (int i = 0; i < scene3d->entity_count; i++)
		for (int fb = 0; fb < FB_COUNT; fb++)
			entity_setMatrixFromBody(scene3d->entity[i], fb);

	cameraControl_update(&viewport_get()->camera, &controller_get()[0], &camera_binding, time_get()->delta);
	viewport_updateCamera(&room_center);
	viewport_setPerspectiveCamera();
}

static const GameStateDef states[STATE_COUNT] = {

	[EXAMPLE_SCENE_STATE] = {
		.update     = exampleGameState_update,
		.scene3d    = &scene,
		.overlay_of = GAME_STATE_NONE,
	},
};


int main()
{
	debug_init_isviewer();
	debug_init_usblog();

	game_init();

	game_start(states, STATE_COUNT, EXAMPLE_SCENE_STATE);

	for (;;) game_runStep();

	game_close();

	return 0;
}
