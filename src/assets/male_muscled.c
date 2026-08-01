#include "character/character.h"
#include "assets/male_muscled.h"


// --- Clip definitions ---

static const CharacterAnimationClipDef male_muscled_clips[] = {
	 
	[MM_ANIM_IDLE_L]            = { "standing-idle-left",                 ANIMATION_SLOT_MAIN,   true  },
	[MM_ANIM_IDLE_R]            = { "standing-idle-right",                MM_SLOT_IDLE_R,        true  },

	[MM_ANIM_STAND_TO_WALK_L]   = { "standing-to-walking-left",           MM_SLOT_WALK,          false },
	[MM_ANIM_STAND_TO_WALK_R]   = { "standing-to-walking-right",          MM_SLOT_WALK,          false },

	[MM_ANIM_STAND_TO_RUN_L]    = { "standing-to-running-left",           MM_SLOT_RUN,           false },
	[MM_ANIM_STAND_TO_RUN_R]    = { "standing-to-running-right",          MM_SLOT_RUN,           false },

	[MM_ANIM_WALK]              = { "walking",                            MM_SLOT_WALK,          true  },
	[MM_ANIM_TURN_WALK_L]       = { "walking-turn-left",                  MM_SLOT_TURN_WALK,     true  },
	[MM_ANIM_TURN_WALK_R]       = { "walking-turn-right",                 MM_SLOT_TURN_WALK,     true  },
	[MM_ANIM_WALK_CHANGE_DIR_L] = { "walking-to-change-direction-left",   MM_SLOT_WALK,          false },
	[MM_ANIM_WALK_CHANGE_DIR_R] = { "walking-to-change-direction-right",  MM_SLOT_WALK,          false },
	[MM_ANIM_WALK_TO_STAND_L]   = { "walking-to-standing-left",           MM_SLOT_WALK,          false },
	[MM_ANIM_WALK_TO_STAND_R]   = { "walking-to-standing-right",          MM_SLOT_WALK,          false },

	[MM_ANIM_WALK_BACK]         = { "walking-backwards",                  MM_SLOT_WALK,          true  },
	[MM_ANIM_WALK_BACK_L]       = { "walking-backwards-left",             MM_SLOT_WALK,          true  },
	[MM_ANIM_WALK_BACK_R]       = { "walking-backwards-right",            MM_SLOT_WALK,          true  },
	[MM_ANIM_WALK_STRAFE_L]     = { "walking-strafe-left",                MM_SLOT_WALK,          true  },
	[MM_ANIM_WALK_STRAFE_R]     = { "walking-strafe-right",               MM_SLOT_WALK,          true  },

	[MM_ANIM_RUN]               = { "running",                            MM_SLOT_RUN,           true  },
	[MM_ANIM_TURN_RUN_L]        = { "running-turn-left",                  MM_SLOT_TURN_RUN,      true  },
	[MM_ANIM_TURN_RUN_R]        = { "running-turn-right",                 MM_SLOT_TURN_RUN,      true  },
	[MM_ANIM_RUN_CHANGE_DIR_L]  = { "running-to-change-direction-left",   MM_SLOT_RUN,           false },
	[MM_ANIM_RUN_CHANGE_DIR_R]  = { "running-to-change-direction-right",  MM_SLOT_RUN,           false },
	[MM_ANIM_RUN_TO_STAND_L]    = { "running-to-standing-left",           MM_SLOT_RUN,           false },
	[MM_ANIM_RUN_TO_STAND_R]    = { "running-to-standing-right",          MM_SLOT_RUN,           false },

	[MM_ANIM_RUN_BACK]          = { "running-backwards",                  MM_SLOT_RUN,           true  },
	[MM_ANIM_RUN_BACK_L]        = { "running-backwards-left",             MM_SLOT_RUN,           true  },
	[MM_ANIM_RUN_BACK_R]        = { "running-backwards-right",            MM_SLOT_RUN,           true  },
	[MM_ANIM_RUN_STRAFE_L]      = { "running-strafe-left",                MM_SLOT_RUN,           true  },
	[MM_ANIM_RUN_STRAFE_R]      = { "running-strafe-right",               MM_SLOT_RUN,           true  },

	[MM_ANIM_SPRINT]            = { "sprinting",                          MM_SLOT_SPRINT,        true  },

	[MM_ANIM_JUMP_L]            = { "jump-left",                          MM_SLOT_JUMP_L,        false },
	[MM_ANIM_JUMP_R]            = { "jump-right",                         MM_SLOT_JUMP_R,        false },
	[MM_ANIM_FALL_L]            = { "falling-idle-left",                  MM_SLOT_JUMP_L,        true  },
	[MM_ANIM_FALL_R]            = { "falling-idle-right",                 MM_SLOT_JUMP_R,        true  },
	[MM_ANIM_LAND_L]            = { "land-left",                          MM_SLOT_LAND_L,        false },
	[MM_ANIM_LAND_R]            = { "land-right",                         MM_SLOT_LAND_R,        false },

	[MM_ANIM_ROLL_L]            = { "running-to-roll-left",               MM_SLOT_ROLL_RUN,      false },
	[MM_ANIM_ROLL_R]            = { "running-to-roll-right",              MM_SLOT_ROLL_RUN,      false },

	[MM_ANIM_SLIDE_L]           = { "slide-left",                         MM_SLOT_ROLL_RUN,      false },
	[MM_ANIM_SLIDE_R]           = { "slide-right",                        MM_SLOT_ROLL_RUN,      false },

	[MM_ANIM_TRANSITION_L]      = { "transition-left",                    MM_SLOT_TRANSITION,    false },
	[MM_ANIM_TRANSITION_R]      = { "transition-right",                   MM_SLOT_TRANSITION,    false },

};

