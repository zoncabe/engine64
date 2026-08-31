/*
	Ported from qu3e q3Contact.h — altered source, not the original software.

	Copyright (c) 2014 Randy Gaul http://www.randygaul.net

	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:
	  1. The origin of this software must not be misrepresented; you must not
	     claim that you wrote the original software. If you use this software
	     in a product, an acknowledgment in the product documentation would be
	     appreciated but is not required.
	  2. Altered source versions must be plainly marked as such, and must not
	     be misrepresented as being the original software.
	  3. This notice may not be removed or altered from any source distribution.
*/

/*
	Contact point, manifold, edge, constraint. Pairs work on PhysicsShape (box
	/ sphere / capsule via tagged union).
*/
#ifndef ENGINE64_CONTACT_H
#define ENGINE64_CONTACT_H

#include <math.h>
#include <stdint.h>

#include "physics/math/e64_vector3.h"
#include "physics/math/e64_math_common.h"
#include "physics/shapes/e64_physics_shape.h"


struct RigidBody;
struct ContactConstraint;


/* 32-bit key identifying a contact point across frames. */
typedef union FeaturePair {
	struct {
		uint8_t in_r;
		uint8_t out_r;
		uint8_t in_i;
		uint8_t out_i;
	};
	int32_t key;
} FeaturePair;


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


/* Up to 8 contact points between two shapes. */
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


/* Restitution keeps the max, so the bounciest side wins; friction takes the
   geometric mean, so the slippery side dominates. */
static inline float contact_mixRestitution(const PhysicsShape *A, const PhysicsShape *B) {
	return (A->restitution > B->restitution) ? A->restitution : B->restitution;
}

static inline float contact_mixFriction(const PhysicsShape *A, const PhysicsShape *B) {
	return sqrtf(A->friction * B->friction);
}


#endif
