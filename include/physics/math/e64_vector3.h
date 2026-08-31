#ifndef ENGINE64_VECTOR3_H
#define ENGINE64_VECTOR3_H


typedef struct Vector3 {
	float x;
	float y;
	float z;
} Vector3;


Vector3 vector3_create(float x, float y, float z);
Vector3 vector3_zero(void);

void vector3_scale(Vector3 *v, float scalar);
void vector3_addScaledVector(Vector3 *v, const Vector3 *w, float scalar);

void vector3_add(Vector3 *v, const Vector3 *w);
void vector3_sub(Vector3 *v, const Vector3 *w);
void vector3_invert(Vector3 *v);
void vector3_normalize(Vector3 *v);

Vector3 vector3_sum(const Vector3 *a, const Vector3 *b);
Vector3 vector3_difference(const Vector3 *a, const Vector3 *b);
Vector3 vector3_scaled(const Vector3 *v, float scalar);
Vector3 vector3_inverted(const Vector3 *v);
Vector3 vector3_abs(const Vector3 *v);
Vector3 vector3_cross(const Vector3 *a, const Vector3 *b);
Vector3 vector3_normalized(const Vector3 *v);
Vector3 vector3_reflected(const Vector3 *v, const Vector3 *normal);

float vector3_dot(const Vector3 *a, const Vector3 *b);
float vector3_squaredMagnitude(const Vector3 *v);
float vector3_magnitude(const Vector3 *v);


#endif
