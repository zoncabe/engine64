#include <t3d/t3dmath.h>

#include "render/e64_transform3d.h"
#include "math/e64_math.h"
#include "physics3d/e64_rigid_body.h"
#include "engine/e64_common.h" /* RENDER_SCALE */

namespace e64 {
namespace transform3d {

Transform3D identity(void)
{
	return (Transform3D){
		.position = { 0.0f, 0.0f, 0.0f },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	};
}

Transform3D fromDeclaration(const Transform3D *declared)
{
	Transform3D t = *declared;

	const Quaternion *q = &t.rotation;
	if (q->x == 0.0f && q->y == 0.0f && q->z == 0.0f && q->w == 0.0f)
		t.rotation = (Quaternion){ 0.0f, 0.0f, 0.0f, 1.0f };

	if (t.scale.x == 0.0f && t.scale.y == 0.0f && t.scale.z == 0.0f)
		t.scale = (Vector3){ 1.0f, 1.0f, 1.0f };

	return t;
}

Transform3D fromEuler(const Vector3 *position, const Vector3 *degrees, const Vector3 *scale)
{
	return (Transform3D){
		.position = *position,
		.rotation = quaternion::fromEuler(deg_to_rad(degrees->x), deg_to_rad(degrees->y), deg_to_rad(degrees->z)),
		.scale = *scale,
	};
}

Transform3D product(const Transform3D *parent, const Transform3D *child)
{
	Vector3 local = {
		parent->scale.x * child->position.x,
		parent->scale.y * child->position.y,
		parent->scale.z * child->position.z,
	};
	Vector3 moved = quaternion::rotateVector(&parent->rotation, &local);

	return (Transform3D){
		.position = { parent->position.x + moved.x, parent->position.y + moved.y, parent->position.z + moved.z },
		.rotation = quaternion::product(&parent->rotation, &child->rotation),
		.scale = { parent->scale.x * child->scale.x, parent->scale.y * child->scale.y, parent->scale.z * child->scale.z },
	};
}

Vector3 mulVector(const Transform3D *t, const Vector3 *v)
{
	Vector3 scaled = { t->scale.x * v->x, t->scale.y * v->y, t->scale.z * v->z };
	Vector3 turned = quaternion::rotateVector(&t->rotation, &scaled);

	return (Vector3){ t->position.x + turned.x, t->position.y + turned.y, t->position.z + turned.z };
}

Vector3 mulVectorInverse(const Transform3D *t, const Vector3 *v)
{
	Quaternion back = { -t->rotation.x, -t->rotation.y, -t->rotation.z, t->rotation.w };
	Vector3 offset = { v->x - t->position.x, v->y - t->position.y, v->z - t->position.z };
	Vector3 turned = quaternion::rotateVector(&back, &offset);

	return (Vector3){ turned.x / t->scale.x, turned.y / t->scale.y, turned.z / t->scale.z };
}

Transform3D inverse(const Transform3D *t)
{
	/* The origin of the scene, seen from inside the transform, is where the
	   inverse stands. */
	Vector3 origin = { 0.0f, 0.0f, 0.0f };

	return (Transform3D){
		.position = mulVectorInverse(t, &origin),
		.rotation = { -t->rotation.x, -t->rotation.y, -t->rotation.z, t->rotation.w },
		.scale = { 1.0f / t->scale.x, 1.0f / t->scale.y, 1.0f / t->scale.z },
	};
}

void toMatrix(const Transform3D *t, T3DMat4FP *out)
{
	t3d_mat4fp_from_srt(
		out,
		(float[3]){ t->scale.x, t->scale.y, t->scale.z },
		(float[4]){ t->rotation.x, t->rotation.y, t->rotation.z, t->rotation.w },
		(float[3]){ t->position.x * RENDER_SCALE, t->position.y * RENDER_SCALE, t->position.z * RENDER_SCALE }
	);
}

Transform toPhysics(const Transform3D *t)
{
	return (Transform){
		.position = t->position,
		.rotation = quaternion::toMatrix3(&t->rotation),
	};
}

Transform3D fromBody(const RigidBody *body, const Vector3 *scale)
{
	return (Transform3D){
		.position = body->tx.position,
		.rotation = body->q,
		.scale = *scale,
	};
}

}
}
