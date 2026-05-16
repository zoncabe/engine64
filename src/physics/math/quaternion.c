#include <fmath.h>
#include <math.h>
#include <assert.h>

#include "physics/math/quaternion.h"


Quaternion quaternion_create(float x, float y, float z, float w)
{
	return (Quaternion){x, y, z, w};
}

Quaternion quaternion_identity(void)
{
	return (Quaternion){0.0f, 0.0f, 0.0f, 1.0f};
}

void quaternion_setAxisAngle(Quaternion *q, const Vector3 *axis, float radians)
{
	float halfAngle = 0.5f * radians;
	float s = fm_sinf(halfAngle);
	q->x = s * axis->x;
	q->y = s * axis->y;
	q->z = s * axis->z;
	q->w = fm_cosf(halfAngle);
}

Quaternion quaternion_fromAxisAngle(const Vector3 *axis, float radians)
{
	Quaternion q;
	quaternion_setAxisAngle(&q, axis, radians);
	return q;
}

void quaternion_toAxisAngle(const Quaternion *q, Vector3 *axis, float *angle)
{
	assert(q->w <= 1.0f);
	*angle = 2.0f * acosf(q->w);
	float l = sqrtf(1.0f - q->w * q->w);
	if (l == 0.0f) {
		*axis = vector3_zero();
	} else {
		float inv = 1.0f / l;
		*axis = (Vector3){q->x * inv, q->y * inv, q->z * inv};
	}
}

Quaternion quaternion_product(const Quaternion *a, const Quaternion *b)
{
	return (Quaternion){
		a->w * b->x + a->x * b->w + a->y * b->z - a->z * b->y,
		a->w * b->y + a->y * b->w + a->z * b->x - a->x * b->z,
		a->w * b->z + a->z * b->w + a->x * b->y - a->y * b->x,
		a->w * b->w - a->x * b->x - a->y * b->y - a->z * b->z,
	};
}

Quaternion quaternion_normalized(const Quaternion *q)
{
	float x = q->x, y = q->y, z = q->z, w = q->w;
	float d = w*w + x*x + y*y + z*z;
	if (d == 0.0f) w = 1.0f;
	d = 1.0f / sqrtf(d);
	if (d > 1.0e-8f) {
		x *= d; y *= d; z *= d; w *= d;
	}
	return (Quaternion){x, y, z, w};
}

void quaternion_integrate(Quaternion *q, const Vector3 *omega, float dt)
{
	Quaternion dq = { omega->x * dt, omega->y * dt, omega->z * dt, 0.0f };
	Quaternion m = quaternion_product(&dq, q);
	q->x += m.x * 0.5f;
	q->y += m.y * 0.5f;
	q->z += m.z * 0.5f;
	q->w += m.w * 0.5f;
	*q = quaternion_normalized(q);
}

Matrix3 quaternion_toMatrix3(const Quaternion *q)
{
	float qx2 = q->x + q->x;
	float qy2 = q->y + q->y;
	float qz2 = q->z + q->z;
	float qxqx2 = q->x * qx2;
	float qxqy2 = q->x * qy2;
	float qxqz2 = q->x * qz2;
	float qxqw2 = q->w * qx2;
	float qyqy2 = q->y * qy2;
	float qyqz2 = q->y * qz2;
	float qyqw2 = q->w * qy2;
	float qzqz2 = q->z * qz2;
	float qzqw2 = q->w * qz2;
	return (Matrix3){
		.ex = { 1.0f - qyqy2 - qzqz2, qxqy2 + qzqw2,        qxqz2 - qyqw2 },
		.ey = { qxqy2 - qzqw2,        1.0f - qxqx2 - qzqz2, qyqz2 + qxqw2 },
		.ez = { qxqz2 + qyqw2,        qyqz2 - qxqw2,        1.0f - qxqx2 - qyqy2 },
	};
}
