#include "actor/actor.h"
#include "assets/male_muscled.h"


// --- Clip definitions ---

static const AnimationClipDef male_muscled_clips[] = {
	 
	[MM_ANIM_IDLE_L]            = { "standing-idle-left",                 ANIMATION_SLOT_MAIN,        true  },
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
	[MM_ANIM_STAND_ROLL_L]      = { "standing-to-roll-left",              MM_SLOT_ROLL_STAND,    false },
	[MM_ANIM_STAND_ROLL_R]      = { "standing-to-roll-right",             MM_SLOT_ROLL_STAND,    false },

	[MM_ANIM_SLIDE_L]           = { "slide-left",                         MM_SLOT_ROLL_RUN,      false },
	[MM_ANIM_SLIDE_R]           = { "slide-right",                        MM_SLOT_ROLL_RUN,      false },

	[MM_ANIM_TRANSITION_L]      = { "transition-left",                    MM_SLOT_TRANSITION,    false },
	[MM_ANIM_TRANSITION_R]      = { "transition-right",                   MM_SLOT_TRANSITION,    false },

};


// --- Node graph ---

static const AnimationNode male_muscled_nodes[] = {

	// base pose — always running on main
	{ ANIMATION_NODE_CLIP,   MM_ANIM_IDLE_L,      0,                    0,               0                   },
	 
	// locomotion layers     
	{ ANIMATION_NODE_BLEND,  MM_ANIM_IDLE_R,      0,                     MM_SLOT_IDLE_R,  ANIMATION_PARAM_IDLE_RIGHT  },
	{ ANIMATION_NODE_BLEND,  MM_ANIM_WALK,        0,                     MM_SLOT_WALK,    ANIMATION_PARAM_WALK        },
	{ ANIMATION_NODE_BLEND,  MM_ANIM_RUN,         0,                     MM_SLOT_RUN,     ANIMATION_PARAM_RUN         },
	{ ANIMATION_NODE_BLEND,  MM_ANIM_SPRINT,      0,                     MM_SLOT_SPRINT,  ANIMATION_PARAM_SPRINT      },
 
	// turn layers 
	{ ANIMATION_NODE_SELECT, MM_ANIM_TURN_WALK_L, MM_ANIM_TURN_WALK_R,   MM_SLOT_TURN_WALK, ANIMATION_PARAM_TURN_WALK },
	{ ANIMATION_NODE_LAYER,  0,                   0,                     MM_SLOT_TURN_WALK, ANIMATION_PARAM_TURN_WALK },
	{ ANIMATION_NODE_SELECT, MM_ANIM_TURN_RUN_L,  MM_ANIM_TURN_RUN_R,    MM_SLOT_TURN_RUN,  ANIMATION_PARAM_TURN_RUN  },
	{ ANIMATION_NODE_LAYER,  0,                   0,                     MM_SLOT_TURN_RUN,  ANIMATION_PARAM_TURN_RUN  },

	// jump layers — SEQUENCE avanza el clip, LAYER solo blendea el buffer resultante
	{ ANIMATION_NODE_SEQUENCE, MM_ANIM_JUMP_L,    MM_ANIM_FALL_L,        MM_SLOT_JUMP_L,  ANIMATION_PARAM_JUMP_L      },
	{ ANIMATION_NODE_SEQUENCE, MM_ANIM_JUMP_R,    MM_ANIM_FALL_R,        MM_SLOT_JUMP_R,  ANIMATION_PARAM_JUMP_R      },
	{ ANIMATION_NODE_LAYER,    0,                 0,                     MM_SLOT_JUMP_L,  ANIMATION_PARAM_JUMP_L      },
	{ ANIMATION_NODE_LAYER,    0,                 0,                     MM_SLOT_JUMP_R,  ANIMATION_PARAM_JUMP_R      },
	  
	// land layers      
	{ ANIMATION_NODE_BLEND,  MM_ANIM_LAND_L,      0,                     MM_SLOT_LAND_L,  ANIMATION_PARAM_LAND_L      },
	{ ANIMATION_NODE_BLEND,  MM_ANIM_LAND_R,      0,                     MM_SLOT_LAND_R,  ANIMATION_PARAM_LAND_R      },

	// roll layers
	{ ANIMATION_NODE_SELECT, MM_ANIM_ROLL_L,       MM_ANIM_ROLL_R,       MM_SLOT_ROLL_RUN,   ANIMATION_PARAM_ROLL_DIR       },
	{ ANIMATION_NODE_LAYER,  0,                    0,                    MM_SLOT_ROLL_RUN,   ANIMATION_PARAM_ROLL_RUN       },
	{ ANIMATION_NODE_SELECT, MM_ANIM_STAND_ROLL_L, MM_ANIM_STAND_ROLL_R, MM_SLOT_ROLL_STAND, ANIMATION_PARAM_STAND_ROLL_DIR },
	{ ANIMATION_NODE_LAYER,  0,                    0,                    MM_SLOT_ROLL_STAND, ANIMATION_PARAM_ROLL_STAND     },
};

