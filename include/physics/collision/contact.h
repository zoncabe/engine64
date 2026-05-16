/*
	contact.h — contact point, manifold, edge, constraint.
	Ported from qu3e q3Contact, renamed to engine conventions. Pairs work on
	PhysicsShape (box / sphere / capsule via tagged union).
*/
#ifndef PHYSICS_COLLISION_CONTACT_H
#define PHYSICS_COLLISION_CONTACT_H

#include <math.h>
#include <stdint.h>

#include "physics/math/vector3.h"
#include "physics/math/math_common.h"
#include "physics/shapes/physics_shape.h"


struct RigidBody;
struct ContactConstraint;


/* FeaturePair — 32-bit key identifying a contact point across frames. */
typedef union FeaturePair {
	struct {
		uint8_t in_r;
		uint8_t out_r;
		uint8_t in_i;
		uint8_t out_i;
	};
	int32_t key;
} FeaturePair;


/* One contact point inside a manifold. */
typedef struct ContactPoint {
	Vector3     position;
	float       penetration;
	float       normal_impulse;
	float       tangent_impulse[2];
	float       bias;
	float       normal_mass;
	float       tangent_mass[2];
	FeaturePair fp;
	uint8_t     warm_started;
} ContactPoint;


/* Contact manifold between two shapes — up to 8 contacts. */
typedef struct ContactManifold {
	PhysicsShape *A;
	PhysicsShape *B;

	Vector3       normal;               /* from A to B */
	Vector3       tangent_vectors[2];
	ContactPoint  contacts[8];
	int32_t       contact_count;

	struct ContactManifold *next;
	struct ContactManifold *prev;

	int sensor;
} ContactManifold;


void contactManifold_setPair(ContactManifold *m, PhysicsShape *a, PhysicsShape *b);


/* Node in a body's intrusive contact list. */
typedef struct ContactEdge {
	struct RigidBody         *other;
	struct ContactConstraint *constraint;
	struct ContactEdge       *next;
	struct ContactEdge       *prev;
} ContactEdge;


enum {
	CONSTRAINT_COLLIDING     = 0x00000001,
	CONSTRAINT_WAS_COLLIDING = 0x00000002,
	CONSTRAINT_ISLAND        = 0x00000004,
};


/* Persistent constraint between two bodies. */
typedef struct ContactConstraint {
	PhysicsShape     *A;
	PhysicsShape     *B;
	struct RigidBody *body_a;
	struct RigidBody *body_b;

	ContactEdge       edge_a;
	ContactEdge       edge_b;
	struct ContactConstraint *next;
	struct ContactConstraint *prev;

	float friction;
	float restitution;

	ContactManifold manifold;

	int32_t flags;
} ContactConstraint;


void contactConstraint_solveCollision(ContactConstraint *c);


/* Mixers — restitution keeps the max (bounciest wins), friction takes
   geometric mean (the slippery side dominates). */
static inline float contact_mixRestitution(const PhysicsShape *A, const PhysicsShape *B) {
	return (A->restitution > B->restitution) ? A->restitution : B->restitution;
}

static inline float contact_mixFriction(const PhysicsShape *A, const PhysicsShape *B) {
	return sqrtf(A->friction * B->friction);
}


#endif
