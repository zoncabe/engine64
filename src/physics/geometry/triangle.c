#include <math.h>

#include "physics/geometry/triangle.h"


/* Möller–Trumbore. A back face is a miss: a surface only counts from the side
   its normal points to, so a floor is floor from above and nothing from below.
   Same convention as the shape raycasts: ray->t bounds the ray, and a hit
   writes the distance along dir into ray->toi. */
int triangle_raycast(const Triangle *t, RaycastData *ray)
{
	Vector3 ab = vector3_difference(&t->vertices[1], &t->vertices[0]);
	Vector3 ac = vector3_difference(&t->vertices[2], &t->vertices[0]);

	Vector3 p   = vector3_cross(&ray->dir, &ac);
	float   det = vector3_dot(&ab, &p);
	if (det < 1.0e-6f) return 0;   /* parallel to the face, or coming from behind it */

	float   inv = 1.0f / det;
	Vector3 ap  = vector3_difference(&ray->start, &t->vertices[0]);

	float u = vector3_dot(&ap, &p) * inv;
	if (u < 0.0f || u > 1.0f) return 0;

	Vector3 q = vector3_cross(&ap, &ab);
	float   v = vector3_dot(&ray->dir, &q) * inv;
	if (v < 0.0f || u + v > 1.0f) return 0;

	float toi = vector3_dot(&ac, &q) * inv;
	if (toi < 0.0f || toi > ray->t) return 0;

	ray->toi    = toi;
	ray->normal = t->normal;
	return 1;
}
