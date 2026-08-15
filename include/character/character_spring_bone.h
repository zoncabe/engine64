/*
	character_spring_bone.h — port of Godot's SpringBoneSimulator3D
	(scene/3d/spring_bone_simulator_3d.cpp) as a skeleton modifier: one
	verlet tail per joint, stepped once per frame with the frame's delta,
	simulated in character space (the "center") so only the animation and
	gravity move it. A set listed root to tip simulates as a chain: each
	joint reads the pose the previous one already wrote.
*/
#ifndef CHARACTER_SPRING_BONE_H
#define CHARACTER_SPRING_BONE_H

#include <stdint.h>
#include <stdbool.h>
#include <t3d/t3dskeleton.h>

#include "physics/math/vector3.h"
#include "physics/math/matrix3.h"
#include "physics/math/quaternion.h"
#include "render/render.h"


#define SPRING_BONE_COLLIDER_MAX 4

/* Godot's SpringBoneCollision3D nodes: a primitive hung from a skeleton
   bone, hand-placed to approximate the mesh. Bone-local position and euler
   rotation (degrees); the capsule's axis and the plane's normal are the
   collider's local +Y, as their Vector3::UP. Metres. */
typedef enum {

	SPRING_BONE_COLLISION_SPHERE,
	SPRING_BONE_COLLISION_CAPSULE,
	SPRING_BONE_COLLISION_PLANE,

} SpringBoneCollisionShape;

typedef struct {

	SpringBoneCollisionShape shape;
	const char *bone;
	Vector3     position;
	Vector3     rotation;
	float       radius;   /* sphere and capsule */
	float       height;   /* capsule */
	bool        inside;   /* sphere and capsule: keep the tail inside */

} SpringBoneColliderDef;

/* Godot's per-joint properties, one tuning per set. Metres and seconds,
   like the rest of physics. */
typedef struct {

	/* One set is one chain, as a Godot setting: the joints are the bones
	   from root to end, resolved by walking the skeleton. count must match
	   for the owner's allocation; root == end is a single-joint chain. */
	const char *root_bone;
	const char *end_bone;
	uint8_t     count;
	float       end_bone_length;          /* tail extension of the last joint, metres */
	float              stiffness;         /* pull toward the animated pose, m/s */
	float              drag;              /* velocity kept per frame is 1 - drag, 0..1 */
	float              gravity;           /* m/s, velocity gained per second */
	Vector3            gravity_direction; /* world space, usually {0, 0, -1} */

	/* KawaiiPhysics WorldDamping: the owner's world motion enters the
	   simulation scaled by 1 - damping (0 = full sway, 1 = none). A frame
	   whose motion exceeds the teleport thresholds is not reflected at all;
	   0 disables a threshold. Metres / radians. */
	float world_damping_location;
	float world_damping_rotation;
	float teleport_distance_threshold;
	float teleport_rotation_threshold;

	/* Optional collision: every joint's tail keeps its radius away from
	   each collider in the list. radius is the uniform thickness; a
	   joint_radius array of count entries overrides it per joint, as
	   Godot's individual_config. */
	float                        radius;   /* metres */
	const float                 *joint_radius;
	const SpringBoneColliderDef *collider;
	uint8_t                      collider_count;

} SpringBonesDef;

/* Godot's SpringBone3DVerletInfo plus what _process_joints reads per joint. */
typedef struct {

	Vector3    forward_vector;   /* toward the child in rest pose, unit */
	float      length;           /* render units, from the child's rest offset */
	Vector3    current_tail;     /* character space */
	Vector3    prev_tail;
	Quaternion current_rot;      /* last from-to rotation, the degenerate fallback */

	const SpringBonesDef  *def;
	const RenderTransform *world;   /* the center */
	int16_t bone;
	float   radius;                 /* render units */
	bool    primed;

	Vector3 pre_skel_comp_pos;   /* owner's transform last frame, for the */
	Matrix3 pre_skel_comp_rot;   /* damped world-motion follow */

	/* def colliders resolved against the skeleton, render units */
	struct {
		SpringBoneCollisionShape shape;
		int16_t bone;
		Vector3 position;
		Matrix3 rotation;
		float   radius;
		float   height;
		bool    inside;
	} collider[SPRING_BONE_COLLIDER_MAX];
	uint8_t collider_count;

} SpringBone;


/* The def's chain resolved against the skeleton, root to end along the
   parents, as Godot's joint setup; 0 when a bone is missing or end does not
   descend from root. */
uint8_t springBones_resolveChain(const T3DSkeleton *skeleton, const SpringBonesDef *def,
                                 int16_t *joints, uint8_t max);

bool springBone_init(SpringBone *spring_bone, const T3DSkeleton *skeleton, int16_t bone,
                     uint8_t joint_index, const SpringBonesDef *def, const RenderTransform *world);

/* SkeletonModifierFn; context is the SpringBone */
void springBone_apply(T3DSkeleton *skeleton, void *context);

#endif
