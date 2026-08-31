#ifndef ENGINE64_MATH_FUNCTIONS_H
#define ENGINE64_MATH_FUNCTIONS_H

#include "physics/math/e64_vector3.h"


float clampf(float v, float lo, float hi);

Vector3 segment_closestToPoint(const Vector3 *a, const Vector3 *b, const Vector3 *point);

void segment_closestToSegment(
	const Vector3 *a1, const Vector3 *b1,
	const Vector3 *a2, const Vector3 *b2,
	Vector3 *closest1, Vector3 *closest2);

void triangle_barycentric(
	const Vector3 *a, const Vector3 *b, const Vector3 *c,
	const Vector3 *point,
	float *u, float *v, float *w);

Vector3 triangle_closestToPoint(
	const Vector3 *a, const Vector3 *b, const Vector3 *c,
	const Vector3 *point);


#endif
