/*
	Ported from qu3e q3Box.cpp — altered source, not the original software.

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
#include <math.h>

#include "physics/shapes/e64_box.h"
#include "physics/shapes/e64_physics_shape.h"   /* MassData */


int box_testPoint(const Box *b, const Transform *world, const Vector3 *p)
{
	Vector3 p0 = transform_mulVectorTransposed(world, p);

	float pv[3] = { p0.x,  p0.y,  p0.z  };
	float ev[3] = { b->e.x, b->e.y, b->e.z };
	for (int i = 0; i < 3; ++i) {
		if (pv[i] >  ev[i]) return 0;
		if (pv[i] < -ev[i]) return 0;
	}
	return 1;
}


int box_raycast(const Box *b, const Transform *world, RaycastData *raycast)
{
	Vector3 d = matrix3_transformVectorTransposed(&world->rotation, &raycast->dir);
	Vector3 p = transform_mulVectorTransposed(world, &raycast->start);

	const float epsilon = 1.0e-8f;
	float tmin = 0.0f;
	float tmax = raycast->t;

	Vector3 n0 = vector3_zero();

	float dv[3] = { d.x, d.y, d.z };
	float pv[3] = { p.x, p.y, p.z };
	float ev[3] = { b->e.x, b->e.y, b->e.z };

	for (int i = 0; i < 3; ++i) {
		if (fabsf(dv[i]) < epsilon) {
			if (pv[i] < -ev[i] || pv[i] > ev[i]) return 0;
		}
		else {
			float d0 = 1.0f / dv[i];
			float s  = (dv[i] >= 0.0f) ? 1.0f : -1.0f;
			float ei = ev[i] * s;

			Vector3 n = vector3_zero();
			float nv[3] = { 0.0f, 0.0f, 0.0f };
			nv[i] = -s;
			n.x = nv[0]; n.y = nv[1]; n.z = nv[2];

			float t0 = -(ei + pv[i]) * d0;
			float t1 =  (ei - pv[i]) * d0;

			if (t0 > tmin) { n0 = n; tmin = t0; }
			if (t1 < tmax) { tmax = t1; }

			if (tmin > tmax) return 0;
		}
	}

	raycast->normal = matrix3_transformVector(&world->rotation, &n0);
	raycast->toi    = tmin;
	return 1;
}


void box_computeAABB(const Box *b, const Transform *world, AABB *aabb)
{
	Vector3 e = b->e;

	Vector3 v[8] = {
		{-e.x, -e.y, -e.z}, {-e.x, -e.y,  e.z},
		{-e.x,  e.y, -e.z}, {-e.x,  e.y,  e.z},
		{ e.x, -e.y, -e.z}, { e.x, -e.y,  e.z},
		{ e.x,  e.y, -e.z}, { e.x,  e.y,  e.z},
	};

	for (int i = 0; i < 8; ++i) {
		v[i] = transform_mulVector(world, &v[i]);
	}

	Vector3 mn = {  FLT_MAX,  FLT_MAX,  FLT_MAX };
	Vector3 mx = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

	for (int i = 0; i < 8; ++i) {
		if (v[i].x < mn.x) mn.x = v[i].x;
		if (v[i].y < mn.y) mn.y = v[i].y;
		if (v[i].z < mn.z) mn.z = v[i].z;
		if (v[i].x > mx.x) mx.x = v[i].x;
		if (v[i].y > mx.y) mx.y = v[i].y;
		if (v[i].z > mx.z) mx.z = v[i].z;
	}

	aabb->min = mn;
	aabb->max = mx;
}


void box_computeMass(const Box *b, const Transform *local, float density, MassData *md)
{
	Vector3 e    = b->e;
	float ex2    = 4.0f * e.x * e.x;
	float ey2    = 4.0f * e.y * e.y;
	float ez2    = 4.0f * e.z * e.z;
	float mass   = 8.0f * e.x * e.y * e.z * density;

	float ix = (1.0f / 12.0f) * mass * (ey2 + ez2);
	float iy = (1.0f / 12.0f) * mass * (ex2 + ez2);
	float iz = (1.0f / 12.0f) * mass * (ex2 + ey2);
	Matrix3 I = matrix3_diagonal(ix, iy, iz);

	/* I = R·I·Rᵀ. */
	Matrix3 rt = matrix3_transposed(&local->rotation);
	Matrix3 tmp = matrix3_product(&local->rotation, &I);
	I = matrix3_product(&tmp, &rt);

	/* Parallel-axis theorem: I += m · (|c|² · identity - c⊗c). */
	Matrix3 identity = matrix3_identity();
	float dot        = vector3_dot(&local->position, &local->position);
	Matrix3 outer    = matrix3_outerProduct(&local->position, &local->position);
	Matrix3 term     = matrix3_scaled(&identity, dot);
	term             = matrix3_difference(&term, &outer);
	Matrix3 scaled_term = matrix3_scaled(&term, mass);
	I = matrix3_sum(&I, &scaled_term);

	md->center  = local->position;
	md->inertia = I;
	md->mass    = mass;
}


void boxDef_init(BoxDef *d)
{
	transform_init(&d->tx);
	d->e           = vector3_zero();
	d->friction    = 0.4f;
	d->restitution = 0.2f;
	d->density     = 1.0f;
	d->sensor      = 0;
}


void boxDef_set(BoxDef *d, const Transform *tx, const Vector3 *full_extents)
{
	d->tx = *tx;
	d->e  = vector3_scaled(full_extents, 0.5f);
}


void boxDef_setFriction(BoxDef *d, float f)     { d->friction = f; }
void boxDef_setRestitution(BoxDef *d, float r)  { d->restitution = r; }
void boxDef_setDensity(BoxDef *d, float rho)    { d->density = rho; }
void boxDef_setSensor(BoxDef *d, int s)         { d->sensor = s; }
