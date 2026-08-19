#include "scene/scene.h"
#include "scene/demo_scene.h"
#include "assets/mr_muscles.h"
#include "assets/miss_jiggles.h"

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

/* Flag parked for now: the cloth's cost was eating the framerate.
static const ClothDef flag_cloth = {
	.mesh_path  = "rom:/collision/brew_flag.collision",
	// Jakobsen's paper: damped Verlet x' = 1.99x - 0.99x* (damping 0.01) and
	// 3-4 relaxation passes.
	.damping    = 0.01f,
	.iterations = 4,
	.pin_max_x  = 0.01f,
};
*/

/* The pool as a volume: footprint of the water plane, from the basin floor
   up to the resting surface. Sensor, so it reports who is inside without
   colliding; buoyancy reads its overlaps once the water binds to it. */
static const PhysicsShapeDef pool_shapes[] = {
	{ .type = SHAPE_BOX, .box = {
		.tx     = { .position = { -5.0f, 12.5f, -1.45f } },
		.e      = { 15.0f, 7.5f, 0.975f },
		.sensor = 1,
	}},
};

/* Color says density, density says how deep it floats: the submerged
   fraction at rest is density over the water's 1000. Green rides at 30%,
   yellow at 80%, red outweighs the water and goes to the bottom. */
#define PROP_DENSITY_GREEN   300.0f
#define PROP_DENSITY_YELLOW  800.0f
#define PROP_DENSITY_RED    1200.0f

#define PROP_BOX(d)    { .type = SHAPE_BOX, .box = { \
	.e = { 0.5f, 0.5f, 0.5f }, \
	.friction = 0.4f, .restitution = 0.1f, .density = d }}

#define PROP_SPHERE(d) { .type = SHAPE_SPHERE, .sphere = { \
	.radius = 0.5f, \
	.friction = 0.4f, .restitution = 0.1f, .density = d }}

static const PhysicsShapeDef green_box_shapes[]     = { PROP_BOX(PROP_DENSITY_GREEN) };
static const PhysicsShapeDef yellow_box_shapes[]    = { PROP_BOX(PROP_DENSITY_YELLOW) };
static const PhysicsShapeDef red_box_shapes[]       = { PROP_BOX(PROP_DENSITY_RED) };
static const PhysicsShapeDef green_sphere_shapes[]  = { PROP_SPHERE(PROP_DENSITY_GREEN) };
static const PhysicsShapeDef yellow_sphere_shapes[] = { PROP_SPHERE(PROP_DENSITY_YELLOW) };
static const PhysicsShapeDef red_sphere_shapes[]    = { PROP_SPHERE(PROP_DENSITY_RED) };

static const RigidBodyDef prop_body = {
	.body_type     = BODY_DYNAMIC,
	.gravity_scale = 1.0f,
	.allow_sleep   = 1,
	.awake         = 1,
	.active        = 1,
};

static const EntityColliderDef room_collider          = { room_shapes,          1 };
static const EntityColliderDef pole_collider          = { pole_shapes,          1 };
static const EntityColliderDef pool_collider          = { pool_shapes,          1 };
static const EntityColliderDef green_box_collider     = { green_box_shapes,     1 };
static const EntityColliderDef yellow_box_collider    = { yellow_box_shapes,    1 };
static const EntityColliderDef red_box_collider       = { red_box_shapes,       1 };
static const EntityColliderDef green_sphere_collider  = { green_sphere_shapes,  1 };
static const EntityColliderDef yellow_sphere_collider = { yellow_sphere_shapes, 1 };
static const EntityColliderDef red_sphere_collider    = { red_sphere_shapes,    1 };

static const WaterDef pool_water = {
	.mesh_path = "rom:/collision/water.collision",
	.wave = {
		{ .direction_x =  1.0f, .direction_y =  0.3f, .amplitude = 0.05f, .frequency = 1.6f, .speed = 1.6f },
		{ .direction_x = -0.4f, .direction_y =  1.0f, .amplitude = 0.03f, .frequency = 2.9f, .speed = 2.3f },
		{ .direction_x =  0.6f, .direction_y = -1.0f, .amplitude = 0.02f, .frequency = 4.3f, .speed = 3.1f },
	},
	.wave_count = 3,
	.scroll_a = {  1.5f, 2.4f },
	.scroll_b = { -2.0f, 1.0f },
	.wrap_a = 64.0f,
	.wrap_b = 64.0f,
	.color  = { 110, 180, 215 },
};


