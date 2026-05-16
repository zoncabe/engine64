#include "scene/scene.h"
#include "scene/demo_scene.h"
#include "assets/male_muscled.h"


static const ActorDef male_muscled_actor_def = {
	.motion_settings    = &male_muscled_motion_settings,
	.animation_settings = &male_muscled_animation_settings,
	.animation_def      = &male_muscled_animation_def,
};


/* --- Body defs (body-side only; shape attaches separately). --- */

static const RigidBodyDef player_body_def = {
	.body_type       = BODY_KINEMATIC,
	.gravity_scale   = 2.0f,
	.layers          = 1,
	.linear_damping  = 0.1f,
	.angular_damping = 1.0f,
	.allow_sleep     = 0,
	.awake           = 1,
	.active          = 1,
	.lock_axis_x     = 1,
	.lock_axis_y     = 1,
};

static const RigidBodyDef static_body_def = {
	.body_type       = BODY_STATIC,
	.layers          = 1,
	.allow_sleep     = 1,
	.awake           = 1,
	.active          = 1,
};

static const RigidBodyDef dynamic_body_def = {
	.body_type       = BODY_DYNAMIC,
	.gravity_scale   = 1.0f,
	.layers          = 1,
	.linear_damping  = 0.05f,
	.angular_damping = 0.1f,
	.allow_sleep     = 1,
	.awake           = 1,
	.active          = 1,
};


/* --- Shape defs. --- */

/* Shape dimensions are in physics units (metres). Render uses ×100. */

static const EntityShapeDef player_shape_def = {
	.type    = SHAPE_CAPSULE,
	.capsule = {
		.tx          = { .position = { 0.0f, 0.0f, 0.95f } },  /* lift so base sits at body z=0 */
		.radius      = 0.30f,
		.half_height = 0.65f,    /* total height = 2*(r + half_height) = 1.9 m */
		.friction    = 0.5f,
		.restitution = 0.0f,
		.density     = 1.0f,
	},
};

static const EntityShapeDef room_shape_def = {
	.type = SHAPE_BOX,
	.box = {
		/* Thin floor slab, top surface at z = 0. */
		.e           = { 30.0f, 30.0f, 0.02f },
		.friction    = 0.6f,
		.restitution = 0.0f,
		.density     = 0.0f,
	},
};

static const EntityShapeDef crate_shape_def = {
	.type = SHAPE_BOX,
	.box = {
		.e           = { 0.5f, 0.5f, 0.5f },
		.friction    = 0.6f,
		.restitution = 0.0f,
		.density     = 1.0f,
	},
};

static const EntityShapeDef ball_shape_def = {
	.type   = SHAPE_SPHERE,
	.sphere = {
		.radius      = 0.25f,
		.friction    = 0.4f,
		.restitution = 0.3f,
		.density     = 1.0f,
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
			.actor      = &male_muscled_actor_def,
			.body       = &player_body_def,
			.shape      = &player_shape_def,
		},
		[1] = {
			.model_path = "rom:/models/room.t3dm",
			.position   = {0.0f, 0.0f, -2.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {1.0f, 1.0f, 1.0f},
			.body       = &static_body_def,
			.shape      = &room_shape_def,
		},
		[2] = {
			.model_path = "rom:/models/axis.t3dm",
			.position   = {0.0f, 0.0f, -2.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {1.5f, 1.5f, 1.5f},
		},

		[3] = {
			.model_path = "rom:/models/green_box.t3dm",
			.position   = {250.0f, -50.0f, 50.0f},
			.scale      = {1.0f, 1.0f, 1.0f},
			.body       = &dynamic_body_def,
			.shape      = &crate_shape_def,
		},
		[4] = {
			.model_path = "rom:/models/red_box.t3dm",
			.position   = {250.0f, 50.0f, 50.0f},
			.scale      = {1.0f, 1.0f, 1.0f},
			.body       = &dynamic_body_def,
			.shape      = &crate_shape_def,
		},
		[5] = {
			.model_path = "rom:/models/yellow_box.t3dm",
			.position   = {350.0f, -50.0f, 50.0f},
			.scale      = {1.0f, 1.0f, 1.0f},
			.body       = &dynamic_body_def,
			.shape      = &crate_shape_def,
		},
		[6] = {
			.model_path = "rom:/models/green_box.t3dm",
			.position   = {350.0f, 50.0f, 50.0f},
			.scale      = {1.0f, 1.0f, 1.0f},
			.body       = &dynamic_body_def,
			.shape      = &crate_shape_def,
		},
		[7] = {
			.model_path = "rom:/models/red_box.t3dm",
			.position   = {250.0f, -50.0f, 150.0f},
			.scale      = {1.0f, 1.0f, 1.0f},
			.body       = &dynamic_body_def,
			.shape      = &crate_shape_def,
		},
		[8] = {
			.model_path = "rom:/models/yellow_box.t3dm",
			.position   = {250.0f, 50.0f, 150.0f},
			.scale      = {1.0f, 1.0f, 1.0f},
			.body       = &dynamic_body_def,
			.shape      = &crate_shape_def,
		},
		[9] = {
			.model_path = "rom:/models/green_box.t3dm",
			.position   = {350.0f, -50.0f, 150.0f},
			.scale      = {1.0f, 1.0f, 1.0f},
			.body       = &dynamic_body_def,
			.shape      = &crate_shape_def,
		},
		[10] = {
			.model_path = "rom:/models/red_box.t3dm",
			.position   = {350.0f, 50.0f, 150.0f},
			.scale      = {1.0f, 1.0f, 1.0f},
			.body       = &dynamic_body_def,
			.shape      = &crate_shape_def,
		},

		[11] = {
			.model_path = "rom:/models/red_sphere.t3dm",
			.position   = {200.0f, -300.0f, 25.0f},
			.scale      = {0.5f, 0.5f, 0.5f},
			.body       = &dynamic_body_def,
			.shape      = &ball_shape_def,
		},
		[12] = {
			.model_path = "rom:/models/yellow_sphere.t3dm",
			.position   = {-300.0f, 200.0f, 25.0f},
			.scale      = {0.5f, 0.5f, 0.5f},
			.body       = &dynamic_body_def,
			.shape      = &ball_shape_def,
		},
	},
	.entity_count = 13,
};
