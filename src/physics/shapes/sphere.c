/*
	sphere.c — sphere methods.
*/
#include <math.h>

#include "physics/shapes/sphere.h"
#include "physics/shapes/physics_shape.h"   /* MassData */
#include "physics/math/math_common.h"        /* PI */


int sphere_testPoint(const Sphere *s, const Transform *world, const Vector3 *p)
{
	Vector3 d = vector3_difference(p, &world->position);
	return vector3_squaredMagnitude(&d) <= s->radius * s->radius;
}


int sphere_raycast(const Sphere *s, const Transform *world, RaycastData *ray)
{
	Vector3 m  = vector3_difference(&ray->start, &world->position);
	float   b  = vector3_dot(&m, &ray->dir);
	float   c  = vector3_dot(&m, &m) - s->radius * s->radius;

	if (c > 0.0f && b > 0.0f) return 0;
	float disc = b * b - c;
	if (disc < 0.0f) return 0;

	float t = -b - sqrtf(disc);
	if (t < 0.0f) t = 0.0f;
	if (t > ray->t) return 0;

	ray->toi = t;
	Vector3 hit  = vector3_scaled(&ray->dir, t);
	hit          = vector3_sum(&ray->start, &hit);
	Vector3 n    = vector3_difference(&hit, &world->position);
	ray->normal  = vector3_normalized(&n);
	return 1;
}


void sphere_computeAABB(const Sphere *s, const Transform *world, AABB *aabb)
{
	Vector3 r = { s->radius, s->radius, s->radius };
	aabb->min = vector3_difference(&world->position, &r);
	aabb->max = vector3_sum(&world->position, &r);
}


void sphere_computeMass(const Sphere *s, const Transform *local, float density, MassData *md)
{
	float r   = s->radius;
	float r2  = r * r;
	float vol = (4.0f / 3.0f) * PI * r2 * r;
	float mass = vol * density;

	/* Solid sphere: I = (2/5) · m · r² on each axis. */
	float i = 0.4f * mass * r2;
	Matrix3 I = matrix3_diagonal(i, i, i);

	/* Parallel axis: I += m · (|c|² · I - c⊗c). */
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


void sphereDef_init(SphereDef *d)
{
	transform_init(&d->tx);
	d->radius      = 0.5f;
	d->friction    = 0.4f;
	d->restitution = 0.2f;
	d->density     = 1.0f;
	d->sensor      = 0;
}

void sphereDef_set(SphereDef *d, const Transform *tx, float radius)
{
	d->tx     = *tx;
	d->radius = radius;
}

void sphereDef_setFriction(SphereDef *d, float f)     { d->friction = f; }
void sphereDef_setRestitution(SphereDef *d, float r)  { d->restitution = r; }
void sphereDef_setDensity(SphereDef *d, float rho)    { d->density = rho; }
void sphereDef_setSensor(SphereDef *d, int s)         { d->sensor = s; }
