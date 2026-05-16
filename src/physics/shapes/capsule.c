/*
	capsule.c — capsule methods.
*/
#include <math.h>

#include "physics/shapes/capsule.h"
#include "physics/shapes/physics_shape.h"   /* MassData */
#include "physics/math/math_common.h"        /* PI */
#include "physics/math/math_functions.h"     /* segment_closestToPoint */


void capsule_getSegment(const Capsule *c, const Transform *world, Vector3 *a, Vector3 *b)
{
	Vector3 local_top    = { 0.0f, 0.0f,  c->half_height };
	Vector3 local_bottom = { 0.0f, 0.0f, -c->half_height };
	*a = transform_mulVector(world, &local_bottom);
	*b = transform_mulVector(world, &local_top);
}


int capsule_testPoint(const Capsule *c, const Transform *world, const Vector3 *p)
{
	Vector3 a, b;
	capsule_getSegment(c, world, &a, &b);
	Vector3 closest = segment_closestToPoint(&a, &b, p);
	Vector3 d       = vector3_difference(p, &closest);
	return vector3_squaredMagnitude(&d) <= c->radius * c->radius;
}


int capsule_raycast(const Capsule *c, const Transform *world, RaycastData *ray)
{
	/* Cheap: enclose the capsule in a sphere at its center with radius = radius + half_height.
	   Good enough for broad raycast; exact capsule-ray iterates cylinder+caps. */
	float     R  = c->radius + c->half_height;
	Vector3   m  = vector3_difference(&ray->start, &world->position);
	float     b  = vector3_dot(&m, &ray->dir);
	float     cc = vector3_dot(&m, &m) - R * R;
	if (cc > 0.0f && b > 0.0f) return 0;
	float disc = b * b - cc;
	if (disc < 0.0f) return 0;
	float t = -b - sqrtf(disc);
	if (t < 0.0f) t = 0.0f;
	if (t > ray->t) return 0;

	ray->toi = t;
	Vector3 hit = vector3_scaled(&ray->dir, t);
	hit         = vector3_sum(&ray->start, &hit);
	Vector3 n   = vector3_difference(&hit, &world->position);
	ray->normal = vector3_normalized(&n);
	return 1;
}


void capsule_computeAABB(const Capsule *c, const Transform *world, AABB *aabb)
{
	Vector3 a, b;
	capsule_getSegment(c, world, &a, &b);
	Vector3 r = { c->radius, c->radius, c->radius };
	Vector3 mn = { a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z };
	Vector3 mx = { a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z };
	aabb->min = vector3_difference(&mn, &r);
	aabb->max = vector3_sum(&mx, &r);
}


void capsule_computeMass(const Capsule *c, const Transform *local, float density, MassData *md)
{
	float r   = c->radius;
	float r2  = r * r;
	float h   = 2.0f * c->half_height;

	float cyl_vol = PI * r2 * h;
	float cap_vol = (4.0f / 3.0f) * PI * r2 * r;
	float mass    = (cyl_vol + cap_vol) * density;

	float cyl_mass = cyl_vol * density;
	float cap_mass = cap_vol * density;

	/* Solid cylinder around its own axis = (1/2) m r²; perpendicular = (1/12) m (3r² + h²). */
	float Izz = 0.5f * cyl_mass * r2 + 0.4f * cap_mass * r2;
	float Ixx = cyl_mass * ( (1.0f/12.0f) * h * h + 0.25f * r2 )
	          + cap_mass * ( 0.4f * r2 + 0.5f * h * h + 0.375f * r * h );
	float Iyy = Ixx;

	Matrix3 I = matrix3_diagonal(Ixx, Iyy, Izz);

	/* Rotate to local frame. */
	Matrix3 rt  = matrix3_transposed(&local->rotation);
	Matrix3 tmp = matrix3_product(&local->rotation, &I);
	I           = matrix3_product(&tmp, &rt);

	/* Parallel axis: I += m · (|pos|² · identity - pos⊗pos). */
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


void capsuleDef_init(CapsuleDef *d)
{
	transform_init(&d->tx);
	d->radius      = 0.5f;
	d->half_height = 0.5f;
	d->friction    = 0.4f;
	d->restitution = 0.0f;
	d->density     = 1.0f;
	d->sensor      = 0;
}

void capsuleDef_set(CapsuleDef *d, const Transform *tx, float radius, float half_height)
{
	d->tx          = *tx;
	d->radius      = radius;
	d->half_height = half_height;
}

void capsuleDef_setFriction(CapsuleDef *d, float f)     { d->friction = f; }
void capsuleDef_setRestitution(CapsuleDef *d, float r)  { d->restitution = r; }
void capsuleDef_setDensity(CapsuleDef *d, float rho)    { d->density = rho; }
void capsuleDef_setSensor(CapsuleDef *d, int s)         { d->sensor = s; }
