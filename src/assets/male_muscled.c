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

	[MM_ANIM_WALK_BACK]         = { "walking-backwards",                  MM_SLOT_STRAFE_WALK,   true  },
	[MM_ANIM_WALK_BACK_L]       = { "walking-backwards-left",             MM_SLOT_WALK,          true  },
	[MM_ANIM_WALK_BACK_R]       = { "walking-backwards-right",            MM_SLOT_WALK,          true  },
	[MM_ANIM_WALK_STRAFE_L]     = { "walking-strafe-left",                MM_SLOT_STRAFE_WALK,   true  },
	[MM_ANIM_WALK_STRAFE_R]     = { "walking-strafe-right",               MM_SLOT_STRAFE_WALK,   true  },

	[MM_ANIM_RUN]               = { "running",                            MM_SLOT_RUN,           true  },
	[MM_ANIM_TURN_RUN_L]        = { "running-turn-left",                  MM_SLOT_TURN_RUN,      true  },
	[MM_ANIM_TURN_RUN_R]        = { "running-turn-right",                 MM_SLOT_TURN_RUN,      true  },
	[MM_ANIM_RUN_CHANGE_DIR_L]  = { "running-to-change-direction-left",   MM_SLOT_RUN,           false },
	[MM_ANIM_RUN_CHANGE_DIR_R]  = { "running-to-change-direction-right",  MM_SLOT_RUN,           false },
	[MM_ANIM_RUN_TO_STAND_L]    = { "running-to-standing-left",           MM_SLOT_RUN,           false },
	[MM_ANIM_RUN_TO_STAND_R]    = { "running-to-standing-right",          MM_SLOT_RUN,           false },

	[MM_ANIM_RUN_BACK]          = { "running-backwards",                  MM_SLOT_STRAFE_RUN,    true  },
	[MM_ANIM_RUN_BACK_L]        = { "running-backwards-left",             MM_SLOT_RUN,           true  },
	[MM_ANIM_RUN_BACK_R]        = { "running-backwards-right",            MM_SLOT_RUN,           true  },
	[MM_ANIM_RUN_STRAFE_L]      = { "running-strafe-left",                MM_SLOT_STRAFE_RUN,    true  },
	[MM_ANIM_RUN_STRAFE_R]      = { "running-strafe-right",               MM_SLOT_STRAFE_RUN,    true  },

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

	[MM_NODE_IDLE]         = { ANIMATION_NODE_CLIP,     ANIMATION_CLIPS(MM_ANIM_IDLE_L),                   1, 1, 0,               0                          },
	[MM_NODE_IDLE_R]       = { ANIMATION_NODE_BLEND,    ANIMATION_CLIPS(MM_ANIM_IDLE_R),                   1, 1, MM_SLOT_IDLE_R,  ANIMATION_PARAM_IDLE_RIGHT },

	[MM_NODE_LOCOMOTION]   = { ANIMATION_NODE_BLEND_2D,
	  ANIMATION_CLIPS(MM_ANIM_TURN_WALK_L, MM_ANIM_WALK,   MM_ANIM_TURN_WALK_R,
	                  MM_ANIM_TURN_RUN_L,  MM_ANIM_RUN,    MM_ANIM_TURN_RUN_R,
	                  MM_ANIM_TURN_RUN_L,  MM_ANIM_SPRINT, MM_ANIM_TURN_RUN_R),
	  3, 3, 0, ANIMATION_PARAM_WALK_TURN, ANIMATION_PARAM_WALK_GAIT, ANIMATION_PARAM_WALK },

	[MM_NODE_JUMP_L]       = { ANIMATION_NODE_SEQUENCE, ANIMATION_CLIPS(MM_ANIM_JUMP_L, MM_ANIM_FALL_L),   2, 1, MM_SLOT_JUMP_L,  ANIMATION_PARAM_JUMP_L     },
	[MM_NODE_JUMP_R]       = { ANIMATION_NODE_SEQUENCE, ANIMATION_CLIPS(MM_ANIM_JUMP_R, MM_ANIM_FALL_R),   2, 1, MM_SLOT_JUMP_R,  ANIMATION_PARAM_JUMP_R     },
	[MM_NODE_JUMP_L_LAYER] = { ANIMATION_NODE_LAYER,    NULL,                                              0, 0, MM_SLOT_JUMP_L,  ANIMATION_PARAM_JUMP_L     },
	[MM_NODE_JUMP_R_LAYER] = { ANIMATION_NODE_LAYER,    NULL,                                              0, 0, MM_SLOT_JUMP_R,  ANIMATION_PARAM_JUMP_R     },

	[MM_NODE_LAND_L]       = { ANIMATION_NODE_BLEND,    ANIMATION_CLIPS(MM_ANIM_LAND_L),                   1, 1, MM_SLOT_LAND_L,  ANIMATION_PARAM_LAND_L     },
	[MM_NODE_LAND_R]       = { ANIMATION_NODE_BLEND,    ANIMATION_CLIPS(MM_ANIM_LAND_R),                   1, 1, MM_SLOT_LAND_R,  ANIMATION_PARAM_LAND_R     },

	[MM_NODE_ROLL]         = { ANIMATION_NODE_SELECT,   ANIMATION_CLIPS(MM_ANIM_ROLL_L, MM_ANIM_ROLL_R),   2, 1, MM_SLOT_ROLL_RUN, ANIMATION_PARAM_ROLL_DIR  },
	[MM_NODE_ROLL_LAYER]   = { ANIMATION_NODE_LAYER,    NULL,                                              0, 0, MM_SLOT_ROLL_RUN, ANIMATION_PARAM_ROLL_RUN  },

	[MM_NODE_STRAFE]       = { ANIMATION_NODE_BLEND_2D,
	  ANIMATION_CLIPS(MM_ANIM_WALK_BACK, MM_ANIM_WALK_BACK_L, MM_ANIM_WALK_STRAFE_L, MM_ANIM_WALK, MM_ANIM_WALK_STRAFE_R, MM_ANIM_WALK_BACK_R, MM_ANIM_WALK_BACK,
	                  MM_ANIM_RUN_BACK,  MM_ANIM_RUN_BACK_L,  MM_ANIM_RUN_STRAFE_L,  MM_ANIM_RUN,  MM_ANIM_RUN_STRAFE_R,  MM_ANIM_RUN_BACK_R,  MM_ANIM_RUN_BACK),
	  7, 2, 0, ANIMATION_PARAM_STRAFE_DIR, ANIMATION_PARAM_STRAFE_GAIT, ANIMATION_PARAM_STRAFE },
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
	.locomotion_node      = MM_NODE_LOCOMOTION,
	.strafe_node          = MM_NODE_STRAFE,

};

