/*
	A body in a room: walk it, run it, jump it, push it into the water and up
	the ladder. Everything the character does comes from its settings and the
	solver; the engine has no movement of its own.

	The prefabs live one per file under prefabs/. What is here is the world:
	where each one stands, the light, the camera and the one state that runs
	the frame.
*/
#include <libdragon.h>

#include "game/e64_game.h"
#include "scene3d/e64_scene3d.h"
#include "entity/e64_entity.h"
#include "viewport/e64_viewport.h"
#include "player/e64_player.h"
#include "control/e64_controller.h"
#include "control/e64_camera_control.h"
#include "control/e64_character_control.h"
#include "control/e64_player_control.h"
#include "shaders/e64_water.h"
#include "time/e64_time.h"


/* --- prefabs ---------------------------------------------------------------
	One file each, under prefabs/: the seven pieces of content, and the camera,
	the light and the fog the scene runs with.
*/

extern const Prefab room;

extern const Prefab water;
extern const Prefab ladder;

extern const Prefab capsule;
extern const Prefab cube;
extern const Prefab sphere;

extern const Prefab character;

extern const CameraDef camera;
extern const LightDef  light;
extern const FogDef    fog;


/* --- the scene -------------------------------------------------------------
	The room is 5000 across, five quads of a thousand a side. The platform
	rises 500 in one corner, the mound peaks at 200 across from it, and the
	pool is sunk 250 into the middle.
*/

/* One row per placement: which prefab, then where it stands. Declaring them
   here is the whole job — the load builds each one in order, registers it in
   the physics and draws it, with nothing else to call. What is left out stays
   zero, and a zero scale means original size. */
static Scene3DPrefab scene_prefabs[] = {

	{ &character, { 2000.0f, -2000.0f, 0.0f }, { 0.0f, 0.0f, 135.0f } },

	/* Three still bodies in a row, parallel to the water's edge. The cube and
	   the ball are modelled around their middle, so at double size they sit
	   a metre up to rest on the floor. The scale carries their collision
	   with it. */
	{ &capsule, {     0.0f, 0.0f,   0.0f } },
	{ &cube,    { -1000.0f, 0.0f, 100.0f }, {0}, { 2.0f, 2.0f, 2.0f } },
	{ &sphere,  {  1000.0f, 0.0f, 100.0f }, {0}, { 2.0f, 2.0f, 2.0f } },
	
	/* Halfway along the platform's east face, facing the mound, 500 tall,
	   which is exactly the climb. Stood off the wall on purpose: flush
	   against it the body meets the platform before it can reach the volume
	   it grabs, and the climb never starts. */
	{ &ladder, { -480.0f, -1000.0f, 0.0f }, { 0.0f, 0.0f, -90.0f } },

	{ &room },

	/* Last on purpose: the water is transparent, so it has to blend over
	   everything already drawn. */
	{ &water, { 0.0f, 1000.0f, -50.0f } },
};

static Scene3DDef scene = {

	.light  = &light,
	.fog    = &fog,
	.camera = &camera,

	.prefab       = scene_prefabs,
	.prefab_count = sizeof(scene_prefabs) / sizeof(scene_prefabs[0]),
};


/* --- the state -------------------------------------------------------------
	A state is one mode of the game: it carries the scene it draws and the
	function the engine calls every frame. This one is the only mode here,
	and its update is where the physics runs, the body is driven and the
	camera follows it.
*/

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

static const CharacterControlBinding character_binding = {

	.player = PLAYER_1,

	.jump   = BTN_A,
	.roll   = BTN_B,
	.sprint = BTN_Z,
};

/* The scene loads its characters in placement order; this one is the only one,
   so the player takes the first, with the buttons above. */
static void GameStateExample_bindCharacter(void)
{
	player_setCharacter(scene3d_getCharacter(0), &character_binding);
}

static void GameStateExample_update(GameContext *ctx)
{
	uint8_t fb = ctx->viewport->fb_index;
	float delta = time_get()->delta;
	
	player_setCharacterControl(PLAYER_1, ctx->viewport);
	player_update();
	
	water_update(delta);
	
	/* A character is not placed by the solver: it collides itself against the
	world the solver just settled, and from there reaches what draws it. */
	scene3d_updateCharacters(fb);
	   
	cameraControl_update(&viewport_get()->camera, &camera_binding, delta);
	viewport_setPerspectiveCamera();
}

enum { GAME_STATE_EXAMPLE, STATE_COUNT };

static const GameStateDef states[STATE_COUNT] = {
	
	[GAME_STATE_EXAMPLE] = {
		.update        = GameStateExample_update,
		.bindCharacter = GameStateExample_bindCharacter,
		.scene3d       = &scene,
		.overlay_of    = GAME_STATE_NONE,
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
