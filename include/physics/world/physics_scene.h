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


typedef int (*PhysicsSceneQueryCallback)(void *user_data, PhysicsShape *shape);


typedef struct PhysicsScene {
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
} PhysicsScene;


void physicsScene_init    (PhysicsScene *s, float dt, Vector3 gravity, int32_t iterations);
void physicsScene_shutdown(PhysicsScene *s);

void physics_step(PhysicsScene *s);

RigidBody *physicsScene_createBody    (PhysicsScene *s, const RigidBodyDef *def);
void       physicsScene_removeBody    (PhysicsScene *s, RigidBody *body);
void       physicsScene_removeAllBodies(PhysicsScene *s);

void physicsScene_setAllowSleep    (PhysicsScene *s, int allow_sleep);
void physicsScene_setIterations    (PhysicsScene *s, int32_t iterations);
void physicsScene_setEnableFriction(PhysicsScene *s, int enabled);

Vector3 physicsScene_getGravity(const PhysicsScene *s);
void    physicsScene_setGravity(PhysicsScene *s, Vector3 gravity);

void physicsScene_setContactListener(PhysicsScene *s, ContactListener *listener);

void physicsScene_queryAABB (const PhysicsScene *s, void *cb_user_data, PhysicsSceneQueryCallback cb, AABB aabb);
void physicsScene_queryPoint(const PhysicsScene *s, void *cb_user_data, PhysicsSceneQueryCallback cb, Vector3 point);
void physicsScene_rayCast   (const PhysicsScene *s, void *cb_user_data, PhysicsSceneQueryCallback cb, RaycastData *raycast);


/* Shim entry points used by rigid_body.c. */
void physicsScene_allocShape (PhysicsScene *s, PhysicsShape **out);
void physicsScene_freeShape  (PhysicsScene *s, PhysicsShape  *shape);
void physicsScene_markNewShape(PhysicsScene *s);


#endif
