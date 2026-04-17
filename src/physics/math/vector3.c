#include "physics/math/vector3.h"


void vector3_scale(Vector3 *v, float scalar)
{
	v->x *= scalar;
	v->y *= scalar;
	v->z *= scalar;
}

void vector3_addScaledVector(Vector3 *v, const Vector3 *w, float scalar)
{
	v->x += w->x * scalar;
	v->y += w->y * scalar;
	v->z += w->z * scalar;
}
