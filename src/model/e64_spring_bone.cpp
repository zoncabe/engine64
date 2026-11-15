/*
	Translation of SpringBoneSimulator3D::_process_joints and the
	SkeletonModifier3D helpers it calls (limit_length, get_from_to_rotation),
	one joint per modifier call. See character3d_spring_bone.h.
*/
#include <math.h>
#include <fgeom.h>
#include <fmath.h>

#include "model/e64_spring_bone.h"
#include "model/e64_skeleton.h"
#include "engine/e64_common.h"
#include "math/e64_math.h"
#include "math/e64_matrix3.h"
#include "time/e64_time.h"

namespace e64 {

/* As the joint list of a Godot setting: the bones from root to end, found
   by walking the end bone's parents. */
uint8_t springBones_resolveChain(const T3DSkeleton *skeleton, const SpringBonesDef *def,
                                 int16_t *joints, uint8_t max)
{
	int16_t root = (int16_t)t3d_skeleton_find_bone((T3DSkeleton *)skeleton, (char *)def->root_bone);
	int16_t end = (int16_t)t3d_skeleton_find_bone((T3DSkeleton *)skeleton, (char *)def->end_bone);
	if (root < 0 || end < 0) return 0;

	int16_t chain[16];
	int depth = 0;

	uint16_t idx = (uint16_t)end;
	while (idx != 0xFFFF && depth < 16) {
		chain[depth++] = (int16_t)idx;
		if (idx == (uint16_t)root) break;
		idx = skeleton->skeletonRef->bones[idx].parentIdx;
	}
	if (chain[depth - 1] != root) return 0;

	uint8_t count = 0;
	for (int i = depth - 1; i >= 0 && count < max; i--)
		joints[count++] = chain[i];

	return count;
}


/* As _make_joints: the joint's forward and length come from its child's
   rest offset; the end bone has no child and extends by the def's length
   along its own axis. */
bool springBone_init(SpringBone *spring_bone, const T3DSkeleton *skeleton, int16_t bone,
                     uint8_t joint_index, const SpringBonesDef *def, const Transform *world)
{
	int16_t index = bone;
	if (index < 0) return false;

	*spring_bone = (SpringBone){};
	spring_bone->bone = index;
	spring_bone->def = def;
	spring_bone->world = world;

	spring_bone->forward_vector = (Vector3){ 0.0f, 1.0f, 0.0f };
	spring_bone->length = def->end_bone_length * RENDER_SCALE;
	spring_bone->current_rot = (Quaternion){ 0.0f, 0.0f, 0.0f, 1.0f };

	const T3DChunkSkeleton *ref = skeleton->skeletonRef;
	for (uint16_t i = 0; i < ref->boneCount; i++) {
		if (ref->bones[i].parentIdx != (uint16_t)index) continue;

		Vector3 axis = { ref->bones[i].position.v[0],
		                 ref->bones[i].position.v[1],
		                 ref->bones[i].position.v[2] };

		spring_bone->length = vector3::magnitude(&axis);
		vector3::normalize(&axis);
		spring_bone->forward_vector = axis;
		break;
	}

	/* individual_config: the per-joint radius overrides the setting's */
	float radius = def->joint_radius ? def->joint_radius[joint_index] : def->radius;
	spring_bone->radius = radius * RENDER_SCALE;

	for (uint8_t i = 0; i < def->collider_count && i < SPRING_BONE_COLLIDER_MAX; i++) {
		const SpringBoneColliderDef *collider = &def->collider[i];

		int16_t collider_bone = (int16_t)t3d_skeleton_find_bone((T3DSkeleton *)skeleton, (char *)collider->bone);
		if (collider_bone < 0) continue;

		uint8_t n = spring_bone->collider_count++;
		spring_bone->collider[n].shape = collider->shape;
		spring_bone->collider[n].bone = collider_bone;
		spring_bone->collider[n].position = vector3::scaled(&collider->position, RENDER_SCALE);
		spring_bone->collider[n].radius = collider->radius * RENDER_SCALE;
		spring_bone->collider[n].height = collider->height * RENDER_SCALE;
		spring_bone->collider[n].inside = collider->inside;

		T3DMat4 mat;
		t3d_mat4_from_srt_euler(&mat,
			(float[3]){1.0f, 1.0f, 1.0f},
			(float[3]){deg_to_rad(collider->rotation.x), deg_to_rad(collider->rotation.y), deg_to_rad(collider->rotation.z)},
			(float[3]){0.0f, 0.0f, 0.0f});
		spring_bone->collider[n].rotation = (Matrix3){
			.ex = { mat.m[0][0], mat.m[0][1], mat.m[0][2] },
			.ey = { mat.m[1][0], mat.m[1][1], mat.m[1][2] },
			.ez = { mat.m[2][0], mat.m[2][1], mat.m[2][2] },
		};
	}

	return true;
}


/* SkeletonModifier3D::limit_length. The guard keeps degenerate cases off
   the N64 FPU: inf/NaN/denormal operands trap to a software handler per
   instruction, and a contact can park the tail on top of the origin. */
static Vector3 springBone_limitLength(const Vector3 *origin, const Vector3 *destination, float length)
{
	Vector3 dir = vector3::difference(destination, origin);
	float mag = vector3::magnitude(&dir);
	if (mag < 1e-6f) return *destination;

	vector3::scale(&dir, length / mag);
	return vector3::sum(origin, &dir);
}

/* SkeletonModifier3D::get_from_to_rotation; from and to are unit vectors.
   At the degenerate poles the previous rotation is the answer, "for
   preventing to glitch". */
static Quaternion springBone_fromToRotation(const Vector3 *from, const Vector3 *to, const Quaternion *prev_rot)
{
	float dot = vector3::dot(from, to);
	if (dot < -0.999999f)
		return *prev_rot;

	Vector3 axis = vector3::cross(from, to);
	if (vector3::squaredMagnitude(&axis) < 1e-12f)
		return *prev_rot;

	if (dot > 1.0f) dot = 1.0f;
	float angle = acosf(dot);

	vector3::normalize(&axis);
	return quaternion::fromAxisAngle(&axis, angle);
}

/* SpringBoneCollisionSphere3D::_collide_sphere */
static Vector3 springBoneCollision_collideSphere(const Vector3 *origin, float radius, bool inside,
                                                 float bone_radius, float bone_length, const Vector3 *current)
{
	(void)bone_length;

	Vector3 diff = vector3::difference(current, origin);
	float length = vector3::magnitude(&diff);
	float r = inside ? radius - bone_radius : bone_radius + radius;
	float distance = inside ? r - length : length - r;

	if (distance > 0.0f)
		return *current;

	/* normalized() of a zero vector is zero in Godot; the epsilon also keeps
	   r / length from blowing up into the FPU's software-trap range. */
	if (length > 1e-6f)
		vector3::scale(&diff, r / length);

	return vector3::sum(origin, &diff);
}

/* SpringBoneCollisionCapsule3D::_collide; the segment ends come from
   get_head_and_tail: origin +/- local Y * (height / 2 - radius). */
static Vector3 springBoneCollision_collideCapsule(const Vector3 *origin, const Matrix3 *basis,
                                                  float radius, float height, bool inside,
                                                  float bone_radius, float bone_length, const Vector3 *current)
{
	Vector3 up = { 0.0f, height * 0.5f - radius, 0.0f };
	up = matrix3::transformVector(basis, &up);

	Vector3 head = vector3::sum(origin, &up);
	Vector3 tail = vector3::difference(origin, &up);

	Vector3 p = vector3::difference(&tail, &head);
	Vector3 q = vector3::difference(current, &head);

	float dot = vector3::dot(&p, &q);
	if (dot <= 0.0f)
		return springBoneCollision_collideSphere(&head, radius, inside, bone_radius, bone_length, current);

	float pls = vector3::squaredMagnitude(&p);
	if (pls < 1e-12f)
		return *current;

	if (pls <= dot) {
		Vector3 end = vector3::sum(&head, &p);
		return springBoneCollision_collideSphere(&end, radius, inside, bone_radius, bone_length, current);
	}

	Vector3 at = p;
	vector3::scale(&at, dot / pls);
	vector3::add(&at, &head);
	return springBoneCollision_collideSphere(&at, radius, inside, bone_radius, bone_length, current);
}

/* SpringBoneCollisionPlane3D::_collide; the normal is the collider's local
   +Y, as Vector3::UP. */
static Vector3 springBoneCollision_collidePlane(const Vector3 *origin, const Matrix3 *basis,
                                                float bone_radius, const Vector3 *current)
{
	Vector3 up = { 0.0f, 1.0f, 0.0f };
	Vector3 normal = matrix3::transformVector(basis, &up);

	Vector3 to_vec = vector3::difference(current, origin);
	float distance = vector3::dot(&to_vec, &normal) - bone_radius;

	if (distance > 0.0f)
		return *current;

	Vector3 push = vector3::scaled(&normal, -distance);
	return vector3::sum(current, &push);
}

/* The center's position, in the units the chain works in. Everything else the
   module holds was scaled once at init: the owner's transform is the only
   thing it reads live, and it is in metres like everything the game declares,
   while the chain lives in the skeleton's space, which is the model's, at
   RENDER_SCALE units per metre. Only its change from frame to frame is used,
   so which point of the owner it is does not matter. */
static Vector3 springBone_centerPosition(const Transform *world)
{
	return vector3::scaled(&world->position, RENDER_SCALE);
}

/* SpringBoneSimulator3D::_process_joints, one joint. */
void springBone_apply(T3DSkeleton *skeleton, void *context)
{
	SpringBone *joint = (SpringBone *)context;
	const SpringBonesDef *setting = joint->def;

	float delta = time::get()->delta;
	if (delta <= 0.0f) return;

	/* No clip has a channel for a spring bone, so nothing writes it back
	   between frames: left alone, the pose read here would be this joint's
	   own output from the frame before, the anchor the stiffness pulls toward
	   would drift with the swing and the gravity would pile up until the bone
	   hangs. Godot's skeleton resets to the animated pose before its
	   modifiers run; for an unanimated bone that pose is the rest one. */
	skeleton->bones[joint->bone].rotation = skeleton->skeletonRef->bones[joint->bone].rotation;

	/* Transform3D current_global_pose = get_bone_global_pose(bone): the
	   composed chain, seeing what previous joints already wrote, as their
	   set_bone_pose_rotation feeds the next joint. Character3D space is the
	   center, so no center transform is left to apply. */
	T3DVec3 pose_position;
	T3DQuat pose_rotation;
	skeleton::getBonePose(skeleton, joint->bone, &pose_position, &pose_rotation);

	Vector3 current_origin = { pose_position.v[0], pose_position.v[1], pose_position.v[2] };
	Quaternion current_rot = { pose_rotation.v[0], pose_rotation.v[1], pose_rotation.v[2], pose_rotation.v[3] };
	Matrix3 current_mat = quaternion::toMatrix3(&current_rot);

	/* Vector3 external = inverted_center_rotation.xform(gravity_direction *
	   gravity * delta): gravity lives in the world, the center rotates. The
	   center's rotation is the owner's matrix as it stands, in the renderer's
	   convention. */
	const Matrix3 *center_rot = &joint->world->rotation;
	Matrix3 center_rot_t = matrix3::transposed(center_rot);
	Vector3 external = matrix3::transformVector(&center_rot_t, &setting->gravity_direction);
	vector3::scale(&external, setting->gravity * RENDER_SCALE * delta);

	Vector3 forward = matrix3::transformVector(&current_mat, &joint->forward_vector);

	if (!joint->primed) {
		Vector3 rest_tail = vector3::scaled(&forward, joint->length);
		vector3::add(&rest_tail, &current_origin);
		joint->current_tail = rest_tail;
		joint->prev_tail = rest_tail;
		joint->pre_skel_comp_pos = springBone_centerPosition(joint->world);
		joint->pre_skel_comp_rot = *center_rot;
		joint->primed = true;
	}
	else {
		/* KawaiiPhysics UpdateSkelCompMove: the owner's transform delta,
		   expressed in the current frame. Past the teleport thresholds the
		   frame is a teleport and none of it is reflected. */
		Vector3 center_position = springBone_centerPosition(joint->world);
		Vector3 rel = vector3::difference(&joint->pre_skel_comp_pos, &center_position);
		Vector3 skel_comp_move_vector = matrix3::transformVector(&center_rot_t, &rel);
		Matrix3 skel_comp_move_rot = matrix3::product(&center_rot_t, &joint->pre_skel_comp_rot);

		float dist_max = setting->teleport_distance_threshold * RENDER_SCALE;
		float trace = skel_comp_move_rot.ex.x + skel_comp_move_rot.ey.y + skel_comp_move_rot.ez.z;
		float cos_angle = (trace - 1.0f) * 0.5f;

		bool teleport =
			(setting->teleport_distance_threshold > 0.0f &&
			 vector3::squaredMagnitude(&skel_comp_move_vector) > dist_max * dist_max) ||
			(setting->teleport_rotation_threshold > 0.0f &&
			 cos_angle < cosf(setting->teleport_rotation_threshold));

		if (!teleport) {
			/* Follow Translation */
			vector3::addScaledVector(&joint->current_tail, &skel_comp_move_vector,
			                        1.0f - setting->world_damping_location);

			/* Follow Rotation */
			Vector3 rotated = matrix3::transformVector(&skel_comp_move_rot, &joint->prev_tail);
			vector3::sub(&rotated, &joint->prev_tail);
			vector3::addScaledVector(&joint->current_tail, &rotated,
			                        1.0f - setting->world_damping_rotation);
		}

		joint->pre_skel_comp_pos = center_position;
		joint->pre_skel_comp_rot = *center_rot;
	}

	/* Integration of velocity by verlet. */
	Vector3 next_tail = joint->current_tail;
	Vector3 velocity = vector3::difference(&joint->current_tail, &joint->prev_tail);
	vector3::addScaledVector(&next_tail, &velocity, 1.0f - setting->drag);
	vector3::addScaledVector(&next_tail, &forward, setting->stiffness * RENDER_SCALE * delta);
	vector3::add(&next_tail, &external);

	/* Limit bone length. */
	next_tail = springBone_limitLength(&current_origin, &next_tail, joint->length);

	/* Collision movement. */
	for (uint8_t i = 0; i < joint->collider_count; i++) {
		/* get_transform_from_skeleton: the collider rides its bone. */
		T3DVec3 collider_position;
		T3DQuat collider_rotation;
		skeleton::getBonePose(skeleton, joint->collider[i].bone, &collider_position, &collider_rotation);

		Quaternion bone_rot = { collider_rotation.v[0], collider_rotation.v[1], collider_rotation.v[2], collider_rotation.v[3] };
		Matrix3 bone_mat = quaternion::toMatrix3(&bone_rot);

		Vector3 origin = matrix3::transformVector(&bone_mat, &joint->collider[i].position);
		origin.x += collider_position.v[0];
		origin.y += collider_position.v[1];
		origin.z += collider_position.v[2];

		Matrix3 basis = matrix3::product(&bone_mat, &joint->collider[i].rotation);

		switch (joint->collider[i].shape) {
			case SPRING_BONE_COLLISION_SPHERE:
				next_tail = springBoneCollision_collideSphere(&origin, joint->collider[i].radius,
					joint->collider[i].inside, joint->radius, joint->length, &next_tail);
				break;
			case SPRING_BONE_COLLISION_CAPSULE:
				next_tail = springBoneCollision_collideCapsule(&origin, &basis, joint->collider[i].radius,
					joint->collider[i].height, joint->collider[i].inside, joint->radius, joint->length, &next_tail);
				break;
			case SPRING_BONE_COLLISION_PLANE:
				next_tail = springBoneCollision_collidePlane(&origin, &basis, joint->radius, &next_tail);
				break;
		}

		/* Limit bone length. */
		next_tail = springBone_limitLength(&current_origin, &next_tail, joint->length);
	}

	/* Store current tails for next process. */
	joint->prev_tail = joint->current_tail;
	joint->current_tail = next_tail;

	/* Flush-to-zero: as the spring settles the implied velocity decays into
	   denormal range, and every op on a denormal traps the N64 FPU into a
	   software handler. Below a hair's width the pair snaps together. */
	Vector3 settle = vector3::difference(&joint->current_tail, &joint->prev_tail);
	if (vector3::squaredMagnitude(&settle) < 1e-8f)
		joint->prev_tail = joint->current_tail;

	/* Convert position to rotation. */
	Vector3 from = forward;
	Vector3 to = vector3::difference(&next_tail, &current_origin);
	vector3::normalize(&to);

	Quaternion from_to = springBone_fromToRotation(&from, &to, &joint->current_rot);
	joint->current_rot = from_to;

	/* Apply rotation: from_to *= current_rot, then back to local pose. */
	Matrix3 from_to_mat = quaternion::toMatrix3(&from_to);
	Matrix3 global_mat = matrix3::product(&from_to_mat, &current_mat);

	Matrix3 parent_t = matrix3::identity();
	uint16_t parent = skeleton->skeletonRef->bones[joint->bone].parentIdx;
	if (parent != 0xFFFF) {
		T3DVec3 parent_position;
		T3DQuat parent_rotation;
		skeleton::getBonePose(skeleton, (int16_t)parent, &parent_position, &parent_rotation);

		Quaternion parent_rot = { parent_rotation.v[0], parent_rotation.v[1], parent_rotation.v[2], parent_rotation.v[3] };
		Matrix3 parent_mat = quaternion::toMatrix3(&parent_rot);
		parent_t = matrix3::transposed(&parent_mat);
	}

	Matrix3 local_mat = matrix3::product(&parent_t, &global_mat);
	Quaternion local = quaternion::fromMatrix3(&local_mat);

	T3DBone *bone = &skeleton->bones[joint->bone];
	bone->rotation = (T3DQuat){{ local.x, local.y, local.z, local.w }};
	bone->hasChanged = 1;
}

}
