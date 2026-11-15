#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <libdragon.h>

#include "entity/e64_entity.h"
#include "character3d/e64_character3d.h"

namespace e64 {

namespace character3d {

Character3D *create(const Def *def, Entity *entity)
{
	/* The spring bone states ride in the same allocation; their only
	   references are the modifier contexts, freed with the character. */
	uint8_t spring_bones = 0;
	if (def->spring_bones)
		for (const SpringBonesDef *set = def->spring_bones; set->count; set++)
			spring_bones += set->count;

	Character3D *character = (Character3D *)malloc(sizeof(Character3D) + spring_bones * sizeof(SpringBone));
	assert(character);

	const ColliderSettings *collider_settings = def->collider_settings;

	*character = (Character3D){
		.entity = entity,
		.collider = (Collider){
			.foot_offset = collider_settings->foot_offset,
		},
		.movement = (Movement){
			.settings = *def->movement_settings,
			.data = { .is_grounded = true },
			.mode = MOVEMENT_STATE_IDLE,
		},
	};

	/* The instance stands at its position: the capsule center goes half the
	   capsule's height above it. */
	Vector3 center = entity->transform.position;
	center.z += collider_settings->height * 0.5f;

	collider::init(&character->collider,
		collider_settings->radius,
		(collider_settings->height - 2.0f * collider_settings->radius) * 0.5f,
		&center,
		entity->transform.rotation.z);

	if (spring_bones > 0) {
		assert(entity->mesh_count > 0);
		const T3DSkeleton *skeleton = entity->mesh[0]->skeleton;
		assert(skeleton);

		SpringBone *spring_bone = (SpringBone *)(character + 1);
		uint8_t n = 0;

		/* Chains rely on this order: the resolved joints run root to tip,
		   so each modifier runs after the one it hangs from. */
		for (const SpringBonesDef *set = def->spring_bones; set->count; set++) {
			int16_t joint[16];
			uint8_t count = springBones_resolveChain(skeleton, set, joint, 16);
			if (count > set->count) count = set->count;

			for (uint8_t i = 0; i < count; i++) {
				if (!springBone_init(&spring_bone[n], skeleton, joint[i], i, set, &character->collider.transform))
					continue;

				skeleton::modifiers::add(&character->skeleton_modifiers, springBone_apply, &spring_bone[n]);
				n++;
			}
		}
	}

	return character;
}

void destroy(Character3D *character)
{
	free(character);
}

}

}
