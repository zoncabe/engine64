/*
	physics_world.c — world assembly, step, body/shape management.
	Ported 1-a-1 from qu3e q3Scene.cpp.
*/
#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "physics/world/physics_world.h"
#include "physics/world/physics_island.h"
#include "physics/collision/contact.h"
#include "physics/collision/contact_solver.h"
#include "physics/collision/contact_manager.h"
#include "physics/collision/collision.h"
#include "physics/broadphase/broad_phase.h"


/* --- Shims exposed to rigid_body.c and broad_phase.c. --- */

void physicsWorld_allocShape(PhysicsWorld *s, PhysicsShape **out)
{
	*out = (PhysicsShape *)physicsPagedAllocator_allocate(&s->shape_allocator);
}

void physicsWorld_freeShape(PhysicsWorld *s, PhysicsShape *shape)
{
	physicsPagedAllocator_free(&s->shape_allocator, shape);
}

void physicsWorld_markNewShape(PhysicsWorld *s)
{
	s->new_shape = 1;
}

void broadPhase_insertShape_fromWorld(PhysicsWorld *s, PhysicsShape *shape, AABB aabb)
{
	broadPhase_insertShape(&s->contact_manager.broadphase, shape, aabb);
}

void broadPhase_removeShape_fromWorld(PhysicsWorld *s, const PhysicsShape *shape)
{
	broadPhase_removeShape(&s->contact_manager.broadphase, shape);
}

void broadPhase_updateShape(PhysicsWorld *s, int32_t index, AABB aabb)
{
	broadPhase_update(&s->contact_manager.broadphase, index, aabb);
}

void contactManager_removeContact_fromWorld(PhysicsWorld *s, ContactConstraint *c)
{
	contactManager_removeContact(&s->contact_manager, c);
}

void contactManager_removeContactsFromBody_fromWorld(PhysicsWorld *s, RigidBody *body)
{
	contactManager_removeContactsFromBody(&s->contact_manager, body);
}


/* --- lifecycle --- */

void physicsWorld_init(PhysicsWorld *s, float dt, Vector3 gravity, int32_t iterations)
{
	physicsStack_init(&s->stack);
	physicsHeap_init (&s->heap);
	contactManager_init(&s->contact_manager, &s->stack);
	physicsPagedAllocator_init(&s->shape_allocator, (int32_t)sizeof(PhysicsShape), 256);

	s->body_count       = 0;
	s->body_list        = NULL;
	s->gravity          = gravity;
	s->dt               = dt;
	s->iterations       = iterations;
	s->new_shape        = 0;
	s->allow_sleep      = 1;
	s->enable_friction  = 1;
	s->contact_listener = NULL;
}


void physicsWorld_removeAllBodies(PhysicsWorld *s)
{
	RigidBody *body = s->body_list;
	while (body) {
		RigidBody *next = body->next;
		rigidBody_removeAllShapes(body);
		physicsHeap_free(&s->heap, body);
		body = next;
	}
	s->body_list  = NULL;
	s->body_count = 0;
}


void physicsWorld_shutdown(PhysicsWorld *s)
{
	physicsWorld_removeAllBodies(s);
	physicsPagedAllocator_shutdown(&s->shape_allocator);
	contactManager_shutdown(&s->contact_manager);
	physicsHeap_shutdown(&s->heap);
	physicsStack_shutdown(&s->stack);
}


/* --- step --- */

