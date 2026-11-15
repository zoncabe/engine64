/*
	3D character

	How to create a simple static mesh character.

	A character is a body the engine moves for the game. It is not simulated by
	the physics solver: it collides itself against the world's static shapes,
	resolves its own penetration and writes its own transform. What it does is
	entirely decided by the settings declared below, since the engine ships no
	movement values of its own.

	What this example adds over example 01:

		a Prefab3D of type PREFAB3D_CHARACTER, carrying a character3d::Def
		a collision mesh on the room, so the body has something to stand on
		a control binding per piece that moves, named by the state
		an update that reads the seat, moves the body and follows it

	One gait is declared, which is the proportional locomotion case: speed
	follows stick displacement. Collision and the solver itself are example 03.
*/
#include <libdragon.h>

#include "game/e64_game.h" /* init, state table, frame step */
#include "game/e64_game_states.h"
#include "scene3d/e64_scene3d.h" /* scene declaration and live scene */
#include "entity/e64_entity3d.h" /* one prefab placed in the world */
#include "viewport/e64_viewport.h" /* screen modes and the live camera */
#include "player/e64_player.h" /* the seat a controller drives */
#include "controller/e64_controller.h" /* controller state and buttons */
#include "camera/e64_camera3d_control.h" /* buttons wired to camera motion */
#include "character3d/e64_character3d_control.h" /* buttons wired to a body */
#include "player/e64_player_control.h" /* reading a seat's buttons */
#include "controller/e64_controls.h" /* the controls a state declares */
#include "camera/e64_spring_arm.h" /* reading the arm back for debug */
#include "debug/e64_debug.h" /* on-screen debug lines */


/* Light, fog and camera are declared in scene/, one file each, and referenced
   here to build the scene. The placement table is further down. */
extern const e64::light::Def light;
extern const e64::fog::Def fog;
extern const e64::camera3d::Def camera;


/* --- the room --------------------------------------------------------------
	Floor and walls, and the triangle mesh the character stands on. A character
	falls through anything with no collision shape.

	Drawn geometry and collision geometry are separate assets built from the
	same .glb: the model importer produces the .t3dm, the collision importer
	the .collision, and the Makefile asks for the second one by listing it
	under assets_collision.
*/

/* A collider is an array of shapes, each with its own offset. SHAPE_MESH is
   triangle geometry and is static only: it collides but is never simulated,
   which is what level geometry wants. */
static const e64::physics::Shape::Def room_shapes[] = {

	{ .type = e64::physics::Shape::SHAPE_MESH, .mesh = { .path = "rom:/collision/room.collision" }},
};

static const e64::collider::Def room_collider = { room_shapes, 1 };

static const e64::Prefab3D room = {

	.type = e64::prefab3d::PREFAB3D_PROP,
	.mesh = { .model = "rom:/models/room.t3dm" },
	.collider = &room_collider,
};


/* --- the character ---------------------------------------------------------
	Everything a character is made of is settings. Distances are in metres and
	speeds in metres per second: a scene is written in the units the physics
	runs in.

	Gravity and terminal fall speed are not declared here, they are engine
	constants shared by every 3D character.
*/

/* A gait is one speed the body accelerates towards, with its own rate for
   getting there and for turning. Gaits are listed from slowest to fastest and
   the stick picks the highest one whose .stick_threshold it has passed.

   This character has one, which is the proportional case: speed follows stick
   displacement directly. Declaring a second gait switches locomotion to target
   speeds, dropping the proportional reading: the stick selects a gait by
   crossing its threshold, and the body accelerates towards that gait's speed at
   its response rate. */
static const e64::character3d::GaitSettings gait[] = {

	{ .target_speed = 7.0f, .response_rate = 8.0f, .rotation_response_rate = 12.0f },
};

static const e64::character3d::MovementSettings movement = {

	/* How fast the body sheds speed and settles its facing with no input. */
	.idle_response_rate = 12.0f,
	.idle_rotation_response_rate = 10.0f,

	.gait = gait,
	.gait_count = E64_ARRAY_COUNT(gait),

	/* The body leaves the floor on the press and keeps rising while the
	   button is held. */
	.jump_response_rate = 10.0f,
	.jump_base_speed = 9.0f,

	/* The fraction of gravity paid while the button stays down, and only on
	   the way up. Lower jumps higher, 1.0 makes holding do nothing.
	   This is what makes a tap and a held press reach different heights.

	   .jump_coyote_time belongs here too, and is left at zero: the jump stops
	   answering the moment the floor is gone. */
	.jump_hold_gravity_scale = 0.7f,

	/* How much of the ground's steering the body keeps in the air, 0 to 1. At
	   zero a jump holds the heading and speed it launched with and the stick
	   does nothing until it lands. At one the air steers like the floor. */
	.air_control = 0.8f,
};

/* The capsule the body collides with, standing upright. Radius and height in
   metres, height being the whole capsule end to end. */
static const e64::character3d::ColliderSettings collider = {

	.radius = 0.35f,
	.height = 1.80f,
};

