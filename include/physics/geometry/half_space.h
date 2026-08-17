/*
	Ported from qu3e q3Geometry.h — altered source, not the original software.

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

/*
	Plane as (normal, distance from origin).
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
