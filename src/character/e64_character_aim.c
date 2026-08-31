/*
	Camera-pitch aim bend. See character_aim.h for the frame math rationale.
*/
#include <fmath.h>
#include <t3d/t3dskeleton.h>

#include "physics/math/e64_math_common.h"
#include "camera/e64_spring_arm.h"
#include "viewport/e64_viewport.h"
#include "entity/e64_entity.h"
#include "character/e64_character.h"


void characterAim_init(Character *character, const CharacterAimingSettings *settings)
{
	CharacterAiming *aiming = &character->aiming;

	aiming->count = settings->count < CHARACTER_AIM_MAX_BONES
	              ? settings->count : CHARACTER_AIM_MAX_BONES;
	aiming->pitch_scale = settings->pitch_scale;

	for (uint8_t i = 0; i < aiming->count; i++)
		aiming->bone[i] = (int16_t)t3d_skeleton_find_bone(&character->animation.main,
		                                                  (char *)settings->bone[i]);
}

void characterAim_apply(T3DSkeleton *skeleton, void *context)
{
	Character *character = context;
	CharacterAiming *aiming = &character->aiming;

	/* The hold is already half an aim: both modes bend, and the combined
	   presence keeps the weight steady through the hold-aim crossfade. */
	float hold  = character->animation.aiming_blend;
	float draw  = character->animation.charging_shoot_blend;
	float blend = 1.0f - (1.0f - hold) * (1.0f - draw);
	if (blend <= 0.0f || aiming->count == 0) return;

	const Camera *camera = &viewport_get()->camera;
	float pitch = cameraSpringArm_getPitch(camera);
	if (pitch == 0.0f) return;

	/* The bend is about the camera's horizontal right. That is the model's X
	   only while the body faces the camera, which the strafe does but nothing
	   guarantees: mid-turn, or with the strafe off while the pose fades, the
	   body sits at its own yaw. So the axis is turned by however far the body
	   is off from facing the camera, which is zero once it is. */
	float facing = cameraSpringArm_getYaw(camera) + 180.0f;
	float offset = (angle_wrap_relative(facing, character->body.rotation.z)
	                - character->body.rotation.z) * 0.01745329f;

	/* One share of the bend per vertebra. */
	float half = pitch * aiming->pitch_scale * blend / (float)aiming->count
	           * 0.5f * 0.01745329f;

	float sin_half = fm_sinf(half);
	T3DQuat delta = {{ fm_cosf(offset) * sin_half,
	                   fm_sinf(offset) * sin_half,
	                   0.0f,
	                   fm_cosf(half) }};

	for (uint8_t i = 0; i < aiming->count; i++) {
		int16_t b = aiming->bone[i];
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