static const CharacterAnimationNode male_muscled_nodes[] = {

	{ ANIMATION_NODE_CLIP,   MM_ANIM_IDLE_L,      0,                    0,               0                   },
	    
	{ ANIMATION_NODE_BLEND,  MM_ANIM_IDLE_R,      0,                     MM_SLOT_IDLE_R,  ANIMATION_PARAM_IDLE_RIGHT  },
	{ ANIMATION_NODE_BLEND,  MM_ANIM_WALK,        0,                     MM_SLOT_WALK,    ANIMATION_PARAM_WALK        },
	{ ANIMATION_NODE_BLEND,  MM_ANIM_RUN,         0,                     MM_SLOT_RUN,     ANIMATION_PARAM_RUN         },
	{ ANIMATION_NODE_BLEND,  MM_ANIM_SPRINT,      0,                     MM_SLOT_SPRINT,  ANIMATION_PARAM_SPRINT      },
 
	{ ANIMATION_NODE_SELECT, MM_ANIM_TURN_WALK_L, MM_ANIM_TURN_WALK_R,   MM_SLOT_TURN_WALK, ANIMATION_PARAM_TURN_WALK },
	{ ANIMATION_NODE_LAYER,  0,                   0,                     MM_SLOT_TURN_WALK, ANIMATION_PARAM_TURN_WALK },
	{ ANIMATION_NODE_SELECT, MM_ANIM_TURN_RUN_L,  MM_ANIM_TURN_RUN_R,    MM_SLOT_TURN_RUN,  ANIMATION_PARAM_TURN_RUN  },
	{ ANIMATION_NODE_LAYER,  0,                   0,                     MM_SLOT_TURN_RUN,  ANIMATION_PARAM_TURN_RUN  },

	{ ANIMATION_NODE_SEQUENCE, MM_ANIM_JUMP_L,    MM_ANIM_FALL_L,        MM_SLOT_JUMP_L,  ANIMATION_PARAM_JUMP_L      },
	{ ANIMATION_NODE_SEQUENCE, MM_ANIM_JUMP_R,    MM_ANIM_FALL_R,        MM_SLOT_JUMP_R,  ANIMATION_PARAM_JUMP_R      },
	{ ANIMATION_NODE_LAYER,    0,                 0,                     MM_SLOT_JUMP_L,  ANIMATION_PARAM_JUMP_L      },
	{ ANIMATION_NODE_LAYER,    0,                 0,                     MM_SLOT_JUMP_R,  ANIMATION_PARAM_JUMP_R      },
    
	{ ANIMATION_NODE_BLEND,  MM_ANIM_LAND_L,      0,                     MM_SLOT_LAND_L,  ANIMATION_PARAM_LAND_L      },
	{ ANIMATION_NODE_BLEND,  MM_ANIM_LAND_R,      0,                     MM_SLOT_LAND_R,  ANIMATION_PARAM_LAND_R      },

	{ ANIMATION_NODE_SELECT, MM_ANIM_ROLL_L,       MM_ANIM_ROLL_R,       MM_SLOT_ROLL_RUN,   ANIMATION_PARAM_ROLL_DIR       },
	{ ANIMATION_NODE_LAYER,  0,                    0,                    MM_SLOT_ROLL_RUN,   ANIMATION_PARAM_ROLL_RUN       },
};

