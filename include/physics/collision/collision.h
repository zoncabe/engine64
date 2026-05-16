/*
	collision.h — narrowphase dispatcher.

	collision() takes a manifold and two shapes, dispatches on (A->type, B->type)
	to the right pairwise function (box-box ported from qu3e; sphere/capsule
	pairs added on top). The manifold normal always points from A to B.
*/
#ifndef PHYSICS_COLLISION_H
#define PHYSICS_COLLISION_H

#include "physics/shapes/physics_shape.h"
#include "physics/collision/contact.h"


void collision(ContactManifold *m, PhysicsShape *a, PhysicsShape *b);

void boxToBox        (ContactManifold *m, PhysicsShape *a, PhysicsShape *b);
void sphereToSphere  (ContactManifold *m, PhysicsShape *a, PhysicsShape *b);
void sphereToBox     (ContactManifold *m, PhysicsShape *sphere, PhysicsShape *box);
void sphereToCapsule (ContactManifold *m, PhysicsShape *sphere, PhysicsShape *capsule);
void capsuleToCapsule(ContactManifold *m, PhysicsShape *a, PhysicsShape *b);
void capsuleToBox    (ContactManifold *m, PhysicsShape *capsule, PhysicsShape *box);


#endif