void physics_step(PhysicsWorld *s)
{
	if (s->new_shape) {
		broadPhase_updatePairs(&s->contact_manager.broadphase);
		s->new_shape = 0;
	}

	contactManager_testCollisions(&s->contact_manager);

	for (RigidBody *body = s->body_list; body; body = body->next) {
		body->flags &= ~BODY_FLAG_ISLAND;
	}

	/* Reserve stack for island buffers. */
	physicsStack_reserve(&s->stack,
		(uint32_t)(sizeof(RigidBody *)             * s->body_count
		         + sizeof(VelocityState)           * s->body_count
		         + sizeof(ContactConstraint *)     * s->contact_manager.contact_count
		         + sizeof(ContactConstraintState)  * s->contact_manager.contact_count
		         + sizeof(RigidBody *)             * s->body_count)
	);

	PhysicsIsland island;
	island.body_capacity    = s->body_count;
	island.contact_capacity = s->contact_manager.contact_count;
	island.bodies           = (RigidBody **)            physicsStack_allocate(&s->stack, (int32_t)(sizeof(RigidBody *) * s->body_count));
	island.velocities       = (VelocityState *)         physicsStack_allocate(&s->stack, (int32_t)(sizeof(VelocityState) * s->body_count));
	island.contacts         = (ContactConstraint **)    physicsStack_allocate(&s->stack, (int32_t)(sizeof(ContactConstraint *) * island.contact_capacity));
	island.contact_states   = (ContactConstraintState *)physicsStack_allocate(&s->stack, (int32_t)(sizeof(ContactConstraintState) * island.contact_capacity));
	island.allow_sleep      = s->allow_sleep;
	island.enable_friction  = s->enable_friction;
	island.body_count       = 0;
	island.contact_count    = 0;
	island.dt               = s->dt;
	island.gravity          = s->gravity;
	island.iterations       = s->iterations;

	int32_t stack_size = s->body_count;
	RigidBody **stack = (RigidBody **)physicsStack_allocate(&s->stack, (int32_t)(sizeof(RigidBody *) * stack_size));

	for (RigidBody *seed = s->body_list; seed; seed = seed->next) {
		if (seed->flags & BODY_FLAG_ISLAND) continue;
		if (!(seed->flags & BODY_FLAG_AWAKE)) continue;
		if (seed->flags & BODY_FLAG_STATIC) continue;

		int32_t stack_count = 0;
		stack[stack_count++] = seed;
		island.body_count    = 0;
		island.contact_count = 0;

		seed->flags |= BODY_FLAG_ISLAND;

		while (stack_count > 0) {
			RigidBody *body = stack[--stack_count];
			physicsIsland_addBody(&island, body);

			rigidBody_setToAwake(body);

			if (body->flags & BODY_FLAG_STATIC) continue;

			ContactEdge *contacts = body->contact_list;
			for (ContactEdge *edge = contacts; edge; edge = edge->next) {
				ContactConstraint *contact = edge->constraint;

				if (contact->flags & CONSTRAINT_ISLAND) continue;
				if (!(contact->flags & CONSTRAINT_COLLIDING)) continue;
				if (contact->A->sensor || contact->B->sensor) continue;

				contact->flags |= CONSTRAINT_ISLAND;
				physicsIsland_addContact(&island, contact);

				RigidBody *other = edge->other;
				if (other->flags & BODY_FLAG_ISLAND) continue;

				assert(stack_count < stack_size);
				stack[stack_count++] = other;
				other->flags |= BODY_FLAG_ISLAND;
			}
		}

		assert(island.body_count != 0);

		physicsIsland_initialize(&island);
		physicsIsland_solve(&island);

		/* Reset static island flag so statics can participate in multiple islands. */
		for (int32_t i = 0; i < island.body_count; ++i) {
			RigidBody *body = island.bodies[i];
			if (body->flags & BODY_FLAG_STATIC) body->flags &= ~BODY_FLAG_ISLAND;
		}
	}

	physicsStack_free(&s->stack, stack);
	physicsStack_free(&s->stack, island.contact_states);
	physicsStack_free(&s->stack, island.contacts);
	physicsStack_free(&s->stack, island.velocities);
	physicsStack_free(&s->stack, island.bodies);

	/* Sync broadphase AABBs. */
	for (RigidBody *body = s->body_list; body; body = body->next) {
		if (body->flags & BODY_FLAG_STATIC) continue;
		rigidBody_synchronizeProxies(body);
	}

	contactManager_findNewContacts(&s->contact_manager);

	for (RigidBody *body = s->body_list; body; body = body->next) {
		body->force  = vector3_zero();
		body->torque = vector3_zero();
	}
}


/* --- body management --- */

RigidBody *physicsWorld_createBody(PhysicsWorld *s, const RigidBodyDef *def)
{
	RigidBody *body = (RigidBody *)physicsHeap_allocate(&s->heap, (int32_t)sizeof(RigidBody));
	rigidBody_init(body, def, s);

	body->prev = NULL;
	body->next = s->body_list;
	if (s->body_list) s->body_list->prev = body;
	s->body_list = body;
	++s->body_count;
	return body;
}


void physicsWorld_removeBody(PhysicsWorld *s, RigidBody *body)
{
	assert(s->body_count > 0);

	contactManager_removeContactsFromBody(&s->contact_manager, body);
	rigidBody_removeAllShapes(body);

	if (body->next) body->next->prev = body->prev;
	if (body->prev) body->prev->next = body->next;
	if (body == s->body_list) s->body_list = body->next;
	--s->body_count;

	physicsHeap_free(&s->heap, body);
}


/* --- settings --- */

void physicsWorld_setAllowSleep(PhysicsWorld *s, int allow_sleep)
{
	s->allow_sleep = allow_sleep;
	if (!allow_sleep) {
		for (RigidBody *body = s->body_list; body; body = body->next) rigidBody_setToAwake(body);
	}
}


void physicsWorld_setIterations(PhysicsWorld *s, int32_t iterations)
{
	s->iterations = (iterations > 1) ? iterations : 1;
}