/* A character is assembled from independent blocks of settings, and only the
   ones it needs. Animation, weapons, aiming, sound, stats and spring bones are
   all optional and left out here: this body walks and jumps, nothing else. */
static const e64::character3d::Def character3d_def = {

	.movement_settings = &movement,
	.collider_settings = &collider
};

/* The prefab a scene can place: the model drawn for it, plus the character
   built on top. PREFAB3D_CHARACTER is what makes the engine create a
   Character3D for this entity instead of leaving it as scenery. */
static const e64::Prefab3D character = {

	.type = e64::prefab3d::PREFAB3D_CHARACTER,
	.mesh = { .model = "rom:/models/capsule.t3dm" },
	.character = &character3d_def,
};


/* --- the controls ----------------------------------------------------------
	Which buttons drive what, and whose controller they are read from. A
	binding names the piece it moves, so the engine wires it when the state is
	entered: nothing below is called from the frame.

	An action left out is BTN_NONE and never fires. The stick is bound in
	neither: it always drives the body, and the camera takes its angle from
	where it already is.
*/

/* Naming a player here also decides what the camera follows: it tracks the
   body seated in that slot, with no target passed in from the game. */
static const e64::camera3d::ControlBinding camera_binding = {

	.player = e64::PLAYER_1,
	.camera = &camera,

	.pan_left = e64::BTN_C_LEFT,
	.pan_right = e64::BTN_C_RIGHT,
	.tilt_up = e64::BTN_C_UP,
	.tilt_down = e64::BTN_C_DOWN,

	.distance_in = e64::BTN_L,
	.distance_out = e64::BTN_R,

	.fov_in = e64::BTN_D_UP,
	.fov_out = e64::BTN_D_DOWN,
};

/* The scene entity it drives is what seats the player: the scene builds a
   body from that row, and that body is the one this controller moves. The
   table is declared further down. */
extern e64::scene3d::Entity scene_entities[];

static const e64::character3d::ControlBinding character3d_binding = {

	.player = e64::PLAYER_1,
	.character = &scene_entities[1],

	.jump = e64::BTN_A,
};

static const e64::controls::Def controls = {

	.camera = &camera_binding,
	.character3d = &character3d_binding,
};



/* --- the scene -------------------------------------------------------------
	One row per entity: which prefab, then position, rotation and scale.
	Prefabs carry no transform, so placing one twice is just another row.

	Rows are built in order and the live scene keeps that order. The character
	binding above points at its row, which is how the player gets that body.

	Fields left out of a row are zero, and a zero scale means original size.
*/

e64::scene3d::Entity scene_entities[] = {

	{ &room },
	{ &character, { 0.0f, 0.0f, 0.0f } },
};

static e64::scene3d::Def scene = {

	.light = &light,
	.fog = &fog,
	.camera = &camera,

	.entity = scene_entities,
	.entity_count = E64_ARRAY_COUNT(scene_entities),
};


/* --- the state -------------------------------------------------------------
	One mode of the game, carrying the scene it runs and the functions the
	engine calls while it is current.
*/

enum { GAMEPLAY3D, STATE_COUNT };

static void gameplay3d_update(void)
{
	e64::Viewport *viewport = e64::viewport::get();

	/* Debug lines have to be rewritten every frame; nothing persists. */
	e64::debug::ui::set(0, "STICK walk");
	e64::debug::ui::set(1, "A jump");
	e64::debug::ui::set(3, "CBUTTONS orbit camera");
	e64::debug::ui::set(4, "L R arm length");
	e64::debug::ui::set(5, "DPAD fov");

	e64::debug::ui::showFPS();
	e64::debug::ui::setRight(0, "arm %.1f", e64::camera3d::springArm::getLength(&viewport->camera));
	e64::debug::ui::setRight(1, "fov %.1f", viewport->camera.field_of_view);
}

static const e64::Game::State::Def states[STATE_COUNT] = {

	[GAMEPLAY3D] = {
		.update = gameplay3d_update,
		.scene3d = &scene,

		/* Wired once, after the scene is loaded and before the first update:
		   the player is seated on the body the binding names, and the camera
		   answers to the buttons that name it. */
		.controls = &controls,

		/* The engine opens no screen by itself, so every state that draws has
		   to name a mode. */
		.viewport = SCREEN_320x240,
	},
};


int main()
{
	/* Where printf output goes: the emulator's debug viewer, and USB on a
	   flashcart. Both optional, neither affects the game. */
	debug_init_isviewer();
	debug_init_usblog();

	/* Brings up video, audio, controllers, physics and the ROM filesystem.
	   Runs before anything else the engine offers. */
	e64::game::init();

	e64::debug::ui::init();

	/* Hands over the state table and enters the initial state, which loads the
	   scene and then wires the controls it declared. */
	e64::game::state::start(states, STATE_COUNT, GAMEPLAY3D);

	/* One frame per iteration: poll the controllers, step the physics, run the
	   current state's update, draw. */
	for (;;) e64::game::runStep();

	e64::game::close();

	return 0;
}
