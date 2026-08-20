#include "scene/scene.h"
#include "scene/demo_scene.h"
#include "assets/mr_muscles.h"
#include "assets/miss_jiggles.h"
#include "assets/room.h"
#include "assets/pole.h"
#include "assets/lamp.h"
#include "assets/crate.h"
#include "assets/ball.h"
#include "assets/brew64_flag.h"
#include "assets/pool.h"


/* What the scene contains and where: asset, position, rotation, scale.
   The load instances these in order; not const because each placement
   receives the reference to the entity it produced. */
static SceneAsset demo_assets[] = {

	{ &mr_muscles,   {1750.0f, -1750.0f, 200.0f}, {0.0f, 0.0f, 125.0f} },
	{ &miss_jiggles, {1600.0f, -1600.0f, 200.0f}, {0.0f, 0.0f, 125.0f} },

	{ &room, {0.0f, 0.0f, -2.0f} },

	/* Flag parked with its pole: the cloth's cost was eating the framerate.
	{ &pole,        {0.0f, 0.0f,  -2.0f}, {0}, {1.0f, 1.0f, 2.0f} },
	{ &brew64_flag, {5.0f, 0.0f, 688.0f}, {0}, {2.5f, 1.0f, 2.5f} },
	*/

	{ &pole, { 1300.0f, 0.0f,   0.0f}, {0}, {1.0f, 1.0f, 1.4f} },
	{ &lamp, { 1300.0f, 0.0f, 869.0f}, {0}, {2.0f, 2.0f, 2.0f} },
	{ &pole, {-1300.0f, 0.0f,   0.0f}, {0}, {1.0f, 1.0f, 1.4f} },
	{ &lamp, {-1300.0f, 0.0f, 869.0f}, {0}, {2.0f, 2.0f, 2.0f} },

	/* Buoyancy props, on the deck beside the pool's ramp edge: a stack of
	   equal crates and a big-to-small stack of balls, red at the bottom of
	   each. Push them in and each color finds its depth. */
	{ &red_crate,    {1200.0f, 1000.0f,  50.0f} },
	{ &yellow_crate, {1200.0f, 1000.0f, 152.0f} },
	{ &green_crate,  {1200.0f, 1000.0f, 254.0f} },

	{ &red_ball,    {1200.0f, 1500.0f,  50.0f} },
	{ &yellow_ball, {1200.0f, 1500.0f, 137.0f}, {0}, {0.7f, 0.7f, 0.7f} },
	{ &green_ball,  {1200.0f, 1500.0f, 196.0f}, {0}, {0.45f, 0.45f, 0.45f} },

	/* Last on purpose: the water is transparent and z-writes, so it has to
	   blend over everything already drawn. Coordinates are baked in the
	   model, world placement included. */
	{ &pool },
};


SceneDef demo_scene = {

	.light = {
		.ambient_color = {10, 10, 10, 0xFF},
		.point = {
			[0] = { .position = {{  1300.0f, 0.0f, 869.0f }}, .color = {255, 255, 255, 0xFF}, .size = 2500.0f },
			[1] = { .position = {{ -1300.0f, 0.0f, 869.0f }}, .color = {255, 255, 255, 0xFF}, .size = 2500.0f },
		},
	},

	.fog = {
		.color   = {70, 80, 100, 0xFF},
		.near    = 400.0f,
		.far     = 3500.0f,
		.enabled = true,
	},

	.wind = { 220.0f, 50.0f, 50.0f },

	.camera = {
		.type = CAMERA_TYPE_SPRING_ARM,
		.spring_arm = {
			.arm_length      = 200.0f,
			.shoulder_offset = 50.0f,
			.yaw             = -48.0f,
			.pitch           = 8.0f,
			.pivot_height    = 130.0f,
		},
	},

	.asset       = demo_assets,
	.asset_count = sizeof(demo_assets) / sizeof(demo_assets[0]),
};