void physicsWorld_setEnableFriction(PhysicsWorld *s, int enabled)
{
	s->enable_friction = enabled;
}


Vector3 physicsWorld_getGravity(const PhysicsWorld *s)       { return s->gravity; }
void    physicsWorld_setGravity(PhysicsWorld *s, Vector3 g)  { s->gravity = g; }


void physicsWorld_setContactListener(PhysicsWorld *s, ContactListener *listener)
{
	s->contact_listener = listener;
	s->contact_manager.contact_listener = listener;
}


/* --- queries --- */

typedef struct QueryAABB_ctx {
	const BroadPhase         *broadphase;
	PhysicsWorldQueryCallback cb;
	void                     *cb_user_data;
	AABB                      aabb;
} QueryAABB_ctx;

static int queryAABB_cb(void *ctx_v, int32_t id)
{
	QueryAABB_ctx *ctx = (QueryAABB_ctx *)ctx_v;
	PhysicsShape *shape = (PhysicsShape *)dynamicAABBTree_getUserData(&ctx->broadphase->tree, id);
	AABB aabb;
	Transform body_tx = rigidBody_getTransform(shape->body);
	physicsShape_computeAABB(shape, &body_tx, &aabb);
	if (aabb_overlaps(&ctx->aabb, &aabb)) {
		return ctx->cb(ctx->cb_user_data, shape);
	}
	return 1;
}

void physicsWorld_queryAABB(const PhysicsWorld *s, void *cb_user_data, PhysicsWorldQueryCallback cb, AABB aabb)
{
	QueryAABB_ctx ctx;
	ctx.broadphase   = &s->contact_manager.broadphase;
	ctx.cb           = cb;
	ctx.cb_user_data = cb_user_data;
	ctx.aabb         = aabb;
	dynamicAABBTree_queryAABB(&s->contact_manager.broadphase.tree, &ctx, queryAABB_cb, aabb);
}


typedef struct QueryPoint_ctx {
	const BroadPhase         *broadphase;
	PhysicsWorldQueryCallback cb;
	void                     *cb_user_data;
	Vector3                   point;
} QueryPoint_ctx;

static int queryPoint_cb(void *ctx_v, int32_t id)
{
	QueryPoint_ctx *ctx = (QueryPoint_ctx *)ctx_v;
	PhysicsShape *shape = (PhysicsShape *)dynamicAABBTree_getUserData(&ctx->broadphase->tree, id);
	Transform body_tx = rigidBody_getTransform(shape->body);
	if (physicsShape_testPoint(shape, &body_tx, &ctx->point)) {
		ctx->cb(ctx->cb_user_data, shape);
	}
	return 1;
}

void physicsWorld_queryPoint(const PhysicsWorld *s, void *cb_user_data, PhysicsWorldQueryCallback cb, Vector3 point)
{
	QueryPoint_ctx ctx;
	ctx.broadphase   = &s->contact_manager.broadphase;
	ctx.cb           = cb;
	ctx.cb_user_data = cb_user_data;
	ctx.point        = point;

	const float k_fattener = 0.5f;
	Vector3 v = { k_fattener, k_fattener, k_fattener };
	AABB aabb;
	aabb.min = vector3_difference(&point, &v);
	aabb.max = vector3_sum(&point, &v);
	dynamicAABBTree_queryAABB(&s->contact_manager.broadphase.tree, &ctx, queryPoint_cb, aabb);
}


typedef struct QueryRaycast_ctx {
	const BroadPhase         *broadphase;
	PhysicsWorldQueryCallback cb;
	void                     *cb_user_data;
	RaycastData              *raycast;
} QueryRaycast_ctx;

static int queryRaycast_cb(void *ctx_v, int32_t id)
{
	QueryRaycast_ctx *ctx = (QueryRaycast_ctx *)ctx_v;
	PhysicsShape *shape = (PhysicsShape *)dynamicAABBTree_getUserData(&ctx->broadphase->tree, id);
	Transform body_tx = rigidBody_getTransform(shape->body);
	if (physicsShape_raycast(shape, &body_tx, ctx->raycast)) {
		return ctx->cb(ctx->cb_user_data, shape);
	}
	return 1;
}

void physicsWorld_rayCast(const PhysicsWorld *s, void *cb_user_data, PhysicsWorldQueryCallback cb, RaycastData *raycast)
{
	QueryRaycast_ctx ctx;
	ctx.broadphase   = &s->contact_manager.broadphase;
	ctx.cb           = cb;
	ctx.cb_user_data = cb_user_data;
	ctx.raycast      = raycast;
	dynamicAABBTree_queryRay(&s->contact_manager.broadphase.tree, &ctx, queryRaycast_cb, raycast);
}
