/*
	aabb.h — axis-aligned bounding box (from qu3e q3AABB).
*/
#ifndef PHYSICS_AABB_H
#define PHYSICS_AABB_H

#include "physics/math/vector3.h"


typedef struct AABB {
	Vector3 min;
	Vector3 max;
} AABB;


int   aabb_containsAABB(const AABB *a, const AABB *other);
int   aabb_containsPoint(const AABB *a, const Vector3 *p);
float aabb_surfaceArea(const AABB *a);
int   aabb_overlaps(const AABB *a, const AABB *b);
AABB  aabb_combine(const AABB *a, const AABB *b);

Vector3 aabb_closestToPoint  (const AABB *a, const Vector3 *p);
Vector3 aabb_closestToSegment(const AABB *a, const Vector3 *p, const Vector3 *q);


#endif
