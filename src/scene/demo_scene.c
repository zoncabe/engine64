#include "scene/scene.h"
#include "scene/demo_scene.h"
#include "assets/male_muscled.h"




static const PhysicsShapeDef room_shapes[] = {
	{ .type = SHAPE_MESH, .mesh = {
		.path        = "rom:/collision/room.collision",
		.friction    = 0.9f,
		.restitution = 0.1f,
	}},
};

static const PhysicsShapeDef pole_shapes[] = {
	{ .type = SHAPE_BOX, .box = {
		.tx = { .position = { 0.0f, 0.0f, 2.75f } },
		.e  = { 0.0422f, 0.0422f, 2.75f },
	}},
};

static const ClothDef flag_cloth = {
	.mesh_path  = "rom:/collision/brew_flag.collision",
	/* Jakobsen's paper: damped Verlet x' = 1.99x - 0.99x* (damping 0.01) and
	   3-4 relaxation passes. */
	.damping    = 0.01f,
	.iterations = 4,
	.pin_max_x  = 0.01f,
};

static const EntityColliderDef room_collider = { room_shapes, 1 };
static const EntityColliderDef pole_collider = { pole_shapes, 1 };


const SceneDef demo_scene = {

	.light = {
		.ambient_color = {10, 10, 10, 0xFF},
		.point = {
			[0] = { .position = {{ 0.0f,  1300.0f, 869.0f }}, .color = {255, 255, 255, 0xFF}, .size = 2500.0f },
			[1] = { .position = {{ 0.0f, -1300.0f, 869.0f }}, .color = {255, 255, 255, 0xFF}, .size = 2500.0f },
		},
	},

	.wind = { 220.0f, 50.0f, 50.0f },

	.camera = {
		.type = CAMERA_TYPE_SPHERICAL,
		.spherical = {
			.distance_from_center      = 200.0f,
			.distance_center_to_target = 300.0f,
			.angle_around_center       = -48.0f,
			.offset_angle              = 30.0f,
			.pitch                     = 8.0f,
			.offset_height             = 130.0f,
		},
	},

	.entity = {
		[0] = {
			.model_path = male_muscled_model,
			.position   = {1750.0f, -1750.0f, 200.0f},
			.rotation   = {0.0f, 0.0f, 125.0f},
			.scale      = {1.0f, 1.0f, 1.0f},
			.character  = &male_muscled_character_def,
		},
		[1] = {
			.model_path = "rom:/models/room.t3dm",
			.position   = {0.0f, 0.0f, -2.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {1.0f, 1.0f, 1.0f},
			.collider   = &room_collider,
		},

		[2] = {
			.model_path = "rom:/models/pole.t3dm",
			.position   = {0.0f, 0.0f, -2.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {1.0f, 1.0f, 2.0f},
			.collider   = &pole_collider,
		},
		[3] = {
			.model_path = "rom:/models/brew_flag.t3dm",
			.position   = {5.0f, 0.0f, 688.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {2.5f, 1.0f, 2.5f},
			.cloth      = &flag_cloth,
		},

		[4] = {
			.model_path = "rom:/models/pole.t3dm",
			.position   = {0.0f, 1300.0f, 0.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {1.0f, 1.0f, 1.4f},
			.collider   = &pole_collider,
		},
		[5] = {
			.model_path = "rom:/models/lamp.t3dm",
			.position   = {0.0f, 1300.0f, 869.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {2.0f, 2.0f, 2.0f},
		},

		[6] = {
			.model_path = "rom:/models/pole.t3dm",
			.position   = {0.0f, -1300.0f, 0.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {1.0f, 1.0f, 1.4f},
			.collider   = &pole_collider,
		},
		[7] = {
			.model_path = "rom:/models/lamp.t3dm",
			.position   = {0.0f, -1300.0f, 869.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {2.0f, 2.0f, 2.0f},
		},
	},
	.entity_count = 8,
};
