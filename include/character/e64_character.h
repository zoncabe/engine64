#ifndef ENGINE64_CHARACTER_H
#define ENGINE64_CHARACTER_H

#include <stdbool.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>

#include "physics/e64_physics.h"
#include "graphics/e64_mesh.h"
#include "character/e64_character_physics.h"
#include "character/e64_character_movement.h"
#include "character/e64_character_stats.h"
#include "character/e64_character_animation.h"
#include "character/e64_character_weapon.h"
#include "character/e64_character_aim.h"
#include "character/e64_character_skeleton.h"
#include "character/e64_character_spring_bone.h"
#include "character/e64_character_sound.h"

typedef struct Entity Entity;

typedef struct CharacterDef {

	const CharacterMovementSettings *movement_settings;
	const CharacterAnimationDef *animation_def;
	const CharacterColliderSettings *collider_settings;
	const CharacterWeaponsDef *weapons_def;
	const SpringBonesDef *spring_bones;   /* optional: array of sets, one tuning each, count 0 terminates */
	const CharacterAimingSettings *aiming_settings;   /* optional: spine chain for the camera-pitch bend */
	const CharacterSoundDef *sound_def;
	const CharacterStatsSettings *stats_settings;

} CharacterDef;

typedef struct Character {

	Entity             *entity;
	KinematicBody       body;
	CharacterCollider   collider;
	CharacterMovement   movement;
	CharacterAnimation  animation;
	CharacterWeapons    weapons;
	CharacterAiming     aiming;
	CharacterSound      sound;
	SkeletonModifiers   skeleton_modifiers;
	CharacterStats      stats;

} Character;


Character *character_create(const CharacterDef *def, Entity *entity);
void character_delete(Character *character);

/* Model-space pose of a bone, composed from the local TRS chain so it is
   current-frame (bone->matrix would lag one skeleton update behind). */
void character_getBonePose(const T3DSkeleton *skeleton, int16_t bone, T3DVec3 *position, T3DQuat *rotation);


#endif
