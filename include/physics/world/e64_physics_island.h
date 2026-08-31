/*
	Ported from qu3e q3Island.h — altered source, not the original software.

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
	Island of bodies and contacts solved together each step.
*/
#ifndef ENGINE64_PHYSICS_ISLAND_H
#define ENGINE64_PHYSICS_ISLAND_H

#include <stdint.h>

#include "physics/math/e64_vector3.h"
#include "physics/e64_physics_settings.h"


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
