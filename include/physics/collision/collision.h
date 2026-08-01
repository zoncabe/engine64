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
#include "physics/geometry/triangle.h"


void collision(ContactManifold *m, PhysicsShape *a, PhysicsShape *b);

void boxToBox        (ContactManifold *m, PhysicsShape *a, PhysicsShape *b);
void sphereToSphere  (ContactManifold *m, PhysicsShape *a, PhysicsShape *b);
void sphereToBox     (ContactManifold *m, PhysicsShape *sphere, PhysicsShape *box);
void sphereToCapsule (ContactManifold *m, PhysicsShape *sphere, PhysicsShape *capsule);
void capsuleToCapsule(ContactManifold *m, PhysicsShape *a, PhysicsShape *b);
void capsuleToBox    (ContactManifold *m, PhysicsShape *capsule, PhysicsShape *box);

/* Triangles come raw from a collision mesh, without a RigidBody, so this
   pair takes the capsule shape and its world transform directly. */
void capsuleToTriangle(ContactManifold *m, const Capsule *capsule, const Transform *world,
                       const Triangle *triangle);

/* Static geometry placed by a world transform, without a RigidBody. */
void capsuleToStaticBox(ContactManifold *m, const Capsule *capsule, const Transform *capsule_world,
                        const Box *box, const Transform *box_world);
void capsuleToStaticSphere(ContactManifold *m, const Capsule *capsule, const Transform *capsule_world,
                           const Sphere *sphere, const Transform *sphere_world);
void capsuleToStaticCapsule(ContactManifold *m, const Capsule *capsule, const Transform *capsule_world,
                            const Capsule *other, const Transform *other_world);


#endif