_Static_assert(MM_ANIM_TURN_WALK_R == MM_ANIM_TURN_WALK_L + 1, "turn_walk L/R must be contiguous (character_animation reads L+1)");
_Static_assert(MM_ANIM_TURN_RUN_R  == MM_ANIM_TURN_RUN_L  + 1, "turn_run L/R must be contiguous (character_animation reads L+1)");
_Static_assert(MM_ANIM_JUMP_R      == MM_ANIM_JUMP_L      + 1, "jump L/R must be contiguous (character_animation reads L+1)");
_Static_assert(MM_ANIM_FALL_R      == MM_ANIM_FALL_L      + 1, "fall L/R must be contiguous (character_animation reads L+1)");
_Static_assert(MM_ANIM_LAND_R      == MM_ANIM_LAND_L      + 1, "land L/R must be contiguous (character_animation reads L+1)");

const CharacterAnimationDef male_muscled_animation_def = {

	.clip            = male_muscled_clips,
	.node            = male_muscled_nodes,
	.settings        = &male_muscled_animation_settings,
	.clip_count      = MM_ANIM_COUNT,
	.node_count      = sizeof(male_muscled_nodes) / sizeof(male_muscled_nodes[0]),
	.buffer_count    = MM_SLOT_COUNT,
	.walk_animation       = MM_ANIM_WALK,
	.run_animation        = MM_ANIM_RUN,
	.sprint_animation     = MM_ANIM_SPRINT,
	.turn_walk_animation  = MM_ANIM_TURN_WALK_L,
	.turn_run_animation   = MM_ANIM_TURN_RUN_L,
	.jump_animation       = MM_ANIM_JUMP_L,
	.fall_animation       = MM_ANIM_FALL_L,
	.land_animation       = MM_ANIM_LAND_L,
	.roll_animation       = MM_ANIM_ROLL_L,
	
};

const CharacterMovementSettings male_muscled_movement_settings = {

	.idle_target_speed            = 0,
	.idle_acceleration_rate           = 10.0f,
	.idle_rotation_acceleration_rate  = 8.0f,

	.walk_target_speed            = 1.75f,
	.walk_acceleration_rate           = 8.0f,
	.walk_rotation_acceleration_rate  = 12.0f,

	.run_target_speed             = 3.9f,
	.run_acceleration_rate            = 9.0f,
	.run_rotation_acceleration_rate   = 15.0f,

	.sprint_target_speed           = 5.2f,
	.sprint_acceleration_rate          = 10.0f,
	.sprint_rotation_acceleration_rate = 14.0f,

	.roll_launch_acceleration_rate = 15.0f,
	.roll_spin_acceleration_rate   = 5.0f,
	.roll_grip_acceleration_rate   = 2.0f,
	.roll_ground_time          = 0.3f,
	.roll_grip_time            = 0.9f,
	.roll_timer_max            = 1.166666f,
	.roll_target_speed_walk    = 3.0f,
	.roll_target_speed_run     = 4.68f,
	.roll_target_speed_sprint  = 6.24f,

	.jump_acceleration_rate    = 0.5f,
	.jump_force_multiplier = 30.0f,
	.jump_minimum_speed    = 5.0f,
	.jump_timer_max        = 0.233333f,

};

const CharacterColliderSettings male_muscled_collider_settings = {

	.radius = 0.35f,
	.height = 1.8f,

};

const CharacterAnimationSettings male_muscled_animation_settings = {

		.action_idle_max_blending_ratio = 0.85f,

		.run_to_walk_ratio              = 1.347826f,
		.walk_to_run_ratio              = 0.741935f,

		.sprint_to_run_ratio            = 1.15f,
		.run_to_sprint_ratio            = 0.869565f,

		.sprint_to_walk_ratio           = 1.55f,
		.walk_to_sprint_ratio           = 0.645161f,

		.walking_anim_length            = 1.033333f,
		.walking_anim_length_half       = 0.516666f,

		.running_anim_length            = 0.766666f,
		.running_anim_length_half       = 0.4f,

		.sprinting_anim_length          = 0.666666f,
		.sprinting_anim_length_half     = 0.333333f,

		.jump_max_blending_ratio = 0.55f,

		.jump_anim_length        = 0.633333f,
		.jump_anim_crouch        = 0.1f,
		.jump_anim_air           = 0.233333f,
		.jump_footing_speed      = 0.4f,

		.land_anim_length        = 0.9f,
		.land_anim_crouch        = 0.5f,
		.land_anim_ground        = 0.266666f,
		.land_anim_stand         = 0.833333f,

		.run_to_rolling_anim_ground   = 0.3f,
		.run_to_rolling_anim_grip     = 0.9f,
		.run_to_rolling_anim_stand    = 0.9f,
		.run_to_rolling_anim_length   = 1.166666f,

};

