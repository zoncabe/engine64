/*
	Physics transform: position + rotation matrix. (from qu3e q3Transform).
	The render-side transform (pos+euler+scale) lives in render/render.h as
	RenderTransform.
*/
#ifndef ENGINE64_TRANSFORM_H
#define ENGINE64_TRANSFORM_H

#include "physics/math/e64_vector3.h"
#include "physics/math/e64_matrix3.h"
#include "physics/geometry/e64_half_space.h"


typedef struct Transform {
	Vector3 position;
	Matrix3 rotation;
} Transform;


void      transform_init(Transform *t);
Transform transform_inverse(const Transform *t);

Vector3   transform_mulVector(const Transform *t, const Vector3 *v);
Vector3   transform_mulVectorScaled(const Transform *t, const Vector3 *scale, const Vector3 *v);

Transform transform_product(const Transform *t, const Transform *u);

Vector3   transform_mulVectorTransposed(const Transform *t, const Vector3 *v);
Transform transform_productTransposed(const Transform *t, const Transform *u);

HalfSpace transform_mulHalfSpace(const Transform *t, const HalfSpace *p);
HalfSpace transform_mulHalfSpaceScaled(const Transform *t, const Vector3 *scale, const HalfSpace *p);
HalfSpace transform_mulHalfSpaceTransposed(const Transform *t, const HalfSpace *p);


#endif
