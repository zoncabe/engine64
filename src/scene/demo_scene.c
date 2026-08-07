#include "scene/scene.h"
#include "scene/demo_scene.h"
#include "assets/male_muscled.h"




/* --- Static collider shapes for the test props (metres). --- */

/* The level itself: a triangle mesh, so it only ever hangs off a static body.
   Friction and restitution here are what everything else slides and lands on. */
static const PhysicsShapeDef room_shapes[] = {
	{ .type = SHAPE_MESH, .mesh = {
		.path        = "rom:/collision/room.collision",
		.friction    = 0.9f,
		.restitution = 0.1f,
	}},
};

/* The prop models are 1 m across, so the shapes are declared at that size and
   the entity scale sets both the visual and the collider. Density is what the
   solver turns into mass, so each one is the target weight over the volume the
   shape ends up with: a 1 m box is 1 m3, so 5 kg/m3 makes it 5 kg. */
static const PhysicsShapeDef crate_shapes[] = {
	{ .type = SHAPE_BOX, .box = {
		.e = { 0.5f, 0.5f, 0.5f },
		.density = 5.0f, .friction = 0.5f, .restitution = 0.0f,
	}},
};

/* The three ball sizes. Radius comes from the entity scale, so what changes
   here is only the density that lands each one on its weight. */
static const PhysicsShapeDef ball_small_shapes[] = {     /* 50 cm -> 1 kg */
	{ .type = SHAPE_SPHERE, .sphere = {
		.radius = 0.5f,
		.density = 15.3f, .friction = 0.4f, .restitution = 0.2f,
	}},
};

static const PhysicsShapeDef ball_medium_shapes[] = {    /* 66 cm -> 2 kg */
	{ .type = SHAPE_SPHERE, .sphere = {
		.radius = 0.5f,
		.density = 13.3f, .friction = 0.4f, .restitution = 0.2f,
	}},
};

static const PhysicsShapeDef ball_large_shapes[] = {     /* 1 m -> 4 kg */
	{ .type = SHAPE_SPHERE, .sphere = {
		.radius = 0.5f,
		.density = 7.6f, .friction = 0.4f, .restitution = 0.2f,
	}},
};

/* Pole, measured off the model: a 0.432 m square base plate 0.032 m tall, and
   a 0.084 m mast running from there up to 5.5 m. Shared by the light posts
   and the flagpole, which is the same model stretched on Z. */
static const PhysicsShapeDef pole_shapes[] = {
	{ .type = SHAPE_BOX, .box = {
		.tx = { .position = { 0.0f, 0.0f, 0.016f } },
		.e  = { 0.216f, 0.216f, 0.016f },
	}},
	{ .type = SHAPE_BOX, .box = {
		.tx = { .position = { 0.0f, 0.0f, 2.75f } },
		.e  = { 0.0422f, 0.0422f, 2.75f },
	}},
};

/* The flag is a 17x17 grid spanning x = 0 .. 2.4 m, with columns every 0.15 m.
   Pinning at 0.01 holds only the column at x = 0 — the 17 particles tied to
   the mast — and lets the rest hang. */
static const ClothDef flag_cloth = {
	.mesh_path  = "rom:/collision/brew_flag.collision",
	.damping    = 0.02f,
	.iterations = 4,
	.pin_max_x  = 0.01f,
};

/* Simulated props. Without this def an entity's body comes out static. */
static const RigidBodyDef dynamic_body = {
	.body_type       = BODY_DYNAMIC,
	.gravity_scale   = 1.0f,
	.layers          = 1,
	.allow_sleep     = 1,
	.awake           = 1,
	.active          = 1,
	.angular_damping = 0.1f,
};

static const EntityColliderDef room_collider        = { room_shapes,        1 };
static const EntityColliderDef ball_small_collider  = { ball_small_shapes,  1 };
static const EntityColliderDef ball_medium_collider = { ball_medium_shapes, 1 };
static const EntityColliderDef ball_large_collider  = { ball_large_shapes,  1 };
static const EntityColliderDef crate_collider  = { crate_shapes,  1 };
static const EntityColliderDef pole_collider   = { pole_shapes,   2 };


