/*
	half_space.h — plane as (normal, distance from origin) (from qu3e q3HalfSpace).
*/
#ifndef PHYSICS_HALF_SPACE_H
#define PHYSICS_HALF_SPACE_H

#include "physics/math/vector3.h"


typedef struct HalfSpace {
	Vector3 normal;
	float   distance;
} HalfSpace;


HalfSpace halfSpace_create(const Vector3 *normal, float distance);
void      halfSpace_setFromTriangle(HalfSpace *h, const Vector3 *a, const Vector3 *b, const Vector3 *c);
void      halfSpace_setFromNormalPoint(HalfSpace *h, const Vector3 *n, const Vector3 *p);
Vector3   halfSpace_origin(const HalfSpace *h);
float     halfSpace_distance(const HalfSpace *h, const Vector3 *p);
Vector3   halfSpace_projected(const HalfSpace *h, const Vector3 *p);

void      vector3_computeBasis(const Vector3 *a, Vector3 *b, Vector3 *c);


#endif
