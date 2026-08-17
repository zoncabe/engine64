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

#include <float.h>

#include "physics/geometry/aabb.h"


int aabb_containsAABB(const AABB *a, const AABB *other)
{
	return
		a->min.x <= other->min.x &&
		a->min.y <= other->min.y &&
		a->min.z <= other->min.z &&
		a->max.x >= other->max.x &&
		a->max.y >= other->max.y &&
		a->max.z >= other->max.z;
}

int aabb_containsPoint(const AABB *a, const Vector3 *p)
{
	return
		a->min.x <= p->x && a->min.y <= p->y && a->min.z <= p->z &&
		a->max.x >= p->x && a->max.y >= p->y && a->max.z >= p->z;
}

float aabb_surfaceArea(const AABB *a)
{
	float x = a->max.x - a->min.x;
	float y = a->max.y - a->min.y;
	float z = a->max.z - a->min.z;
	return 2.0f * (x*y + x*z + y*z);
}

int aabb_overlaps(const AABB *a, const AABB *b)
{
	if (a->max.x < b->min.x || a->min.x > b->max.x) return 0;
	if (a->max.y < b->min.y || a->min.y > b->max.y) return 0;
	if (a->max.z < b->min.z || a->min.z > b->max.z) return 0;
	return 1;
}

AABB aabb_combine(const AABB *a, const AABB *b)
{
	AABB c;
	c.min = (Vector3){
		a->min.x < b->min.x ? a->min.x : b->min.x,
		a->min.y < b->min.y ? a->min.y : b->min.y,
		a->min.z < b->min.z ? a->min.z : b->min.z,
	};
	c.max = (Vector3){
		a->max.x > b->max.x ? a->max.x : b->max.x,
		a->max.y > b->max.y ? a->max.y : b->max.y,
		a->max.z > b->max.z ? a->max.z : b->max.z,
	};
	return c;
}


Vector3 aabb_closestToPoint(const AABB *a, const Vector3 *p)
{
	Vector3 c;
	c.x = p->x < a->min.x ? a->min.x : (p->x > a->max.x ? a->max.x : p->x);
	c.y = p->y < a->min.y ? a->min.y : (p->y > a->max.y ? a->max.y : p->y);
	c.z = p->z < a->min.z ? a->min.z : (p->z > a->max.z ? a->max.z : p->z);
	return c;
}


/* Closest point on the AABB to the segment [a, b]. Ported from the old engine
   (ODE-style iterative clipping against the box slabs). */
Vector3 aabb_closestToSegment(const AABB *aabb, const Vector3 *a, const Vector3 *b)
{
	Vector3 center    = { 0.5f * (aabb->min.x + aabb->max.x),
	                      0.5f * (aabb->min.y + aabb->max.y),
	                      0.5f * (aabb->min.z + aabb->max.z) };
	Vector3 half_size = { 0.5f * (aabb->max.x - aabb->min.x),
	                      0.5f * (aabb->max.y - aabb->min.y),
	                      0.5f * (aabb->max.z - aabb->min.z) };

	Vector3 s    = vector3_difference(a, &center);
	Vector3 v    = vector3_difference(b, a);
	Vector3 sign = { 1.0f, 1.0f, 1.0f };

	if (v.x < 0.0f) { s.x = -s.x; v.x = -v.x; sign.x = -1.0f; }
	if (v.y < 0.0f) { s.y = -s.y; v.y = -v.y; sign.y = -1.0f; }
	if (v.z < 0.0f) { s.z = -s.z; v.z = -v.z; sign.z = -1.0f; }

	Vector3 v2 = { v.x * v.x, v.y * v.y, v.z * v.z };
	Vector3 tanchor = { 2.0f, 2.0f, 2.0f };
	Vector3 region  = { 0.0f, 0.0f, 0.0f };

	if (v.x > FLT_MIN) {
		if      (s.x < -half_size.x) { region.x = -1.0f; tanchor.x = (-half_size.x - s.x) / v.x; }
		else if (s.x >  half_size.x) { region.x =  1.0f; tanchor.x = ( half_size.x - s.x) / v.x; }
	}
	if (v.y > FLT_MIN) {
		if      (s.y < -half_size.y) { region.y = -1.0f; tanchor.y = (-half_size.y - s.y) / v.y; }
		else if (s.y >  half_size.y) { region.y =  1.0f; tanchor.y = ( half_size.y - s.y) / v.y; }
	}
	if (v.z > FLT_MIN) {
		if      (s.z < -half_size.z) { region.z = -1.0f; tanchor.z = (-half_size.z - s.z) / v.z; }
		else if (s.z >  half_size.z) { region.z =  1.0f; tanchor.z = ( half_size.z - s.z) / v.z; }
	}

	float t = 0.0f;
	float dd2dt = 0.0f;
	if (region.x != 0.0f) dd2dt -= v2.x * tanchor.x;
	if (region.y != 0.0f) dd2dt -= v2.y * tanchor.y;
	if (region.z != 0.0f) dd2dt -= v2.z * tanchor.z;

	if (dd2dt < 0.0f) {
		for (;;) {
			float next_t = 1.0f;
			if (tanchor.x > t && tanchor.x < 1.0f && tanchor.x < next_t) next_t = tanchor.x;
			if (tanchor.y > t && tanchor.y < 1.0f && tanchor.y < next_t) next_t = tanchor.y;
			if (tanchor.z > t && tanchor.z < 1.0f && tanchor.z < next_t) next_t = tanchor.z;

			float next_dd2dt = 0.0f;
			next_dd2dt += (region.x != 0.0f ? v2.x : 0.0f) * (next_t - tanchor.x);
			next_dd2dt += (region.y != 0.0f ? v2.y : 0.0f) * (next_t - tanchor.y);
			next_dd2dt += (region.z != 0.0f ? v2.z : 0.0f) * (next_t - tanchor.z);

			if (next_dd2dt >= 0.0f) {
				float m = (next_dd2dt - dd2dt) / (next_t - t);
				t -= dd2dt / m;
				break;
			}

			if (tanchor.x == next_t) { tanchor.x = ( half_size.x - s.x) / v.x; region.x += 1.0f; }
			if (tanchor.y == next_t) { tanchor.y = ( half_size.y - s.y) / v.y; region.y += 1.0f; }
			if (tanchor.z == next_t) { tanchor.z = ( half_size.z - s.z) / v.z; region.z += 1.0f; }

			t     = next_t;
			dd2dt = next_dd2dt;
			if (t >= 1.0f) { t = 1.0f; break; }
		}
	}

	Vector3 tmp = {
		sign.x * (s.x + t * v.x),
		sign.y * (s.y + t * v.y),
		sign.z * (s.z + t * v.z),
	};

	if (tmp.x < -half_size.x) tmp.x = -half_size.x;
	else if (tmp.x > half_size.x) tmp.x = half_size.x;
	if (tmp.y < -half_size.y) tmp.y = -half_size.y;
	else if (tmp.y > half_size.y) tmp.y = half_size.y;
	if (tmp.z < -half_size.z) tmp.z = -half_size.z;
	else if (tmp.z > half_size.z) tmp.z = half_size.z;

	return (Vector3){ tmp.x + center.x, tmp.y + center.y, tmp.z + center.z };
}
