#include <fmath.h>
#include <math.h>

#include "physics/math/matrix3.h"


Matrix3 matrix3_create(float a, float b, float c, float d, float e, float f, float g, float h, float i)
{
	return (Matrix3){
		.ex = {a, b, c},
		.ey = {d, e, f},
		.ez = {g, h, i},
	};
}

Matrix3 matrix3_fromColumns(const Vector3 *ex, const Vector3 *ey, const Vector3 *ez)
{
	return (Matrix3){ .ex = *ex, .ey = *ey, .ez = *ez };
}

Matrix3 matrix3_identity(void)
{
	return (Matrix3){
		.ex = {1.0f, 0.0f, 0.0f},
		.ey = {0.0f, 1.0f, 0.0f},
		.ez = {0.0f, 0.0f, 1.0f},
	};
}

Matrix3 matrix3_zero(void)
{
	return (Matrix3){0};
}

Matrix3 matrix3_diagonal(float x, float y, float z)
{
	return (Matrix3){
		.ex = {x, 0.0f, 0.0f},
		.ey = {0.0f, y, 0.0f},
		.ez = {0.0f, 0.0f, z},
	};
}

Matrix3 matrix3_fromAxisAngle(const Vector3 *axis, float angle)
{
	float s, c;
	fm_sincosf(angle, &s, &c);
	float x = axis->x, y = axis->y, z = axis->z;
	float xy = x * y, yz = y * z, zx = z * x;
	float t = 1.0f - c;
	return (Matrix3){
		.ex = { x*x*t + c,   xy*t + z*s, zx*t - y*s },
		.ey = { xy*t - z*s,  y*y*t + c,  yz*t + x*s },
		.ez = { zx*t + y*s,  yz*t - x*s, z*z*t + c  },
	};
}

Matrix3 matrix3_outerProduct(const Vector3 *u, const Vector3 *v)
{
	return (Matrix3){
		.ex = { v->x * u->x, v->y * u->x, v->z * u->x },
		.ey = { v->x * u->y, v->y * u->y, v->z * u->y },
		.ez = { v->x * u->z, v->y * u->z, v->z * u->z },
	};
}

Matrix3 matrix3_inverse(const Matrix3 *m)
{
	Vector3 t0 = vector3_cross(&m->ey, &m->ez);
	Vector3 t1 = vector3_cross(&m->ez, &m->ex);
	Vector3 t2 = vector3_cross(&m->ex, &m->ey);
	float detinv = 1.0f / vector3_dot(&m->ez, &t2);
	return (Matrix3){
		.ex = { t0.x * detinv, t1.x * detinv, t2.x * detinv },
		.ey = { t0.y * detinv, t1.y * detinv, t2.y * detinv },
		.ez = { t0.z * detinv, t1.z * detinv, t2.z * detinv },
	};
}

Matrix3 matrix3_difference(const Matrix3 *a, const Matrix3 *b)
{
	return (Matrix3){
		.ex = vector3_difference(&a->ex, &b->ex),
		.ey = vector3_difference(&a->ey, &b->ey),
		.ez = vector3_difference(&a->ez, &b->ez),
	};
}

void matrix3_sub(Matrix3 *m, const Matrix3 *n)
{
	vector3_sub(&m->ex, &n->ex);
	vector3_sub(&m->ey, &n->ey);
	vector3_sub(&m->ez, &n->ez);
}

void matrix3_setRows(Matrix3 *m, const Vector3 *ex, const Vector3 *ey, const Vector3 *ez)
{
	m->ex = *ex;
	m->ey = *ey;
	m->ez = *ez;
}

Vector3 matrix3_column0(const Matrix3 *m) { return (Vector3){m->ex.x, m->ey.x, m->ez.x}; }
Vector3 matrix3_column1(const Matrix3 *m) { return (Vector3){m->ex.y, m->ey.y, m->ez.y}; }
Vector3 matrix3_column2(const Matrix3 *m) { return (Vector3){m->ex.z, m->ey.z, m->ez.z}; }

float matrix3_get(const Matrix3 *m, int i, int j)
{
	const Vector3 *col = (i == 0) ? &m->ex : (i == 1) ? &m->ey : &m->ez;
	return (j == 0) ? col->x : (j == 1) ? col->y : col->z;
}


void matrix3_setIdentity(Matrix3 *m)
{
	m->ex = (Vector3){1.0f, 0.0f, 0.0f};
	m->ey = (Vector3){0.0f, 1.0f, 0.0f};
	m->ez = (Vector3){0.0f, 0.0f, 1.0f};
}

