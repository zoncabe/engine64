#include "../../include/actor/actor.h"
#include "../../include/assets/male_muscled.h"


// --- Clip definitions ---

static const AnimClipDef male_muscled_clips[] = {
    [MM_ANIM_IDLE_L]      = { "standing-idle-left",    ANIM_SLOT_MAIN,   true  },
    [MM_ANIM_IDLE_R]      = { "standing-idle-right",   MM_SLOT_IDLE_R,   true  },
    [MM_ANIM_WALK]        = { "walking",                MM_SLOT_WALK,     true  },
    [MM_ANIM_RUN]         = { "running",                MM_SLOT_RUN,      true  },
    [MM_ANIM_SPRINT]      = { "sprinting",              MM_SLOT_SPRINT,   true  },
    [MM_ANIM_TURN_WALK_L] = { "walking-turn-left",      MM_SLOT_TURN_WALK,true  },
    [MM_ANIM_TURN_WALK_R] = { "walking-turn-right",     MM_SLOT_TURN_WALK,true  },
    [MM_ANIM_TURN_RUN_L]  = { "running-turn-left",      MM_SLOT_TURN_RUN, true  },
    [MM_ANIM_TURN_RUN_R]  = { "running-turn-right",     MM_SLOT_TURN_RUN, true  },
    [MM_ANIM_JUMP_L]      = { "jump-left",              MM_SLOT_JUMP_L,   false },
    [MM_ANIM_JUMP_R]      = { "jump-right",             MM_SLOT_JUMP_R,   false },
    [MM_ANIM_FALL_L]      = { "falling-idle-left",      MM_SLOT_JUMP_L,   true  },
    [MM_ANIM_FALL_R]      = { "falling-idle-right",     MM_SLOT_JUMP_R,   true  },
    [MM_ANIM_LAND_L]      = { "land-left",              MM_SLOT_LAND_L,   false },
    [MM_ANIM_LAND_R]      = { "land-right",             MM_SLOT_LAND_R,   false },
    [MM_ANIM_ROLL_L]      = { "running-to-roll-left",   MM_SLOT_ROLL,     false },
    [MM_ANIM_ROLL_R]      = { "running-to-roll-right",  MM_SLOT_ROLL,     false },
};


// --- Node graph ---

static const AnimNode male_muscled_nodes[] = {

    // base pose — always running on main
    { ANIM_NODE_CLIP,   MM_ANIM_IDLE_L,      0,               0,               0                   },

    // locomotion layers
    { ANIM_NODE_BLEND,  MM_ANIM_IDLE_R,      0,               MM_SLOT_IDLE_R,  ANIM_PARAM_IDLE_RIGHT  },
    { ANIM_NODE_BLEND,  MM_ANIM_WALK,        0,               MM_SLOT_WALK,    ANIM_PARAM_WALK     },
    { ANIM_NODE_BLEND,  MM_ANIM_RUN,         0,               MM_SLOT_RUN,     ANIM_PARAM_RUN      },
    { ANIM_NODE_BLEND,  MM_ANIM_SPRINT,      0,               MM_SLOT_SPRINT,  ANIM_PARAM_SPRINT   },

    // turn layers
    { ANIM_NODE_SELECT, MM_ANIM_TURN_WALK_L, MM_ANIM_TURN_WALK_R, MM_SLOT_TURN_WALK, ANIM_PARAM_TURN_WALK },
    { ANIM_NODE_SELECT, MM_ANIM_TURN_RUN_L,  MM_ANIM_TURN_RUN_R,  MM_SLOT_TURN_RUN,  ANIM_PARAM_TURN_RUN  },

    // jump layers — SEQUENCE avanza el clip, LAYER solo blendea el buffer resultante
    { ANIM_NODE_SEQUENCE, MM_ANIM_JUMP_L,    MM_ANIM_FALL_L,  MM_SLOT_JUMP_L,  0                  },
    { ANIM_NODE_SEQUENCE, MM_ANIM_JUMP_R,    MM_ANIM_FALL_R,  MM_SLOT_JUMP_R,  0                  },
    { ANIM_NODE_LAYER,    0,                 0,               MM_SLOT_JUMP_L,  ANIM_PARAM_JUMP_L   },
    { ANIM_NODE_LAYER,    0,                 0,               MM_SLOT_JUMP_R,  ANIM_PARAM_JUMP_R   },

    // land layers
    { ANIM_NODE_BLEND,  MM_ANIM_LAND_L,      0,               MM_SLOT_LAND_L,  ANIM_PARAM_LAND_L   },
    { ANIM_NODE_BLEND,  MM_ANIM_LAND_R,      0,               MM_SLOT_LAND_R,  ANIM_PARAM_LAND_R   },

    // roll layer
    { ANIM_NODE_SELECT, MM_ANIM_ROLL_L,      MM_ANIM_ROLL_R,  MM_SLOT_ROLL,    ANIM_PARAM_ROLL     },
    { ANIM_NODE_BLEND,  MM_ANIM_ROLL_L,      0,               MM_SLOT_ROLL,    ANIM_PARAM_ROLL     },
};


