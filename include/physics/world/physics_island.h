/*
	physics_island.h — island of bodies/contacts solved together each step.
	Ported from qu3e q3Island.
*/
#ifndef PHYSICS_WORLD_ISLAND_H
#define PHYSICS_WORLD_ISLAND_H

#include <stdint.h>

#include "physics/math/vector3.h"
#include "physics/physics_settings.h"


struct RigidBody;
struct ContactConstraint;
struct ContactConstraintState;


typedef struct VelocityState {
	Vector3 w;
	Vector3 v;
} VelocityState;


typedef struct PhysicsIsland {
	struct RigidBody              **bodies;
	VelocityState                  *velocities;
	int32_t                         body_capacity;
	int32_t                         body_count;

	struct ContactConstraint      **contacts;
	struct ContactConstraintState  *contact_states;
	int32_t                         contact_count;
	int32_t                         contact_capacity;

	float    dt;
	Vector3  gravity;
	int32_t  iterations;

	int      allow_sleep;
	int      enable_friction;
} PhysicsIsland;


void physicsIsland_solve      (PhysicsIsland *island);
void physicsIsland_addBody    (PhysicsIsland *island, struct RigidBody *body);
void physicsIsland_addContact (PhysicsIsland *island, struct ContactConstraint *contact);
void physicsIsland_initialize (PhysicsIsland *island);


#endif