void matrix3_setZero(Matrix3 *m)
{
	m->ex = (Vector3){0.0f, 0.0f, 0.0f};
	m->ey = (Vector3){0.0f, 0.0f, 0.0f};
	m->ez = (Vector3){0.0f, 0.0f, 0.0f};
}

void matrix3_setDiagonal(Matrix3 *m, float x, float y, float z)
{
	m->ex = (Vector3){x,    0.0f, 0.0f};
	m->ey = (Vector3){0.0f, y,    0.0f};
	m->ez = (Vector3){0.0f, 0.0f, z   };
}

void matrix3_setFromEuler(Matrix3 *m, float pitch, float yaw, float roll)
{
	float sp, cp, sy, cy, sr, cr;
	fm_sincosf(pitch, &sp, &cp);
	fm_sincosf(yaw,   &sy, &cy);
	fm_sincosf(roll,  &sr, &cr);

	m->ex = (Vector3){
		 cy * cr,
		 cy * sr,
		-sy
	};
	m->ey = (Vector3){
		sp * sy * cr - cp * sr,
		sp * sy * sr + cp * cr,
		sp * cy
	};
	m->ez = (Vector3){
		cp * sy * cr + sp * sr,
		cp * sy * sr - sp * cr,
		cp * cy
	};
}

Vector3 matrix3_toEuler(const Matrix3 *m)
{
	Vector3 euler;
	float sy_p = -m->ex.z;
	float cy_p = sqrtf(m->ex.x * m->ex.x + m->ex.y * m->ex.y);

	euler.y = fm_atan2f(sy_p, cy_p);
	if (cy_p > 1e-4f) {
		euler.x = fm_atan2f(m->ey.z, m->ez.z);
		euler.z = fm_atan2f(m->ex.y, m->ex.x);
	} else {
		euler.x = 0.0f;
		euler.z = fm_atan2f(-m->ey.x, m->ey.y);
	}
	return euler;
}


void matrix3_transpose(Matrix3 *m)
{
	Matrix3 t = matrix3_transposed(m);
	*m = t;
}

void matrix3_scale(Matrix3 *m, float scalar)
{
	vector3_scale(&m->ex, scalar);
	vector3_scale(&m->ey, scalar);
	vector3_scale(&m->ez, scalar);
}

void matrix3_add(Matrix3 *m, const Matrix3 *n)
{
	vector3_add(&m->ex, &n->ex);
	vector3_add(&m->ey, &n->ey);
	vector3_add(&m->ez, &n->ez);
}

Matrix3 matrix3_transposed(const Matrix3 *m)
{
	return (Matrix3){
		.ex = {m->ex.x, m->ey.x, m->ez.x},
		.ey = {m->ex.y, m->ey.y, m->ez.y},
		.ez = {m->ex.z, m->ey.z, m->ez.z},
	};
}

Matrix3 matrix3_scaled(const Matrix3 *m, float scalar)
{
	return (Matrix3){
		.ex = vector3_scaled(&m->ex, scalar),
		.ey = vector3_scaled(&m->ey, scalar),
		.ez = vector3_scaled(&m->ez, scalar),
	};
}

Matrix3 matrix3_sum(const Matrix3 *a, const Matrix3 *b)
{
	return (Matrix3){
		.ex = vector3_sum(&a->ex, &b->ex),
		.ey = vector3_sum(&a->ey, &b->ey),
		.ez = vector3_sum(&a->ez, &b->ez),
	};
}

Matrix3 matrix3_product(const Matrix3 *a, const Matrix3 *b)
{
	return (Matrix3){
		.ex = matrix3_transformVector(a, &b->ex),
		.ey = matrix3_transformVector(a, &b->ey),
		.ez = matrix3_transformVector(a, &b->ez),
	};
}

Vector3 matrix3_transformVector(const Matrix3 *m, const Vector3 *v)
{
	return (Vector3){
		m->ex.x * v->x + m->ey.x * v->y + m->ez.x * v->z,
		m->ex.y * v->x + m->ey.y * v->y + m->ez.y * v->z,
		m->ex.z * v->x + m->ey.z * v->y + m->ez.z * v->z,
	};
}

Vector3 matrix3_transformVectorTransposed(const Matrix3 *m, const Vector3 *v)
{
	return (Vector3){
		vector3_dot(&m->ex, v),
		vector3_dot(&m->ey, v),
		vector3_dot(&m->ez, v),
	};
}
