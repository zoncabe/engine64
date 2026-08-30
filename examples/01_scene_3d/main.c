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


/* --- prefabs ---------------------------------------------------------------
	A prefab is a model plus what its kind needs. A prop with no collider and
	no body is only drawn.
*/

static const Prefab room    = { .type = PREFAB_PROP, .model = "rom:/models/room.t3dm"    };
static const Prefab capsule = { .type = PREFAB_PROP, .model = "rom:/models/capsule.t3dm" };


/* --- the scene -------------------------------------------------------------
	What the world contains and where. The load instances these in order;
	not const because each placement receives the entity it produced.
*/

/* One row per placement: which prefab, then where it stands. Declaring them
   here is the whole job — the load builds each one in order, registers it in
   the physics and draws it, with nothing else to call. What is left out stays
   zero, and a zero scale means original size. */
static Scene3DPrefab scene_prefabs[] = {

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

/* Seven slots, shared between directional and point lights. They are read in
   order and cut at the first empty one, so the six left over cost nothing. */
static const LightDef light = {

	.ambient_color = { 60, 60, 70, 0xFF },

	.source = {
		{ .type  = LIGHT_POINT,
		  .color = { 255, 245, 220, 0xFF },
		  .point = { .position = {{ 0.0f, 0.0f, 700.0f }}, .size = 2500.0f } },
	},
};

static const FogDef fog = { .enabled = false };

static Scene3DDef scene = {

	.light  = &light,
	.fog    = &fog,
	.camera = &camera,

	.prefab       = scene_prefabs,
	.prefab_count = sizeof(scene_prefabs) / sizeof(scene_prefabs[0]),
};


/* --- the state -------------------------------------------------------------
	A state is one mode of the game: the title menu, the match, the pause,
	the credits. One at a time. Each carries the scene it draws, the sprites
	and fonts it needs, and the function the engine calls every frame. Leaving
	a state frees all of that, and the next one loads its own.

	This table is the whole game as far as the engine is concerned: it takes
	it at startup and from then on the only thing it ever calls is the update
	of the current state. That is why there is one even here, for a single
	mode that never changes.
*/

enum { GAME_STATE_EXAMPLE, STATE_COUNT };

static Vector3 room_center = { 0.0f, 0.0f, 100.0f };

/* Naming the buttons is the whole job: the engine reads this every frame and
   turns, pulls back and zooms the camera on its own. Anything left at
   BTN_NONE simply never happens. */
static const CameraControlBinding camera_binding = {

	.player = PLAYER_1,

	.pan_left  = BTN_C_LEFT,
	.pan_right = BTN_C_RIGHT,
	.tilt_up   = BTN_C_UP,
	.tilt_down = BTN_C_DOWN,

	.distance_in  = BTN_L,
	.distance_out = BTN_R,

	.fov_in    = BTN_D_UP,
	.fov_out   = BTN_D_DOWN,
};

static void GameStateExample_update(GameContext *ctx)
{
	(void)ctx;

	Scene3D *scene3d = scene3d_get();

	/* Model matrices are kept per framebuffer. Nothing moves here, so they
	   could be written once, but filling them every frame is what a scene
	   with anything moving in it has to do anyway. */
	for (int i = 0; i < scene3d->entity_count; i++)
		for (int fb = 0; fb < FB_COUNT; fb++)
			entity_setMatrixFromBody(scene3d->entity[i], fb);

	cameraControl_update(&viewport_get()->camera, &camera_binding, time_get()->delta);
	viewport_updateCamera(&room_center);
	viewport_setPerspectiveCamera();
}

static const GameStateDef states[STATE_COUNT] = {

	[GAME_STATE_EXAMPLE] = {
		.update     = GameStateExample_update,
		.scene3d    = &scene,
		.overlay_of = GAME_STATE_NONE,
	},
};


int main()
{
	debug_init_isviewer();
	debug_init_usblog();

	game_init();

	game_start(states, STATE_COUNT, GAME_STATE_EXAMPLE);

	for (;;) game_runStep();

	game_close();

	return 0;
}
