#ifndef VECTOR_3_H
#define VECTOR_3_H


typedef struct Vector3 {
	float x;
	float y;
	float z;
} Vector3;


void vector3_scale(Vector3 *v, float scalar);
void vector3_addScaledVector(Vector3 *v, const Vector3 *w, float scalar);


#endif