const SceneDef demo_scene = {

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

	.entity = {
		[0] = {
			.model_path = mr_muscles_model,
			.position   = {1750.0f, -1750.0f, 200.0f},
			.rotation   = {0.0f, 0.0f, 125.0f},
			.scale      = {1.0f, 1.0f, 1.0f},
			.cull       = true,
			.character  = &mr_muscles_character_def,
		},
		
		[1] = {
			.model_path = miss_jiggles_model,
			.position   = {1600.0f, -1600.0f, 200.0f},
			.rotation   = {0.0f, 0.0f, 125.0f},
			.scale      = {1.0f, 1.0f, 1.0f},
			.cull       = true,
			.character  = &miss_jiggles_character_def,
		},

		[2] = {
			.model_path = "rom:/models/room.t3dm",
			.position   = {0.0f, 0.0f, -2.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {1.0f, 1.0f, 1.0f},
			.cull       = true,
			.collider   = &room_collider,
		},

		/* Flag parked with its pole and cloth def, same reason.
		{
			.model_path = "rom:/models/pole.t3dm",
			.position   = {0.0f, 0.0f, -2.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {1.0f, 1.0f, 2.0f},
			.cull       = true,
			.collider   = &pole_collider,
		},
		{
			.model_path = "rom:/models/brew_flag.t3dm",
			.position   = {5.0f, 0.0f, 688.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {2.5f, 1.0f, 2.5f},
			.cull       = true,
			.cloth      = &flag_cloth,
		},
		*/
		[3] = {
			.model_path = "rom:/models/pole.t3dm",
			.position   = {1300.0f, 0.0f, 0.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {1.0f, 1.0f, 1.4f},
			.cull       = true,
			.collider   = &pole_collider,
		},
		[4] = {
			.model_path = "rom:/models/lamp.t3dm",
			.position   = {1300.0f, 0.0f, 869.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {2.0f, 2.0f, 2.0f},
			.cull       = true,
		},

		[5] = {
			.model_path = "rom:/models/pole.t3dm",
			.position   = {-1300.0f, 0.0f, 0.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {1.0f, 1.0f, 1.4f},
			.cull       = true,
			.collider   = &pole_collider,
		},
		[6] = {
			.model_path = "rom:/models/lamp.t3dm",
			.position   = {-1300.0f, 0.0f, 869.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {2.0f, 2.0f, 2.0f},
			.cull       = true,
		},

		/* Buoyancy props, on the deck beside the pool's ramp edge: a stack
		   of equal boxes and a big-to-small stack of spheres, red at the
		   bottom of each. Push them in and each color finds its depth. */
		[7] = {
			.model_path = "rom:/models/red_box.t3dm",
			.position   = {1200.0f, 1000.0f, 50.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {1.0f, 1.0f, 1.0f},
			.cull       = true,
			.collider   = &red_box_collider,
			.body       = &prop_body,
		},
		[8] = {
			.model_path = "rom:/models/yellow_box.t3dm",
			.position   = {1200.0f, 1000.0f, 152.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {1.0f, 1.0f, 1.0f},
			.cull       = true,
			.collider   = &yellow_box_collider,
			.body       = &prop_body,
		},
		[9] = {
			.model_path = "rom:/models/green_box.t3dm",
			.position   = {1200.0f, 1000.0f, 254.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {1.0f, 1.0f, 1.0f},
			.cull       = true,
			.collider   = &green_box_collider,
			.body       = &prop_body,
		},

		[10] = {
			.model_path = "rom:/models/red_sphere.t3dm",
			.position   = {1200.0f, 1500.0f, 50.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {1.0f, 1.0f, 1.0f},
			.cull       = true,
			.collider   = &red_sphere_collider,
			.body       = &prop_body,
		},
		[11] = {
			.model_path = "rom:/models/yellow_sphere.t3dm",
			.position   = {1200.0f, 1500.0f, 137.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {0.7f, 0.7f, 0.7f},
			.cull       = true,
			.collider   = &yellow_sphere_collider,
			.body       = &prop_body,
		},
		[12] = {
			.model_path = "rom:/models/green_sphere.t3dm",
			.position   = {1200.0f, 1500.0f, 196.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {0.45f, 0.45f, 0.45f},
			.cull       = true,
			.collider   = &green_sphere_collider,
			.body       = &prop_body,
		},

		/* Last on purpose: the water is transparent and z-writes, so it has
		   to blend over everything already drawn. Coordinates are baked in
		   the model, world placement included. */
		[13] = {
			.model_path = "rom:/models/water.t3dm",
			.position   = {0.0f, 0.0f, 0.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {1.0f, 1.0f, 1.0f},
			.cull       = true,
			.water      = &pool_water,
			.collider   = &pool_collider,
		},
	},
	.entity_count = 14,

	.sound_count = 0,
};
