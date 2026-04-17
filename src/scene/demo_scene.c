#include "scene/scene.h"
#include "scene/demo_scene.h"
#include "assets/male_muscled.h"


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

	.actor = {
		[0] = {
			.model_path         = male_muscled_model,
			.position           = {-210.0f, -210.0f, 0.0f},
			.rotation           = {0.0f, 0.0f, 200.0f},
			.scale              = {1.0f, 1.0f, 1.0f},
			.motion_settings    = &male_muscled_motion_settings,
			.animation_settings = &male_muscled_animation_settings,
			.animation_def      = &male_muscled_animation_def,
		},
	},
	.actor_count = 1,

	.scenery = {
		[0] = { "rom:/models/room.t3dm",  {0.0f, 0.0f, -2.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f} },
		[1] = { "rom:/models/axis.t3dm",  {0.0f, 0.0f, -2.0f}, {0.0f, 0.0f, 0.0f}, {1.5f, 1.5f, 1.5f} },
	},
	.scenery_count = 2,
};
