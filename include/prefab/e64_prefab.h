/*
	Content declaration: a model plus what its kind needs, behind a tag.
	Position, rotation and scale are not here, they belong to the placement,
	so one prefab can be placed any number of times.
*/
#ifndef ENGINE64_PREFAB_H
#define ENGINE64_PREFAB_H

#include <stdint.h>

#include "entity/e64_entity.h"
#include "physics/body/e64_rigid_body.h"
#include "physics/cloth/e64_cloth.h"
#include "character/e64_character.h"
#include "shaders/e64_water.h"
#include "sound/e64_prefab_sound.h"


typedef enum {

	PREFAB_CHARACTER,
	PREFAB_PROP,
	PREFAB_CLOTH,
	PREFAB_WATER,

} PrefabType;


typedef struct Prefab {

	PrefabType type;
	const char *model;

	const PrefabSound *sound;
	uint8_t sound_count;

	/* Solid for a prop, sensor volume for water. NULL: no collision. */
	const EntityColliderDef *collider;

	/* The kind the tag names. A prop without a body is static. */
	union {
		const CharacterDef *character;
		const RigidBodyDef *prop;
		const ClothDef     *cloth;
		const WaterDef     *water;
	};

} Prefab;


#endif
