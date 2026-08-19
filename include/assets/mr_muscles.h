#ifndef MR_MUSCLES_H
#define MR_MUSCLES_H

#include "character/character.h"

#define mr_muscles_model "rom:/models/mr_muscles.t3dm"
#define gorilla_model "rom:/models/gorilla.t3dm"


typedef enum {

	MM_SLOT_IDLE_R,
	MM_SLOT_WALK,
	MM_SLOT_RUN,
	MM_SLOT_SPRINT,
	MM_SLOT_TURN_WALK,
	MM_SLOT_TURN_RUN,
	MM_SLOT_STRAFE_WALK,
	MM_SLOT_STRAFE_RUN,
	MM_SLOT_STRAFE_LOCKED_WALK,
	MM_SLOT_STRAFE_LOCKED_WALK_SIDE,
	MM_SLOT_STRAFE_LOCKED_RUN,
	MM_SLOT_STRAFE_LOCKED_RUN_SIDE,
	MM_SLOT_BOW_WALK_AIMING,
	MM_SLOT_BOW_WALK_AIMING_SIDE,

	MM_SLOT_JUMP_L,
	MM_SLOT_JUMP_R,
	MM_SLOT_LAND_L,
	MM_SLOT_LAND_R,

	MM_SLOT_ROLL_RUN,

	/* Two slots for the swim grid: adjacent columns must land in different
	   buffers or their poses overwrite each other instead of blending. */
	MM_SLOT_SWIM_A,
	MM_SLOT_SWIM_B,

	MM_SLOT_TRANSITION,

	MM_SLOT_COUNT

} MrMusclesSlot;

typedef enum {

	MM_NODE_IDLE,
	MM_NODE_IDLE_R,
	MM_NODE_LOCOMOTION,
	MM_NODE_JUMP_L,
	MM_NODE_JUMP_R,
	MM_NODE_JUMP_L_LAYER,
	MM_NODE_JUMP_R_LAYER,
	MM_NODE_LAND_L,
	MM_NODE_LAND_R,
	MM_NODE_ROLL,
	MM_NODE_ROLL_LAYER,
	MM_NODE_STRAFE,
	MM_NODE_STRAFE_LOCKED,
	MM_NODE_BOW_WALK_AIMING,
	MM_NODE_SWIM,

	MM_NODE_COUNT

} MrMusclesNode;

typedef enum {

	MM_ANIM_IDLE_L,
	MM_ANIM_IDLE_R,

	MM_ANIM_STAND_TO_WALK_L,
	MM_ANIM_STAND_TO_WALK_R,

	MM_ANIM_STAND_TO_RUN_L,
	MM_ANIM_STAND_TO_RUN_R,

	MM_ANIM_WALK,
	MM_ANIM_TURN_WALK_L,
	MM_ANIM_TURN_WALK_R,
	MM_ANIM_WALK_CHANGE_DIR_L,
	MM_ANIM_WALK_CHANGE_DIR_R,
	MM_ANIM_WALK_TO_STAND_L,
	MM_ANIM_WALK_TO_STAND_R,

	MM_ANIM_WALK_BACK,
	MM_ANIM_WALK_BACK_L,
	MM_ANIM_WALK_BACK_R,
	MM_ANIM_WALK_STRAFE_L,
	MM_ANIM_WALK_STRAFE_R,

	MM_ANIM_RUN,
	MM_ANIM_TURN_RUN_L,
	MM_ANIM_TURN_RUN_R,
	MM_ANIM_RUN_CHANGE_DIR_L,
	MM_ANIM_RUN_CHANGE_DIR_R,
	MM_ANIM_RUN_TO_STAND_L,
	MM_ANIM_RUN_TO_STAND_R,

	MM_ANIM_RUN_BACK,
	MM_ANIM_RUN_BACK_L,
	MM_ANIM_RUN_BACK_R,
	MM_ANIM_RUN_STRAFE_L,
	MM_ANIM_RUN_STRAFE_R,

	MM_ANIM_SPRINT,

	MM_ANIM_STRAFE_LOCKED_WALK_FWD,
	MM_ANIM_STRAFE_LOCKED_WALK_BACK,
	MM_ANIM_STRAFE_LOCKED_WALK_L,
	MM_ANIM_STRAFE_LOCKED_WALK_R,
	MM_ANIM_STRAFE_LOCKED_RUN_FWD,
	MM_ANIM_STRAFE_LOCKED_RUN_BACK,
	MM_ANIM_STRAFE_LOCKED_RUN_L,
	MM_ANIM_STRAFE_LOCKED_RUN_R,

	MM_ANIM_BOW_WALK_AIMING_FWD,
	MM_ANIM_BOW_WALK_AIMING_BACK,
	MM_ANIM_BOW_WALK_AIMING_L,
	MM_ANIM_BOW_WALK_AIMING_R,

	MM_ANIM_JUMP_L,
	MM_ANIM_JUMP_R,
	MM_ANIM_FALL_L,
	MM_ANIM_FALL_R,
	MM_ANIM_LAND_L,
	MM_ANIM_LAND_R,

	MM_ANIM_ROLL_L,
	MM_ANIM_ROLL_R,

	MM_ANIM_SWIM_IDLE,
	MM_ANIM_SWIM_SLOW,
	MM_ANIM_SWIM_FAST,

	MM_ANIM_SLIDE_L,
	MM_ANIM_SLIDE_R,

	MM_ANIM_TRANSITION_L,
	MM_ANIM_TRANSITION_R,

	MM_ANIM_COUNT
	
} MrMusclesAnim;


extern const CharacterMovementSettings    mr_muscles_movement_settings;
extern const CharacterColliderSettings    mr_muscles_collider_settings;

extern const char *const mr_muscles_weapon_meshes[];
extern const CharacterWeaponsDef mr_muscles_weapons_def;
extern const CharacterDef mr_muscles_character_def;

extern const WeaponDef weapon_ak47;
extern const WeaponDef weapon_bow;
extern const WeaponDef weapon_m1911;
extern const WeaponDef weapon_knife;
extern const CharacterAnimationSettings mr_muscles_animation_settings;
extern const CharacterAnimationDef           mr_muscles_animation_def;

#endif
