#include "physics/math/transform.h"


void transform_init(Transform *t)
{
	t->position = vector3_zero();
	t->rotation = matrix3_identity();
}


Transform transform_inverse(const Transform *t)
{
	Transform inv;
	inv.rotation = matrix3_transposed(&t->rotation);
	Vector3 neg  = vector3_inverted(&t->position);
	inv.position = matrix3_transformVector(&inv.rotation, &neg);
	return inv;
}


Vector3 transform_mulVector(const Transform *t, const Vector3 *v)
{
	Vector3 r = matrix3_transformVector(&t->rotation, v);
	return vector3_sum(&r, &t->position);
}


Vector3 transform_mulVectorScaled(const Transform *t, const Vector3 *scale, const Vector3 *v)
{
	Vector3 scaled = { scale->x * v->x, scale->y * v->y, scale->z * v->z };
	Vector3 r      = matrix3_transformVector(&t->rotation, &scaled);
	return vector3_sum(&r, &t->position);
}


Transform transform_product(const Transform *t, const Transform *u)
{
	Transform out;
	out.rotation = matrix3_product(&t->rotation, &u->rotation);
	Vector3 rp   = matrix3_transformVector(&t->rotation, &u->position);
	out.position = vector3_sum(&rp, &t->position);
	return out;
}


Vector3 transform_mulVectorTransposed(const Transform *t, const Vector3 *v)
{
	Vector3 d = vector3_difference(v, &t->position);
	return matrix3_transformVectorTransposed(&t->rotation, &d);
}


Transform transform_productTransposed(const Transform *t, const Transform *u)
{
	Transform out;
	Matrix3 rt = matrix3_transposed(&t->rotation);
	out.rotation = matrix3_product(&rt, &u->rotation);
	Vector3 d = vector3_difference(&u->position, &t->position);
	out.position = matrix3_transformVector(&rt, &d);
	return out;
}


HalfSpace transform_mulHalfSpace(const Transform *t, const HalfSpace *p)
{
	Vector3 origin = halfSpace_origin(p);
	Vector3 worldOrigin = transform_mulVector(t, &origin);
	Vector3 worldNormal = matrix3_transformVector(&t->rotation, &p->normal);
	HalfSpace out;
	out.normal   = worldNormal;
	out.distance = vector3_dot(&worldOrigin, &worldNormal);
	return out;
}


HalfSpace transform_mulHalfSpaceScaled(const Transform *t, const Vector3 *scale, const HalfSpace *p)
{
	Vector3 origin = halfSpace_origin(p);
	Vector3 worldOrigin = transform_mulVectorScaled(t, scale, &origin);
	Vector3 worldNormal = matrix3_transformVector(&t->rotation, &p->normal);
	HalfSpace out;
	out.normal   = worldNormal;
	out.distance = vector3_dot(&worldOrigin, &worldNormal);
	return out;
}


HalfSpace transform_mulHalfSpaceTransposed(const Transform *t, const HalfSpace *p)
{
	Vector3 planeOrigin = vector3_scaled(&p->normal, p->distance);
	Vector3 localOrigin = transform_mulVectorTransposed(t, &planeOrigin);
	Vector3 localNormal = matrix3_transformVectorTransposed(&t->rotation, &p->normal);
	HalfSpace out;
	out.normal   = localNormal;
	out.distance = vector3_dot(&localOrigin, &localNormal);
	return out;
}
