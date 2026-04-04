#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "vector3.h"

typedef struct {
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
} Transform;

void transform_init(Transform *t);

#endif
