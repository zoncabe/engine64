/*
	Camera-pitch aim bend. See character_aim.h for the frame math rationale.
*/
#include <fmath.h>
#include <t3d/t3dskeleton.h>

#include "entity/entity.h"
#include "character/character.h"


void characterAim_init(Character *character)
{
	CharacterAim *aim = &character->aim;

	for (uint8_t i = 0; i < aim->def->count && i < CHARACTER_AIM_MAX_BONES; i++)
		aim->bone[i] = (int16_t)t3d_skeleton_find_bone(&character->animation.main,
		                                               (char *)aim->def->bone[i]);
}

void characterAim_apply(T3DSkeleton *skeleton, void *context)
{
	Character *character = context;
	CharacterAim *aim = &character->aim;

	/* The hold is already half an aim: both modes bend, and the combined
	   presence keeps the weight steady through the hold-aim crossfade. */
	float hold  = character->animation.bow_hold_blend;
	float draw  = character->animation.bow_aim_blend;
	float blend = 1.0f - (1.0f - hold) * (1.0f - draw);
	if (blend <= 0.0f || !aim->pitch || *aim->pitch == 0.0f) return;

	/* One share of the bend per vertebra, about the model's side axis (X):
	   the body is already yaw-locked to the camera while aiming, so the
	   camera's horizontal right is a constant axis in model space. */
	float half = *aim->pitch * aim->def->pitch_scale * blend / (float)aim->def->count
	           * 0.5f * 0.01745329f;
	T3DQuat delta = {{ fm_sinf(half), 0.0f, 0.0f, fm_cosf(half) }};

	for (uint8_t i = 0; i < aim->def->count && i < CHARACTER_AIM_MAX_BONES; i++) {
		int16_t b = aim->bone[i];
		if (b < 0) continue;

		/* Conjugate the model-space delta into this bone's frame through the
		   parent chain as it stands right now — earlier vertebrae already
		   carry their share, so each one bends about the same world axis. */
		T3DVec3 parent_pos;
		T3DQuat parent_rot;
		character_getBonePose(skeleton, (int16_t)skeleton->skeletonRef->bones[b].parentIdx,
		                      &parent_pos, &parent_rot);

		T3DQuat inverse = {{ -parent_rot.v[0], -parent_rot.v[1], -parent_rot.v[2], parent_rot.v[3] }};
		T3DBone *bone = &skeleton->bones[b];

		T3DQuat q, local;
		t3d_quat_mul(&q, &delta, &parent_rot);
		t3d_quat_mul(&local, &inverse, &q);
		t3d_quat_mul(&q, &local, &bone->rotation);

		bone->rotation = q;
		bone->hasChanged = 1;
	}
}