static const CharacterGaitSettings male_muscled_gaits[] = {

	{ .target_speed = 1.55f, .response_rate =  8.0f, .rotation_response_rate = 12.0f },
	{ .target_speed = 3.2f,  .response_rate =  9.0f, .rotation_response_rate = 15.0f },
	{ .target_speed = 4.4f,  .response_rate = 10.0f, .rotation_response_rate = 14.0f },

};

const CharacterMovementSettings male_muscled_movement_settings = {

	.idle_target_speed            = 0,
	.idle_response_rate           = 10.0f,
	.idle_rotation_response_rate  = 8.0f,

	.gait       = male_muscled_gaits,
	.gait_count = sizeof(male_muscled_gaits) / sizeof(male_muscled_gaits[0]),

	.roll_target_speed         = 4.6f,
	.roll_launch_response_rate = 15.0f,
	.roll_spin_response_rate   = 5.0f,
	.roll_grip_response_rate   = 2.0f,
	.roll_ground_time          = 0.3f,
	.roll_grip_time            = 0.9f,
	.roll_timer_max            = 1.166666f,

	.jump_response_rate    = 0.5f,
	.jump_force_multiplier = 30.0f,
	.jump_minimum_speed    = 4.4f,
	.jump_timer_max        = 0.233333f,

};

const CharacterColliderSettings male_muscled_collider_settings = {

	.radius = 0.35f,
	.height = 1.8f,

};


// --- Weapons ---

const char *const male_muscled_weapon_meshes[] = { "ak47", "knife", "m1911" };

const CharacterWeaponsDef male_muscled_weapons_def = {
	.mesh       = male_muscled_weapon_meshes,
	.mesh_count = 3,
	/* Off for the demo capture. Restore these to carry the loadout again. */
	.weapon = {
		//[WEAPON_SLOT_WAIST] = &weapon_m1911,
		//[WEAPON_SLOT_BACK]  = &weapon_ak47,
		//[WEAPON_SLOT_MELEE] = &weapon_knife,
	},
};

const WeaponDef weapon_ak47 = {
	.mesh          = "ak47",
	.bone          = "rifle",
	.holster_bone  = "mixamorig:Spine2",
	.hand_bone     = "mixamorig:RightHand",
	.type          = WEAPON_TYPE_RIFLE,
	.magazine_size = 30,
	.max_integrity = 100,
	.holster_position = {{ -11.0f, 6.35f, -16.19f }},
	.holster_rotation = {{ -0.6255f, -0.1841f, -0.7357f, 0.1833f }},
	.holding_rotation = {{ 0.0f, 0.0f, 0.0f, 1.0f }},
};

const WeaponDef weapon_m1911 = {
	.mesh          = "m1911",
	.bone          = "handgun",
	.holster_bone  = "mixamorig:Hips",
	.hand_bone     = "mixamorig:RightHand",
	.type          = WEAPON_TYPE_HANDGUN,
	.magazine_size = 7,
	.max_integrity = 100,
	.holster_position = {{ -22.0f, -3.93f, 0.2f }},
	.holster_rotation = {{ 0.9995f, 0.0f, 0.0f, 0.0316f }},
	.holding_rotation = {{ 0.0f, 0.0f, 0.0f, 1.0f }},
};

const WeaponDef weapon_knife = {
	.mesh          = "knife",
	.bone          = "melee-weapon",
	.holster_bone  = "mixamorig:LeftUpLeg",
	.hand_bone     = "mixamorig:RightHand",
	.type          = WEAPON_TYPE_MELEE,
	.magazine_size = 0,
	.max_integrity = 100,
	.holster_position = {{ -8.74f, -8.37f, 0.64f }},
	.holster_rotation = {{ 0.013f, 0.9991f, -0.0389f, 0.0066f }},
	.holding_rotation = {{ 0.0f, 0.0f, 0.0f, 1.0f }},
};

const CharacterAnimationSettings male_muscled_animation_settings = {

		.action_idle_max_blending_ratio = 0.85f,

		.turn_max_angle  = 5.0f,
		.turn_max_weight = 0.3f,

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

		.strafe_turn_rate        = 8.0f,
		.strafe_blend_rate       = 2.0f,

};



const CharacterDef male_muscled_character_def = {

	.movement_settings  = &male_muscled_movement_settings,
	.animation_def      = &male_muscled_animation_def,
	.collider_settings  = &male_muscled_collider_settings,
	.weapons_def        = &male_muscled_weapons_def,

};