const SceneDef demo_scene = {

	/* Only the ceiling lamp is lit. Point lights are shaded per vertex per
	   light in the RSP, so every extra one costs the whole scene's vertex
	   count; the corner posts keep their lamp geometry, unlit. */
	/* Both lamps again, pushed further out along the same axis and at the same
	   height, with the radius widened to keep the middle of the room lit. */
	.light = {
		.ambient_color = {10, 10, 10, 0xFF},
		.point = {
			[0] = { .position = {{ 0.0f,  1300.0f, 869.0f }}, .color = {255, 255, 255, 0xFF}, .size = 2500.0f },
			[1] = { .position = {{ 0.0f, -1300.0f, 869.0f }}, .color = {255, 255, 255, 0xFF}, .size = 2500.0f },
		},
	},

	/* Blows across the flag's plane, with a lift term so it rides against
	   gravity instead of only swinging sideways. */
	.wind = { 300.0f, 50.0f, 50.0f },

	.camera = {
		.type = CAMERA_TYPE_SPHERICAL,
		.spherical = {
			.distance_from_center      = 200.0f,
			.distance_center_to_target = 300.0f,
			/* Points the view straight at the flag from the platform: the flag
			   bears -44.9 deg from there, and the offset_angle swings the line
			   of sight about 3 deg past the angle itself. */
			.angle_around_center       = -48.0f,
			.offset_angle              = 30.0f,
			.pitch                     = 8.0f,
			.offset_height             = 130.0f,
		},
	},

	.entity = {
		[0] = {
			/* Centre of the raised square platform, whose top face sits at
			   z = 200 and spans x 1500..2000, y -2000..-1500. */
			.model_path = male_muscled_model,
			.position   = {1750.0f, -1750.0f, 200.0f},
			.rotation   = {0.0f, 0.0f, 125.0f},   /* facing the flag */
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
		/* Flagpole: same model as the light posts, stretched on Z. The collider
		   scales with it, so the mast box grows in step. */
		[2] = {
			.model_path = "rom:/models/pole.t3dm",
			.position   = {0.0f, 0.0f, -2.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {1.0f, 1.0f, 2.0f},
			.collider   = &pole_collider,
		},

		/* Flag, hung off the central pole: 10 cm below its top and 5 cm out
		   on X, as if tied to a rope running down the mast. */
		[3] = {
			.model_path = "rom:/models/brew_flag.t3dm",
			.position   = {5.0f, 0.0f, 688.0f},
			.rotation   = {0.0f, 0.0f, 0.0f},
			.scale      = {2.5f, 1.0f, 2.5f},
			.cloth      = &flag_cloth,
		},

		/* Light posts flanking the flagpole, raised to 1.7x their old height.
		   The lamp's origin is its centre and it is 99 across, so its underside
		   sits at 1019.5 - 49.5 = 970: the mast has to reach exactly that, and
		   550 * 1.764 does. */
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

		/* --- Physics test: 2x2x3 stack of 1 m crates, 5 kg each, on the flat
		   ground between the character and the flagpole. The room's floor is at
		   z = -2, so the first layer's centre sits at 48 and each one above
		   clears the last by a hair. --- */
#define CRATE(i, X, Y, Z) \
		[i] = { \
			.model_path = "rom:/models/green_box.t3dm", \
			.position   = {X, Y, Z}, \
			.rotation   = {0.0f, 0.0f, 0.0f}, \
			.scale      = {1.0f, 1.0f, 1.0f}, \
			.body       = &dynamic_body, \
			.collider   = &crate_collider, \
		}

		CRATE( 8, 1050.0f, 1050.0f,  48.0f),
		CRATE( 9, 1150.0f, 1050.0f,  48.0f),
		CRATE(10, 1050.0f, 1150.0f,  48.0f),
		CRATE(11, 1150.0f, 1150.0f,  48.0f),

		CRATE(12, 1050.0f, 1050.0f, 149.0f),
		CRATE(13, 1150.0f, 1050.0f, 149.0f),
		CRATE(14, 1050.0f, 1150.0f, 149.0f),
		CRATE(15, 1150.0f, 1150.0f, 149.0f),

		CRATE(16, 1050.0f, 1050.0f, 250.0f),
		CRATE(17, 1150.0f, 1050.0f, 250.0f),
		CRATE(18, 1050.0f, 1150.0f, 250.0f),
		CRATE(19, 1150.0f, 1150.0f, 250.0f),
#undef CRATE

		/* --- And a pyramid on the flat ground: four 1 m reds at the bottom,
		   four 66 cm yellows on them, four 50 cm greens on top. Each layer's
		   centre is the one below plus both radii. --- */
#define BALL(i, MODEL, COLLIDER, S, X, Y, Z) \
		[i] = { \
			.model_path = MODEL, \
			.position   = {X, Y, Z}, \
			.rotation   = {0.0f, 0.0f, 0.0f}, \
			.scale      = {S, S, S}, \
			.body       = &dynamic_body, \
			.collider   = COLLIDER, \
		}

		BALL(20, "rom:/models/red_sphere.t3dm", &ball_large_collider, 1.0f, -1150.0f, 1050.0f,  48.0f),
		BALL(21, "rom:/models/red_sphere.t3dm", &ball_large_collider, 1.0f, -1050.0f, 1050.0f,  48.0f),
		BALL(22, "rom:/models/red_sphere.t3dm", &ball_large_collider, 1.0f, -1150.0f, 1150.0f,  48.0f),
		BALL(23, "rom:/models/red_sphere.t3dm", &ball_large_collider, 1.0f, -1050.0f, 1150.0f,  48.0f),

		BALL(24, "rom:/models/yellow_sphere.t3dm", &ball_medium_collider, 0.66f, -1150.0f, 1050.0f, 132.0f),
		BALL(25, "rom:/models/yellow_sphere.t3dm", &ball_medium_collider, 0.66f, -1050.0f, 1050.0f, 132.0f),
		BALL(26, "rom:/models/yellow_sphere.t3dm", &ball_medium_collider, 0.66f, -1150.0f, 1150.0f, 132.0f),
		BALL(27, "rom:/models/yellow_sphere.t3dm", &ball_medium_collider, 0.66f, -1050.0f, 1150.0f, 132.0f),

		BALL(28, "rom:/models/green_sphere.t3dm", &ball_small_collider, 0.5f, -1150.0f, 1050.0f, 191.0f),
		BALL(29, "rom:/models/green_sphere.t3dm", &ball_small_collider, 0.5f, -1050.0f, 1050.0f, 191.0f),
		BALL(30, "rom:/models/green_sphere.t3dm", &ball_small_collider, 0.5f, -1150.0f, 1150.0f, 191.0f),
		BALL(31, "rom:/models/green_sphere.t3dm", &ball_small_collider, 0.5f, -1050.0f, 1150.0f, 191.0f),
#undef BALL
	},
	.entity_count = 32,
};
