#ifndef CHARACTER_H
#define CHARACTER_H

#include <stdbool.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>

#include "physics/physics.h"
#include "graphics/mesh.h"
#include "character/character_physics.h"
#include "character/character_movement.h"
#include "character/character_stats.h"
#include "character/character_animation.h"
#include "character/character_weapon.h"
#include "character/character_aim.h"
#include "character/character_skeleton.h"
#include "character/character_spring_bone.h"
#include "character/character_sound.h"

typedef struct Entity Entity;

typedef struct CharacterDef {

	const CharacterMovementSettings *movement_settings;
	const CharacterAnimationDef *animation_def;
	const CharacterColliderSettings *collider_settings;
	const CharacterWeaponsDef *weapons_def;
	const SpringBonesDef *spring_bones;   /* optional: array of sets, one tuning each, count 0 terminates */
	const CharacterAimDef *aim_def;       /* optional: spine chain for the camera-pitch bend */
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
	CharacterAim        aim;
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
