#include <math.h>

#include "physics/spring/spring.h"
#include "physics/math/verlet.h"


void spring_reset(Spring *spring)
{
	*spring = (Spring){0};
}


Vector3 spring_update(Spring *spring, const SpringSettings *settings, Vector3 anchor, float dt)
{
	if (!spring->primed) {
		spring->position = anchor;
		spring->previous = anchor;
		spring->primed   = true;
	}

	Vector3 to_anchor = vector3_difference(&anchor, &spring->position);
	Vector3 accel     = vector3_scaled(&to_anchor, settings->stiffness);
	vector3_add(&accel, &settings->gravity);
	float   retain    = expf(-settings->damping * dt);

	verlet_integrate(&spring->position, &spring->previous, accel, retain, dt);

	/* Clamp to the radius: the guarantee that it never detaches. */
	Vector3 offset = vector3_difference(&spring->position, &anchor);
	float dist_sq  = vector3_squaredMagnitude(&offset);
	float max_sq   = settings->max_offset * settings->max_offset;

	if (dist_sq > max_sq && dist_sq > 0.0f) {
		vector3_scale(&offset, settings->max_offset / sqrtf(dist_sq));
		spring->position = vector3_sum(&anchor, &offset);
	}

	return offset;
}
