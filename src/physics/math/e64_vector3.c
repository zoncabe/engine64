#include <math.h>

#include "physics/math/e64_vector3.h"


Vector3 vector3_create(float x, float y, float z)
{
	return (Vector3){x, y, z};
}

Vector3 vector3_zero(void)
{
	return (Vector3){0.0f, 0.0f, 0.0f};
}

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

void vector3_add(Vector3 *v, const Vector3 *w)
{
	v->x += w->x;
	v->y += w->y;
	v->z += w->z;
}

void vector3_sub(Vector3 *v, const Vector3 *w)
{
	v->x -= w->x;
	v->y -= w->y;
	v->z -= w->z;
}

void vector3_invert(Vector3 *v)
{
	v->x = -v->x;
	v->y = -v->y;
	v->z = -v->z;
}

void vector3_normalize(Vector3 *v)
{
	float mag = vector3_magnitude(v);
	if (mag <= 0.0f) return;
	float inv = 1.0f / mag;
	v->x *= inv;
	v->y *= inv;
	v->z *= inv;
}

Vector3 vector3_sum(const Vector3 *a, const Vector3 *b)
{
	return (Vector3){a->x + b->x, a->y + b->y, a->z + b->z};
}

Vector3 vector3_difference(const Vector3 *a, const Vector3 *b)
{
	return (Vector3){a->x - b->x, a->y - b->y, a->z - b->z};
}

Vector3 vector3_scaled(const Vector3 *v, float scalar)
{
	return (Vector3){v->x * scalar, v->y * scalar, v->z * scalar};
}

Vector3 vector3_inverted(const Vector3 *v)
{
	return (Vector3){-v->x, -v->y, -v->z};
}

Vector3 vector3_abs(const Vector3 *v)
{
	return (Vector3){ fabsf(v->x), fabsf(v->y), fabsf(v->z) };
}

Vector3 vector3_cross(const Vector3 *a, const Vector3 *b)
{
	return (Vector3){
		a->y * b->z - a->z * b->y,
		a->z * b->x - a->x * b->z,
		a->x * b->y - a->y * b->x,
	};
}

Vector3 vector3_normalized(const Vector3 *v)
{
	Vector3 out = *v;
	vector3_normalize(&out);
	return out;
}

float vector3_dot(const Vector3 *a, const Vector3 *b)
{
	return a->x * b->x + a->y * b->y + a->z * b->z;
}

float vector3_squaredMagnitude(const Vector3 *v)
{
	return v->x * v->x + v->y * v->y + v->z * v->z;
}

float vector3_magnitude(const Vector3 *v)
{
	return sqrtf(vector3_squaredMagnitude(v));
}

Vector3 vector3_reflected(const Vector3 *v, const Vector3 *normal)
{
	float t = 2.0f * vector3_dot(v, normal);
	return (Vector3){
		v->x - t * normal->x,
		v->y - t * normal->y,
		v->z - t * normal->z,
	};
}
