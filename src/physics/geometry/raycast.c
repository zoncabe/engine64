#include "physics/geometry/raycast.h"


void raycast_set(RaycastData *r, const Vector3 *start, const Vector3 *dir, float endTime)
{
	r->start = *start;
	r->dir   = *dir;
	r->t     = endTime;
}

Vector3 raycast_getImpactPoint(const RaycastData *r)
{
	Vector3 off = vector3_scaled(&r->dir, r->toi);
	return vector3_sum(&r->start, &off);
}
