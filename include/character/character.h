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
#include "character/character_animation.h"
#include "character/character_weapon.h"

typedef struct Entity Entity;

typedef struct CharacterDef {

	const CharacterMovementSettings *movement_settings;
	const CharacterAnimationDef *animation_def;
	const CharacterColliderSettings *collider_settings;
	const CharacterWeaponsDef *weapons_def;

} CharacterDef;

typedef struct Character {

	Entity             *entity;
	KinematicBody       body;
	CharacterCollider   collider;
	CharacterMovement   movement;
	CharacterAnimation  animation;
	CharacterWeapons    weapons;

} Character;


Character *character_create(const CharacterDef *def, Entity *entity);
void character_delete(Character *character);


#endif
