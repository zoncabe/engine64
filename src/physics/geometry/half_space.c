#include <math.h>

#include "physics/geometry/half_space.h"


HalfSpace halfSpace_create(const Vector3 *normal, float distance)
{
	return (HalfSpace){ .normal = *normal, .distance = distance };
}

void halfSpace_setFromTriangle(HalfSpace *h, const Vector3 *a, const Vector3 *b, const Vector3 *c)
{
	Vector3 ab = vector3_difference(b, a);
	Vector3 ac = vector3_difference(c, a);
	Vector3 n  = vector3_cross(&ab, &ac);
	h->normal   = vector3_normalized(&n);
	h->distance = vector3_dot(&h->normal, a);
}

void halfSpace_setFromNormalPoint(HalfSpace *h, const Vector3 *n, const Vector3 *p)
{
	h->normal   = vector3_normalized(n);
	h->distance = vector3_dot(&h->normal, p);
}

Vector3 halfSpace_origin(const HalfSpace *h)
{
	return vector3_scaled(&h->normal, h->distance);
}

float halfSpace_distance(const HalfSpace *h, const Vector3 *p)
{
	return vector3_dot(&h->normal, p) - h->distance;
}

Vector3 halfSpace_projected(const HalfSpace *h, const Vector3 *p)
{
	Vector3 off = vector3_scaled(&h->normal, halfSpace_distance(h, p));
	return vector3_difference(p, &off);
}


void vector3_computeBasis(const Vector3 *a, Vector3 *b, Vector3 *c)
{
	if (fabsf(a->x) >= 0.57735027f) {
		*b = (Vector3){ a->y, -a->x, 0.0f };
	} else {
		*b = (Vector3){ 0.0f, a->z, -a->y };
	}
	*b = vector3_normalized(b);
	*c = vector3_cross(a, b);
}
