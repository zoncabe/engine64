/*
	Ported from qu3e q3Scene.h — altered source, not the original software.

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
	Top-level world: bodies, broadphase and contact manager. Owns the memory
	allocators.
*/
#ifndef PHYSICS_WORLD_H
#define PHYSICS_WORLD_H

#include <stdint.h>

#include "physics/math/vector3.h"
#include "physics/memory/physics_stack.h"
#include "physics/memory/physics_heap.h"
#include "physics/memory/physics_paged_allocator.h"
#include "physics/collision/contact_manager.h"
#include "physics/body/rigid_body.h"
#include "physics/buoyancy/buoyancy.h"
#include "physics/cloth/cloth.h"
#include "physics/shapes/physics_shape.h"
#include "physics/geometry/aabb.h"
#include "physics/geometry/raycast.h"


struct ContactConstraint;


typedef struct ContactListener {
	void *user_data;
	void (*begin_contact)(void *user_data, const struct ContactConstraint *contact);
	void (*end_contact)  (void *user_data, const struct ContactConstraint *contact);
} ContactListener;


typedef int (*PhysicsWorldQueryCallback)(void *user_data, PhysicsShape *shape);


/* Registered water volumes; one per water surface. */
#define PHYSICS_MAX_BUOYANCY_VOLUMES 2


typedef struct PhysicsWorld {
	ContactManager        contact_manager;
	PhysicsPagedAllocator shape_allocator;

	int32_t               body_count;
	RigidBody            *body_list;

	int32_t               cloth_count;
	Cloth                *cloth_list;

	const BuoyancyVolume *buoyancy[PHYSICS_MAX_BUOYANCY_VOLUMES];
	int32_t               buoyancy_count;

	PhysicsStack          stack;
	PhysicsHeap           heap;

	Vector3               gravity;
	Vector3               wind;      /* pushes cloths only; write it per frame */
	float                 dt;
	float                 accumulator;   /* cloth clock; rigid bodies step on the frame's dt */
	int32_t               iterations;

	int                   new_shape;
	int                   allow_sleep;
	int                   enable_friction;

	ContactListener      *contact_listener;
} PhysicsWorld;


void physicsWorld_init    (PhysicsWorld *s, float dt, Vector3 gravity, int32_t iterations);
void physicsWorld_shutdown(PhysicsWorld *s);

/* Advances the world by the frame's elapsed time, running as many fixed steps
   of dt as fit into it. Call this one, not physics_step, from the game loop:
   stepping once per frame ties the simulation's speed to the framerate. */
void physics_update(PhysicsWorld *s, float delta);

void physics_step(PhysicsWorld *s);

RigidBody *physicsWorld_createBody    (PhysicsWorld *s, const RigidBodyDef *def);
void       physicsWorld_removeBody    (PhysicsWorld *s, RigidBody *body);
void       physicsWorld_removeAllBodies(PhysicsWorld *s);

/* Cloths are stepped by physics_step along with the bodies. The def names the
   welded collision mesh that seeds the particles; it is loaded here, read, and
   dropped, so the caller never handles it. */
Cloth *physicsWorld_createCloth    (PhysicsWorld *s, const ClothDef *def);
void   physicsWorld_removeCloth    (PhysicsWorld *s, Cloth *cloth);
void   physicsWorld_removeAllCloths(PhysicsWorld *s);

void physicsWorld_setAllowSleep    (PhysicsWorld *s, int allow_sleep);
void physicsWorld_setIterations    (PhysicsWorld *s, int32_t iterations);
void physicsWorld_setEnableFriction(PhysicsWorld *s, int enabled);

Vector3 physicsWorld_getGravity(const PhysicsWorld *s);
void    physicsWorld_setGravity(PhysicsWorld *s, Vector3 gravity);

/* Only cloths feel it. Meant to be rewritten every frame, gusts included. */
void    physicsWorld_setWind(PhysicsWorld *s, Vector3 wind);

/* The volume is borrowed, not copied: its owner keeps it alive for the
   world's lifetime. Buoyancy runs inside physics_step on every dynamic
   body overlapping the volume's sensor shape. */
void    physicsWorld_addBuoyancy(PhysicsWorld *s, const BuoyancyVolume *volume);

void physicsWorld_setContactListener(PhysicsWorld *s, ContactListener *listener);

void physicsWorld_queryAABB (const PhysicsWorld *s, void *cb_user_data, PhysicsWorldQueryCallback cb, AABB aabb);
void physicsWorld_queryPoint(const PhysicsWorld *s, void *cb_user_data, PhysicsWorldQueryCallback cb, Vector3 point);
void physicsWorld_rayCast   (const PhysicsWorld *s, void *cb_user_data, PhysicsWorldQueryCallback cb, RaycastData *raycast);


/* Shim entry points used by rigid_body.c. */
void physicsWorld_allocShape (PhysicsWorld *s, PhysicsShape **out);
void physicsWorld_freeShape  (PhysicsWorld *s, PhysicsShape  *shape);
void physicsWorld_markNewShape(PhysicsWorld *s);


#endif
