/*
	raycast.h — ray with impact info (from qu3e q3RaycastData).
*/
#ifndef PHYSICS_RAYCAST_H
#define PHYSICS_RAYCAST_H

#include "physics/math/vector3.h"


typedef struct RaycastData {
	Vector3 start;
	Vector3 dir;
	float   t;
	float   toi;
	Vector3 normal;
} RaycastData;


void    raycast_set(RaycastData *r, const Vector3 *start, const Vector3 *dir, float endTime);
Vector3 raycast_getImpactPoint(const RaycastData *r);

#endif
