/*
	Weapon slots, equip and bone posing. Weapon meshes live inside the
	character model, skinned to dedicated root-level bones; posing those bones
	parks the weapon on its holster reference bone or on the hand.
*/
#include <assert.h>
#include <string.h>
#include <fgeom.h>
#include <t3d/t3dskeleton.h>

#include "entity/entity.h"
#include "character/character.h"


/* T3DQuat and T3DVec3 are libdragon's fm_quat_t / float[3] underneath. */
static T3DVec3 quat_rotateVec3(const T3DQuat *q, const T3DVec3 *v)
{
	fm_vec3_t out;
	fm_vec3_rotate(&out, (const fm_vec3_t *)v, (const fm_quat_t *)q);
	return (T3DVec3){{ out.x, out.y, out.z }};
}

void character_equipWeapon(Character *character, uint8_t slot_id, const WeaponDef *weapon)
{
	assert(slot_id < WEAPON_SLOT_COUNT);

	CharacterWeapons *weapons = &character->weapons;
	T3DSkeleton *skeleton = &character->animation.main;

	uint8_t part = 0;
	for (int i = 0; i < weapons->def->mesh_count; i++) {
		if (strcmp(weapon->mesh, weapons->def->mesh[i]) == 0) {
			part = 1 + i;
			break;
		}
	}
	assert(part);   /* weapon mesh must exist in the character model */

	weapons->slot[slot_id] = (WeaponSlot){
		.weapon       = weapon,
		.rounds       = weapon->magazine_size,
		.integrity    = weapon->max_integrity,
		.part         = part,
		.bone         = (int16_t)t3d_skeleton_find_bone(skeleton, (char *)weapon->bone),
		.holster_bone = (int16_t)t3d_skeleton_find_bone(skeleton, (char *)weapon->holster_bone),
		.hand_bone    = (int16_t)t3d_skeleton_find_bone(skeleton, (char *)weapon->hand_bone),
	};

	character->entity->mesh->visible |= 1u << part;
}

void character_unequipWeapon(Character *character, uint8_t slot_id)
{
	assert(slot_id < WEAPON_SLOT_COUNT);

	WeaponSlot *slot = &character->weapons.slot[slot_id];
	if (!slot->weapon) return;

	character->entity->mesh->visible &= ~(1u << slot->part);
	if (character->weapons.drawn == slot_id)
		character->weapons.drawn = CHARACTER_WEAPON_DRAWN_NONE;

	*slot = (WeaponSlot){0};
}

void characterWeapon_setBones(Character *character)
{
	CharacterWeapons *weapons = &character->weapons;
	T3DSkeleton *skeleton = &character->animation.main;

	for (int s = 0; s < WEAPON_SLOT_COUNT; s++) {
		WeaponSlot *slot = &weapons->slot[s];
		if (!slot->weapon || slot->bone < 0) continue;

		bool drawn = (weapons->drawn == s);
		int16_t reference     = drawn ? slot->hand_bone : slot->holster_bone;
		const T3DVec3 *offset_pos = drawn ? &slot->weapon->holding_position : &slot->weapon->holster_position;
		const T3DQuat *offset_rot = drawn ? &slot->weapon->holding_rotation : &slot->weapon->holster_rotation;
		if (reference < 0) continue;

		T3DVec3 ref_pos;
		T3DQuat ref_rot;
		character_getBonePose(skeleton, reference, &ref_pos, &ref_rot);

		T3DVec3 step = quat_rotateVec3(&ref_rot, offset_pos);

		T3DBone *bone = &skeleton->bones[slot->bone];
		bone->position = (T3DVec3){{
			ref_pos.v[0] + step.v[0],
			ref_pos.v[1] + step.v[1],
			ref_pos.v[2] + step.v[2],
		}};
		t3d_quat_mul(&bone->rotation, &ref_rot, (T3DQuat *)offset_rot);
		bone->hasChanged = 1;
	}
}
