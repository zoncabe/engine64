#ifndef QUATERNION_H
#define QUATERNION_H

#include "physics/math/vector3.h"
#include "physics/math/matrix3.h"


typedef struct Quaternion {
	float x, y, z, w;
} Quaternion;


Quaternion quaternion_create(float x, float y, float z, float w);
Quaternion quaternion_identity(void);
Quaternion quaternion_fromAxisAngle(const Vector3 *axis, float radians);
Quaternion quaternion_product(const Quaternion *a, const Quaternion *b);
Quaternion quaternion_normalized(const Quaternion *q);
Quaternion quaternion_nlerp(const Quaternion *a, const Quaternion *b, float t);

void quaternion_setAxisAngle(Quaternion *q, const Vector3 *axis, float radians);
void quaternion_toAxisAngle(const Quaternion *q, Vector3 *axis, float *angle);
void quaternion_integrate(Quaternion *q, const Vector3 *omega, float dt);

Matrix3 quaternion_toMatrix3(const Quaternion *q);
Quaternion quaternion_fromMatrix3(const Matrix3 *m);


#endif
