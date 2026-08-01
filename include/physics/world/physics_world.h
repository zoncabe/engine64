/*
	physics_world.h — top-level world: bodies + broadphase + contact manager.
	Owns the memory allocators.
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


typedef struct PhysicsWorld {
	ContactManager        contact_manager;
	PhysicsPagedAllocator shape_allocator;

	int32_t               body_count;
	RigidBody            *body_list;

	PhysicsStack          stack;
	PhysicsHeap           heap;

	Vector3               gravity;
	float                 dt;
	int32_t               iterations;

	int                   new_shape;
	int                   allow_sleep;
	int                   enable_friction;

	ContactListener      *contact_listener;
} PhysicsWorld;


void physicsWorld_init    (PhysicsWorld *s, float dt, Vector3 gravity, int32_t iterations);
void physicsWorld_shutdown(PhysicsWorld *s);

void physics_step(PhysicsWorld *s);

RigidBody *physicsWorld_createBody    (PhysicsWorld *s, const RigidBodyDef *def);
void       physicsWorld_removeBody    (PhysicsWorld *s, RigidBody *body);
void       physicsWorld_removeAllBodies(PhysicsWorld *s);

void physicsWorld_setAllowSleep    (PhysicsWorld *s, int allow_sleep);
void physicsWorld_setIterations    (PhysicsWorld *s, int32_t iterations);
void physicsWorld_setEnableFriction(PhysicsWorld *s, int enabled);

Vector3 physicsWorld_getGravity(const PhysicsWorld *s);
void    physicsWorld_setGravity(PhysicsWorld *s, Vector3 gravity);

void physicsWorld_setContactListener(PhysicsWorld *s, ContactListener *listener);

void physicsWorld_queryAABB (const PhysicsWorld *s, void *cb_user_data, PhysicsWorldQueryCallback cb, AABB aabb);
void physicsWorld_queryPoint(const PhysicsWorld *s, void *cb_user_data, PhysicsWorldQueryCallback cb, Vector3 point);
void physicsWorld_rayCast   (const PhysicsWorld *s, void *cb_user_data, PhysicsWorldQueryCallback cb, RaycastData *raycast);


/* Shim entry points used by rigid_body.c. */
void physicsWorld_allocShape (PhysicsWorld *s, PhysicsShape **out);
void physicsWorld_freeShape  (PhysicsWorld *s, PhysicsShape  *shape);
void physicsWorld_markNewShape(PhysicsWorld *s);


#endif
