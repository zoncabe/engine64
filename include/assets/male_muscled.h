#ifndef MALE_MUSCLED_H
#define MALE_MUSCLED_H

#include "character/character.h"

#define male_muscled_model "rom:/models/male_muscled.t3dm"
#define gorilla_model "rom:/models/gorilla.t3dm"

typedef enum {

	MM_STATE_IDLE,
	MM_STATE_WALKING,
	MM_STATE_RUNNING,
	MM_STATE_SPRINTING,
	MM_STATE_ROLLING,
	MM_STATE_JUMPING,
	MM_STATE_FALLING,
	MM_STATE_COUNT

} MaleMusledStates;


typedef enum {

	MM_SLOT_IDLE_R,
	MM_SLOT_WALK,
	MM_SLOT_RUN,
	MM_SLOT_SPRINT,
	MM_SLOT_TURN_WALK,
	MM_SLOT_TURN_RUN,

	MM_SLOT_JUMP_L,
	MM_SLOT_JUMP_R,
	MM_SLOT_LAND_L,
	MM_SLOT_LAND_R,

	MM_SLOT_ROLL_RUN,

	MM_SLOT_TRANSITION,
	
	MM_SLOT_COUNT

} MaleMuscledSlot;

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

	MM_ANIM_JUMP_L,
	MM_ANIM_JUMP_R,
	MM_ANIM_FALL_L,
	MM_ANIM_FALL_R,
	MM_ANIM_LAND_L,
	MM_ANIM_LAND_R,

	MM_ANIM_ROLL_L,
	MM_ANIM_ROLL_R,

	MM_ANIM_SLIDE_L,
	MM_ANIM_SLIDE_R,

	MM_ANIM_TRANSITION_L,
	MM_ANIM_TRANSITION_R,

	MM_ANIM_COUNT
	
} MaleMuscledAnim;


extern const CharacterMovementSettings    male_muscled_movement_settings;
extern const CharacterColliderSettings    male_muscled_collider_settings;
extern const CharacterAnimationSettings male_muscled_animation_settings;
extern const CharacterAnimationDef           male_muscled_animation_def;

#endif
