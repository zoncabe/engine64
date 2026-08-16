#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <libdragon.h>
#include <fgeom.h>
#include <t3d/t3dmodel.h>

#include "entity/entity.h"
#include "character/character.h"
#include "character/character_animation.h"


/* SkeletonModifierFn: the weapon posing; context is the Character. */
static void character_weaponModifier(T3DSkeleton *skeleton, void *context)
{
	(void)skeleton;
	characterWeapon_setBones(context);
}

Character *character_create(const CharacterDef *def, Entity *entity)
{
	/* The spring bone states ride in the same allocation; their only
	   references are the modifier contexts, freed with the character. */
	uint8_t spring_bones = 0;
	if (def->spring_bones)
		for (const SpringBonesDef *set = def->spring_bones; set->count; set++)
			spring_bones += set->count;

	Character *character = malloc(sizeof(Character) + spring_bones * sizeof(SpringBone));
	assert(character);

	*character = (Character){
		.entity    = entity,
		.body      = (KinematicBody){ .position = vector3_scaled(&entity->transform.position, RENDER_SCALE_INV), .rotation = entity->transform.rotation },
		.movement  = (CharacterMovement){ .settings = def->movement_settings, .data.is_grounded = true, .current = MOVEMENT_STATE_IDLE },
		.animation = (CharacterAnimation){ .def = def->animation_def },
		.weapons   = (CharacterWeapons){ .def = def->weapons_def, .drawn = CHARACTER_WEAPON_DRAWN_NONE },
		/* No previous frame to compare against yet: a cycle of -1 crosses
		   nothing, and the body starts standing on the floor. */
		.sound     = (CharacterSound){ .def = def->sound_def, .previous_cycle = -1.0f, .previous_grounded = true },
	};

	characterCollider_init(&character->collider,
		def->collider_settings->radius,
		(def->collider_settings->height - 2.0f * def->collider_settings->radius) * 0.5f);
	characterCollider_setVertical(&character->collider, &character->body.position);

	characterAnimation_initGraph(character, def->animation_def);
	entity->mesh->skeleton = &character->animation.main;

	skeletonModifiers_add(&character->skeleton_modifiers, character_weaponModifier, character);

	if (spring_bones > 0) {
		SpringBone *spring_bone = (SpringBone *)(character + 1);
		uint8_t n = 0;

		/* Chains rely on this order: the resolved joints run root to tip,
		   so each modifier runs after the one it hangs from. */
		for (const SpringBonesDef *set = def->spring_bones; set->count; set++) {
			int16_t joint[16];
			uint8_t count = springBones_resolveChain(&character->animation.main, set, joint, 16);
			if (count > set->count) count = set->count;

			for (uint8_t i = 0; i < count; i++) {
				if (!springBone_init(&spring_bone[n], &character->animation.main, joint[i],
				                     i, set, &entity->transform))
					continue;

				skeletonModifiers_add(&character->skeleton_modifiers, springBone_apply, &spring_bone[n]);
				n++;
			}
		}
	}

	/* Part 0 = body, parts 1..N = one per weapon object, def order.
	   Only the body starts visible; equipping turns weapon bits on. */
	mesh_recordParts(entity->mesh, def->weapons_def->mesh, def->weapons_def->mesh_count,
		(const T3DMat4FP *)t3d_segment_placeholder(T3D_SEGMENT_SKELETON));

	return character;
}

/* T3DQuat and T3DVec3 are libdragon's fm_quat_t / float[3] underneath. */
static T3DVec3 quat_rotateVec3(const T3DQuat *q, const T3DVec3 *v)
{
	fm_vec3_t out;
	fm_vec3_rotate(&out, (const fm_vec3_t *)v, (const fm_quat_t *)q);
	return (T3DVec3){{ out.x, out.y, out.z }};
}

void character_getBoneModelSpacePose(const T3DSkeleton *skeleton, int16_t bone, T3DVec3 *position, T3DQuat *rotation)
{
	uint16_t chain[16];
	int depth = 0;

	uint16_t idx = (uint16_t)bone;
	while (idx != 0xFFFF && depth < 16) {
		chain[depth++] = idx;
		idx = skeleton->skeletonRef->bones[idx].parentIdx;
	}

	*position = (T3DVec3){{ 0.0f, 0.0f, 0.0f }};
	*rotation = (T3DQuat){{ 0.0f, 0.0f, 0.0f, 1.0f }};

	for (int i = depth - 1; i >= 0; i--) {
		const T3DBone *b = &skeleton->bones[chain[i]];

		T3DVec3 step = quat_rotateVec3(rotation, &b->position);
		position->v[0] += step.v[0];
		position->v[1] += step.v[1];
		position->v[2] += step.v[2];

		T3DQuat next;
		t3d_quat_mul(&next, rotation, (T3DQuat *)&b->rotation);
		*rotation = next;
	}
}

/* Model-space pose of a bone, composed from the local TRS chain so it is
   current-frame (bone->matrix would lag one skeleton update behind). */
void character_getBonePose(const T3DSkeleton *skeleton, int16_t bone, T3DVec3 *position, T3DQuat *rotation)
{
	uint16_t chain[16];
	int depth = 0;

	uint16_t idx = (uint16_t)bone;
	while (idx != 0xFFFF && depth < 16) {
		chain[depth++] = idx;
		idx = skeleton->skeletonRef->bones[idx].parentIdx;
	}

	*position = (T3DVec3){{ 0.0f, 0.0f, 0.0f }};
	*rotation = (T3DQuat){{ 0.0f, 0.0f, 0.0f, 1.0f }};

	for (int i = depth - 1; i >= 0; i--) {
		const T3DBone *b = &skeleton->bones[chain[i]];

		fm_vec3_t step;
		fm_vec3_rotate(&step, (const fm_vec3_t *)&b->position, (const fm_quat_t *)rotation);
		position->v[0] += step.x;
		position->v[1] += step.y;
		position->v[2] += step.z;

		T3DQuat next;
		t3d_quat_mul(&next, rotation, (T3DQuat *)&b->rotation);
		*rotation = next;
	}
}

void character_delete(Character *character)
{
	CharacterAnimation *animation = &character->animation;

	for (int i = 0; i < animation->def->clip_count; i++)
		t3d_anim_destroy(&animation->clip[i]);
	for (int i = 0; i < animation->def->buffer_count; i++)
		t3d_skeleton_destroy(&animation->buffer[i]);
	t3d_skeleton_destroy(&animation->main);

	free(animation->clip);
	free(animation->clip_cooldown);
	free(animation->buffer);
	free(animation->node_state);
	free(animation->node_active);
	free(character);
}
