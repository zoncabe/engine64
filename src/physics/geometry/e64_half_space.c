/*
	Ported from qu3e q3Geometry.cpp — altered source, not the original software.

	Copyright (c) 2014 Randy Gaul http://www.randygaul.net

	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:
	  1. The origin of this software must not be misrepresented; you must not
	     claim that you wrote the original software. If you use this software
	     in a product, an acknowledgment in the product documentation would be
	     appreciated but is not required.
	  2. Altered source versions must be plainly marked as such, and must not
	     be misrepresented as being the original software.
	  3. This notice may not be removed or altered from any source distribution.
*/

#include <math.h>

#include "physics/geometry/e64_half_space.h"


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
