#ifndef RIGID_BODY_H
#define RIGID_BODY_H

#include "physics/math/vector3.h"

typedef struct RigidBody {
	Vector3 acceleration;
	Vector3 velocity;
} RigidBody;

void rigidBody_init(RigidBody *body);

#endif
