#ifndef ENGINE64_MATRIX3_H
#define ENGINE64_MATRIX3_H

#include "physics/math/e64_vector3.h"


typedef struct Matrix3 {
	Vector3 ex;
	Vector3 ey;
	Vector3 ez;
} Matrix3;


Matrix3 matrix3_create(float a, float b, float c, float d, float e, float f, float g, float h, float i);
Matrix3 matrix3_fromColumns(const Vector3 *ex, const Vector3 *ey, const Vector3 *ez);
Matrix3 matrix3_identity(void);
Matrix3 matrix3_zero(void);
Matrix3 matrix3_diagonal(float x, float y, float z);
Matrix3 matrix3_fromAxisAngle(const Vector3 *axis, float angle);
Matrix3 matrix3_outerProduct(const Vector3 *u, const Vector3 *v);
Matrix3 matrix3_inverse(const Matrix3 *m);
Matrix3 matrix3_difference(const Matrix3 *a, const Matrix3 *b);

void matrix3_setIdentity(Matrix3 *m);
void matrix3_setZero(Matrix3 *m);
void matrix3_setDiagonal(Matrix3 *m, float x, float y, float z);
void matrix3_setFromEuler(Matrix3 *m, float pitch, float yaw, float roll);
void matrix3_setRows(Matrix3 *m, const Vector3 *ex, const Vector3 *ey, const Vector3 *ez);
Vector3 matrix3_toEuler(const Matrix3 *m);

void matrix3_transpose(Matrix3 *m);
void matrix3_scale(Matrix3 *m, float scalar);
void matrix3_add(Matrix3 *m, const Matrix3 *n);
void matrix3_sub(Matrix3 *m, const Matrix3 *n);

Matrix3 matrix3_transposed(const Matrix3 *m);
Matrix3 matrix3_scaled(const Matrix3 *m, float scalar);
Matrix3 matrix3_sum(const Matrix3 *a, const Matrix3 *b);
Matrix3 matrix3_product(const Matrix3 *a, const Matrix3 *b);

Vector3 matrix3_transformVector(const Matrix3 *m, const Vector3 *v);
Vector3 matrix3_transformVectorTransposed(const Matrix3 *m, const Vector3 *v);

Vector3 matrix3_column0(const Matrix3 *m);
Vector3 matrix3_column1(const Matrix3 *m);
Vector3 matrix3_column2(const Matrix3 *m);
float   matrix3_get(const Matrix3 *m, int i, int j);


#endif
