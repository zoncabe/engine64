#include "scene/scene.h"
#include "scene/demo_scene.h"
#include "assets/male_muscled.h"




/* --- Static collider shapes for the test props (metres). --- */

static const EntityShapeDef crate_shape_def = {
	.type = SHAPE_BOX,
	.box  = { .e = { 0.5f, 0.5f, 0.5f } },
};

static const EntityShapeDef ball_shape_def = {
	.type   = SHAPE_SPHERE,
	.sphere = { .radius = 0.5f },
};

/* The capsule model has its base at the origin: lift the shape to centre it. */
static const EntityShapeDef pillar_shape_def = {
	.type    = SHAPE_CAPSULE,
	.capsule = {
		.tx          = { .position = { 0.0f, 0.0f, 0.9f } },
		.radius      = 0.354f,
		.half_height = 0.546f,
	},
};


const SceneDef demo_scene = {

	.light = {
		.ambient_color = {60, 60, 60, 0xFF},
		.directional = {
			[0] = { .color = {210, 210, 210, 0xFF}, .direction = {{1.0f, -1.0f, 1.0f}} },
		},
	},

	.camera = {
		.type = CAMERA_TYPE_SPHERICAL,
		.spherical = {
			.distance_from_center      = 200.0f,
			.distance_center_to_target = 300.0f,
			.angle_around_center       = 20.0f,
			.offset_angle              = 30.0f,
			.pitch                     = 8.0f,
			.offset_height             = 130.0f,
		},
	},

	.entity = {
		[0] = {
			.model_path = male_muscled_model,
			.position   = {-210.0f, -210.0f, 0.0f},
			.rotation   = {0.0f, 0.0f, 200.0f},
			.scale      = {1.0f, 1.0f, 1.0f},
			.character  = &male_muscled_character_def,
			.weapon = {
				[WEAPON_SLOT_WAIST] = &weapon_m1911,
				[WEAPON_SLOT_BACK]  = &weapon_ak47,
				[WEAPON_SLOT_MELEE] = &weapon_knife,
			},
		},
		[1] = {
			.model_path     = "rom:/models/room.t3dm",
			.collision_path = "rom:/collision/room.collision",
			.position   = {0.0f, 0.0f, -2.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {1.0f, 1.0f, 1.0f},
		},
		[2] = {
			.model_path = "rom:/models/axis.t3dm",
			.position   = {0.0f, 0.0f, -2.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {1.5f, 1.5f, 1.5f},
		},

		/* Ramp battery: big tilted crates half-buried, increasing slope. */
		[3] = {
			.model_path = "rom:/models/green_box.t3dm",
			.position   = {900.0f, -900.0f, -350.0f},
			.rotation   = {0.0f, 20.0f, 0.0f},
			.scale      = {10.0f, 10.0f, 10.0f},
			.shape      = &crate_shape_def,
		},
		[4] = {
			.model_path = "rom:/models/yellow_box.t3dm",
			.position   = {1700.0f, 300.0f, -320.0f},
			.rotation   = {0.0f, 35.0f, 15.0f},
			.scale      = {10.0f, 10.0f, 10.0f},
			.shape      = &crate_shape_def,
		},
		[5] = {
			.model_path = "rom:/models/red_box.t3dm",
			.position   = {-1300.0f, 900.0f, -400.0f},
			.rotation   = {25.0f, 0.0f, 0.0f},
			.scale      = {10.0f, 10.0f, 10.0f},
			.shape      = &crate_shape_def,
		},

		/* Terrain mounds: spheres half-buried in the floor. */
		[6] = {
			.model_path = "rom:/models/green_sphere.t3dm",
			.position   = {-900.0f, -1500.0f, -250.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {8.0f, 8.0f, 8.0f},
			.shape      = &ball_shape_def,
		},
		[7] = {
			.model_path = "rom:/models/red_sphere.t3dm",
			.position   = {500.0f, 1300.0f, -200.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {6.0f, 6.0f, 6.0f},
			.shape      = &ball_shape_def,
		},

		/* Stairs: three crates with 0.4 m rises. */
		[8] = {
			.model_path = "rom:/models/green_box.t3dm",
			.position   = {-500.0f, -900.0f, -60.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {2.0f, 2.0f, 2.0f},
			.shape      = &crate_shape_def,
		},
		[9] = {
			.model_path = "rom:/models/yellow_box.t3dm",
			.position   = {-500.0f, -1100.0f, -20.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {2.0f, 2.0f, 2.0f},
			.shape      = &crate_shape_def,
		},
		[10] = {
			.model_path = "rom:/models/red_box.t3dm",
			.position   = {-500.0f, -1300.0f, 20.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {2.0f, 2.0f, 2.0f},
			.shape      = &crate_shape_def,
		},

		/* Two capsule pillars with a bar bridging them at jump height. */
		[11] = {
			.model_path = "rom:/models/capsule.t3dm",
			.position   = {0.0f, 700.0f, -2.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {2.0f, 2.0f, 2.0f},
			.shape      = &pillar_shape_def,
		},
		[12] = {
			.model_path = "rom:/models/capsule.t3dm",
			.position   = {300.0f, 700.0f, -2.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {2.0f, 2.0f, 2.0f},
			.shape      = &pillar_shape_def,
		},
		[13] = {
			.model_path = "rom:/models/yellow_box.t3dm",
			.position   = {150.0f, 700.0f, 250.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {4.0f, 1.0f, 1.0f},
			.shape      = &crate_shape_def,
		},

		/* Tilted low ceiling slab to test collision from below. */
		[14] = {
			.model_path = "rom:/models/red_box.t3dm",
			.position   = {-300.0f, -400.0f, 300.0f},
			.rotation   = {0.0f, 10.0f, 0.0f},
			.scale      = {4.0f, 4.0f, 1.0f},
			.shape      = &crate_shape_def,
		},
	},
	.entity_count = 15,
};
