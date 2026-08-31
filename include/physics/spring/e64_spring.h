/*
	Damped spring particle: one Verlet particle pulled toward a moving anchor.
	The cheap jiggle: gives lag, bounce and settle, no deformation. What the
	caller consumes is the clamped offset.
*/
#ifndef ENGINE64_SPRING_H
#define ENGINE64_SPRING_H

#include <stdbool.h>

#include "physics/math/e64_vector3.h"


typedef struct {

	float   stiffness;    /* pull toward the anchor, 1/s2: omega = sqrt(stiffness) */
	float   damping;      /* exponential bleed of the implied velocity, 1/s */
	float   max_offset;   /* clamp radius around the anchor */
	Vector3 gravity;      /* constant acceleration, units/s2; sag = gravity / stiffness */

} SpringSettings;

typedef struct {

	Vector3 position;
	Vector3 previous;
	bool    primed;     /* starts on the anchor, not at the origin */

} Spring;


void spring_reset(Spring *spring);

/* Steps the particle and returns position - anchor, clamped to max_offset. */
Vector3 spring_update(Spring *spring, const SpringSettings *settings, Vector3 anchor, float dt);

#endif