const AnimDef male_muscled_anim_def = {
    .clip         = male_muscled_clips,
    .node         = male_muscled_nodes,
    .clip_count   = MM_ANIM_COUNT,
    .node_count   = sizeof(male_muscled_nodes) / sizeof(male_muscled_nodes[0]),
    .buffer_count = MM_SLOT_COUNT,
    .walk_anim       = MM_ANIM_WALK,
    .run_anim        = MM_ANIM_RUN,
    .sprint_anim     = MM_ANIM_SPRINT,
    .turn_walk_anim  = MM_ANIM_TURN_WALK_L,
    .turn_run_anim   = MM_ANIM_TURN_RUN_L,
    .jump_anim       = MM_ANIM_JUMP_L,
    .land_anim       = MM_ANIM_LAND_L,
    .roll_anim       = MM_ANIM_ROLL_L,
};


const ActorMotionSettings male_muscled_motion_settings = {
    
    .idle_acceleration_rate = MALE_MUSCLED_IDLE_ACCELERATION_RATE,
    .walk_acceleration_rate = MALE_MUSCLED_WALK_ACCELERATION_RATE,
    .run_acceleration_rate = MALE_MUSCLED_RUN_ACCELERATION_RATE,
    .sprint_acceleration_rate = MALE_MUSCLED_SPRINT_ACCELERATION_RATE,
    .roll_acceleration_grip_rate = MALE_MUSCLED_ROLL_ACCELERATION_GRIP_RATE,

    .jump_acceleration_rate = MALE_MUSCLED_JUMP_ACCELERATION_RATE,
    .aerial_control_rate = MALE_MUSCLED_AERIAL_CONTROL_RATE,

    .walk_target_speed = 175,
    .run_target_speed = MALE_MUSCLED_RUN_TARGET_SPEED,
    .sprint_target_speed = MALE_MUSCLED_SPRINT_TARGET_SPEED,

    .roll_change_grip_time = MALE_MUSCLED_RUN_TO_ROLLING_ANIM_GRIP,
    .roll_timer_max = MALE_MUSCLED_RUN_TO_ROLLING_ANIM_LENGTH,

    .jump_timer_max = MALE_MUSCLED_JUMP_ANIM_AIR,
    
    .jump_force_multiplier = MALE_MUSCLED_JUMP_FORCE_MULTIPLIER,  
    .jump_minimum_speed = MALE_MUSCLED_JUMP_MINIMUM_SPEED,

    .gravity = MALE_MUSCLED_GRAVITY,
    .fall_max_speed = MALE_MUSCLED_FALL_MAX_SPEED,
};    
    

const ActorAnimationSettings male_muscled_animation_settings = {

    .standing_locomotion = {

        .action_idle_max_blending_ratio = MALE_MUSCLED_ACTION_IDLE_MAX_BLENDING_RATIO,

        .run_to_walk_ratio = MALE_MUSCLED_RUN_TO_WALK_RATIO,
        .walk_to_run_ratio = MALE_MUSCLED_WALK_TO_RUN_RATIO,

        .sprint_to_run_ratio = MALE_MUSCLED_SPRINT_TO_RUN_RATIO,
        .run_to_sprint_ratio = MALE_MUSCLED_RUN_TO_SPRINT_RATIO,

        .sprint_to_walk_ratio = MALE_MUSCLED_SPRINT_TO_WALK_RATIO,
        .walk_to_sprint_ratio = MALE_MUSCLED_WALK_TO_SPRINT_RATIO,

        .walking_anim_length = MALE_MUSCLED_WALKING_ANIM_LENGTH,
        .walking_anim_length_half = MALE_MUSCLED_WALKING_ANIM_LENGTH_HALF,

        .running_anim_length = MALE_MUSCLED_RUNNING_ANIM_LENGTH,
        .running_anim_length_half = MALE_MUSCLED_RUNNING_ANIM_LENGTH_HALF,

        .sprinting_anim_length = MALE_MUSCLED_SPRINTING_ANIM_LENGTH,
        .sprinting_anim_length_half = MALE_MUSCLED_SPRINTING_ANIM_LENGTH_HALF,
    },

    .jump = {

        .jump_max_blending_ratio = MALE_MUSCLED_JUMP_MAX_BLENDING_RATIO,

        .jump_anim_length = MALE_MUSCLED_JUMP_ANIM_LENGTH,
        .jump_anim_crouch = MALE_MUSCLED_JUMP_ANIM_CROUCH,
        .jump_anim_air = MALE_MUSCLED_JUMP_ANIM_AIR,
        .jump_footing_speed = MALE_MUSCLED_JUMP_FOOTING_SPEED,

        .land_anim_length = MALE_MUSCLED_LAND_ANIM_LENGTH,
        .land_anim_crouch = MALE_MUSCLED_LAND_ANIM_CROUCH,
        .land_anim_ground = MALE_MUSCLED_LAND_ANIM_GROUND,
        .land_anim_stand = MALE_MUSCLED_LAND_ANIM_STAND,
    },

    .roll = {

        .run_to_rolling_anim_ground = MALE_MUSCLED_RUN_TO_ROLLING_ANIM_GROUND,
        .run_to_rolling_anim_grip = MALE_MUSCLED_RUN_TO_ROLLING_ANIM_GRIP,
        .run_to_rolling_anim_stand = MALE_MUSCLED_RUN_TO_ROLLING_ANIM_STAND,
        .run_to_rolling_anim_length = MALE_MUSCLED_RUN_TO_ROLLING_ANIM_LENGTH,

        .stand_to_rolling_anim_grip = MALE_MUSCLED_STAND_TO_ROLLING_ANIM_GRIP,
        .stand_to_rolling_anim_length = MALE_MUSCLED_STAND_TO_ROLLING_ANIM_LENGTH,
    },
};

