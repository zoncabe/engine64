#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <libdragon.h>
#include <fgeom.h>
#include <t3d/t3dmodel.h>

#include "entity/entity.h"
#include "character/character.h"
#include "character/character_animation.h"


Character *character_create(const CharacterDef *def, Entity *entity)
{
	Character *character = malloc(sizeof(Character));
	assert(character);

	*character = (Character){
		.entity    = entity,
		.body      = (KinematicBody){ .position = vector3_scaled(&entity->transform.position, RENDER_SCALE_INV), .rotation = entity->transform.rotation },
		.movement  = (CharacterMovement){ .settings = def->movement_settings, .data.is_grounded = true, .current = MOVEMENT_STATE_IDLE },
		.animation = (CharacterAnimation){ .def = def->animation_def },
		.weapons   = (CharacterWeapons){ .def = def->weapons_def, .drawn = CHARACTER_WEAPON_DRAWN_NONE },
	};

	characterCollider_init(&character->collider,
		def->collider_settings->radius,
		(def->collider_settings->height - 2.0f * def->collider_settings->radius) * 0.5f);
	characterCollider_setVertical(&character->collider, &character->body.position);

	characterAnimation_initGraph(character, def->animation_def);
	entity->mesh->skeleton = &character->animation.main;

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
	free(animation->buffer);
	free(animation->node_state);
	free(animation->node_active);
	free(character);
}
