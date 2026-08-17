/*
	Jakobsen's integration step and constraint projection ("Advanced Character
	Physics", GDC 2001). The primitives behind cloth, springs and particle
	soft bodies. Header-only: each one is a handful of multiply-adds and
	belongs inlined in its caller's loop.
*/
#ifndef VERLET_H
#define VERLET_H

#include <stdbool.h>

#include "physics/math/vector3.h"


/* x += (x - oldx) * retain + a * dt2. The position pair is the state: the
   difference is the implied velocity, and retain bleeds it (1 = none; the
   paper's damped variant is 0.99 per step). */
static inline void verlet_integrate(Vector3 *position, Vector3 *previous, Vector3 acceleration, float retain, float dt)
{
	float dt2 = dt * dt;
	Vector3 current = *position;

	position->x += (current.x - previous->x) * retain + acceleration.x * dt2;
	position->y += (current.y - previous->y) * retain + acceleration.y * dt2;
	position->z += (current.z - previous->z) * retain + acceleration.z * dt2;

	*previous = current;
}

/* First-order distance projection, no sqrt: exact at the rest length, which
   is where a satisfied constraint sits. The factor already carries each end's
   half share; a pinned end pushes the whole correction to the free one. */
static inline void verlet_projectDistance(Vector3 *a, Vector3 *b, float rest_length_sq, bool pin_a, bool pin_b)
{
	if (pin_a && pin_b) return;

	Vector3 delta = { b->x - a->x, b->y - a->y, b->z - a->z };

	float dist_sq = delta.x*delta.x + delta.y*delta.y + delta.z*delta.z;
	float scale   = rest_length_sq / (dist_sq + rest_length_sq) - 0.5f;

	if (pin_a) {
		scale *= 2.0f;
		b->x += delta.x * scale;
		b->y += delta.y * scale;
		b->z += delta.z * scale;
	}
	else if (pin_b) {
		scale *= 2.0f;
		a->x -= delta.x * scale;
		a->y -= delta.y * scale;
		a->z -= delta.z * scale;
	}
	else {
		a->x -= delta.x * scale;
		a->y -= delta.y * scale;
		a->z -= delta.z * scale;
		b->x += delta.x * scale;
		b->y += delta.y * scale;
		b->z += delta.z * scale;
	}
}

#endif