_Static_assert(MM_ANIM_TURN_WALK_R == MM_ANIM_TURN_WALK_L + 1, "turn_walk L/R must be contiguous (actor_animation reads L+1)");
_Static_assert(MM_ANIM_TURN_RUN_R  == MM_ANIM_TURN_RUN_L  + 1, "turn_run L/R must be contiguous (actor_animation reads L+1)");
_Static_assert(MM_ANIM_JUMP_R      == MM_ANIM_JUMP_L      + 1, "jump L/R must be contiguous (actor_animation reads L+1)");
_Static_assert(MM_ANIM_FALL_R      == MM_ANIM_FALL_L      + 1, "fall L/R must be contiguous (actor_animation reads L+1)");
_Static_assert(MM_ANIM_LAND_R      == MM_ANIM_LAND_L      + 1, "land L/R must be contiguous (actor_animation reads L+1)");

const AnimationDef male_muscled_animation_def = {

	.clip            = male_muscled_clips,
	.node            = male_muscled_nodes,
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
	.stand_roll_animation = MM_ANIM_STAND_ROLL_L,
	
};


const ActorMotionSettings male_muscled_motion_settings = {

	.locomotion[LOCOMOTION_IDLE]   = { .target_speed = 0,     .response_rate = 6.0f,  .rotation_response_rate = 8.0f },
	.locomotion[LOCOMOTION_WALK]   = { .target_speed = 1.75f, .response_rate = 8.0f,  .rotation_response_rate = 12.0f },
	.locomotion[LOCOMOTION_RUN]    = { .target_speed = 3.9f,  .response_rate = 9.0f,  .rotation_response_rate = 15.0f },
	.locomotion[LOCOMOTION_SPRINT] = { .target_speed = 5.2f,  .response_rate = 10.0f, .rotation_response_rate = 14.0f },

	.roll = {
		.launch_response_rate            = 15.0f,
		.spin_response_rate              = 5.0f,
		.grip_response_rate              = 2.0f,
		.ground_time                     = 0.3f,
		.grip_time                       = 0.9f,
		.timer_max                       = 1.166666f,
		.target_speed[LOCOMOTION_IDLE]   = 3.0f,
		.target_speed[LOCOMOTION_WALK]   = 3.0f,
		.target_speed[LOCOMOTION_RUN]    = 4.68f,
		.target_speed[LOCOMOTION_SPRINT] = 6.24f,
	},

	.jump = {
		.response_rate    = 0.5f,
		.force_multiplier = 30.0f,
		.minimum_speed    = 5.0f,
		.timer_max        = 0.233333f,
	},

};

const ActorAnimationSettings male_muscled_animation_settings = {

	.standing_locomotion = {

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
	},

	.jump = {

		.jump_max_blending_ratio = 0.55f,

		.jump_anim_length        = 0.633333f,
		.jump_anim_crouch        = 0.1f,
		.jump_anim_air           = 0.233333f,
		.jump_footing_speed      = 0.4f,

		.land_anim_length        = 0.9f,
		.land_anim_crouch        = 0.5f,
		.land_anim_ground        = 0.266666f,
		.land_anim_stand         = 0.833333f,
	},

	.roll = {

		.run_to_rolling_anim_ground   = 0.3f,
		.run_to_rolling_anim_grip     = 0.9f,
		.run_to_rolling_anim_stand    = 0.9f,
		.run_to_rolling_anim_length   = 1.166666f,

		.stand_to_rolling_anim_ground = 0.533333f,
		.stand_to_rolling_anim_grip   = 1.166666f,
		.stand_to_rolling_anim_stand  = 1.166666f,
		.stand_to_rolling_anim_length = 1.5f,
	},
};

