/*
	rigid_body.c — ported from qu3e q3Body.cpp, renamed to engine conventions.
	Owns a linked list of PhysicsShape (Box / Sphere / Capsule via tagged union).
*/
#include <assert.h>
#include <stddef.h>

#include "physics/body/rigid_body.h"
#include "physics/shapes/physics_shape.h"


/* Forward declarations — resolved by physics_world.c (shims). */
struct ContactConstraint;

void  physicsWorld_allocShape (struct PhysicsWorld *s, PhysicsShape **out);
void  physicsWorld_freeShape  (struct PhysicsWorld *s, PhysicsShape  *shape);
void  physicsWorld_markNewShape(struct PhysicsWorld *s);

void  broadPhase_insertShape_fromWorld(struct PhysicsWorld *s, PhysicsShape *shape, AABB aabb);
void  broadPhase_removeShape_fromWorld(struct PhysicsWorld *s, const PhysicsShape *shape);
void  broadPhase_updateShape          (struct PhysicsWorld *s, int32_t index, AABB aabb);

void  contactManager_removeContact_fromWorld          (struct PhysicsWorld *s, struct ContactConstraint *c);
void  contactManager_removeContactsFromBody_fromWorld(struct PhysicsWorld *s, RigidBody *body);


void rigidBodyDef_init(RigidBodyDef *d)
{
	d->axis             = vector3_zero();
	d->angle            = 0.0f;
	d->position         = vector3_zero();
	d->linear_velocity  = vector3_zero();
	d->angular_velocity = vector3_zero();

	d->gravity_scale   = 1.0f;
	d->body_type       = BODY_STATIC;
	d->layers          = 0x00000001;
	d->owner           = NULL;
	d->allow_sleep     = 1;
	d->awake           = 1;
	d->active          = 1;
	d->lock_axis_x     = 0;
	d->lock_axis_y     = 0;
	d->lock_axis_z     = 0;
	d->linear_damping  = 0.0f;
	d->angular_damping = 0.1f;
}


void rigidBody_init(RigidBody *b, const RigidBodyDef *def, struct PhysicsWorld *world)
{
	b->linear_velocity  = def->linear_velocity;
	b->angular_velocity = def->angular_velocity;
	b->force            = vector3_zero();
	b->torque           = vector3_zero();

	Vector3 axis_n = vector3_normalized(&def->axis);
	b->q           = quaternion_fromAxisAngle(&axis_n, def->angle);
	b->tx.rotation = quaternion_toMatrix3(&b->q);
	b->tx.position = def->position;

	b->sleep_time      = 0.0f;
	b->gravity_scale   = def->gravity_scale;
	b->layers          = def->layers;
	b->owner           = def->owner;
	b->world           = world;
	b->flags           = 0;
	b->linear_damping  = def->linear_damping;
	b->angular_damping = def->angular_damping;

	if (def->body_type == BODY_DYNAMIC) {
		b->flags |= BODY_FLAG_DYNAMIC;
	}
	else if (def->body_type == BODY_STATIC) {
		b->flags |= BODY_FLAG_STATIC;
		b->linear_velocity  = vector3_zero();
		b->angular_velocity = vector3_zero();
		b->force            = vector3_zero();
		b->torque           = vector3_zero();
	}
	else if (def->body_type == BODY_KINEMATIC) {
		b->flags |= BODY_FLAG_KINEMATIC;
	}

	if (def->allow_sleep) b->flags |= BODY_FLAG_ALLOW_SLEEP;
	if (def->awake)       b->flags |= BODY_FLAG_AWAKE;
	if (def->active)      b->flags |= BODY_FLAG_ACTIVE;
	if (def->lock_axis_x) b->flags |= BODY_FLAG_LOCK_X;
	if (def->lock_axis_y) b->flags |= BODY_FLAG_LOCK_Y;
	if (def->lock_axis_z) b->flags |= BODY_FLAG_LOCK_Z;

	b->shapes       = NULL;
	b->contact_list = NULL;
	b->next         = NULL;
	b->prev         = NULL;
	b->island_index = 0;

	b->inv_inertia_model = matrix3_diagonal(0.0f, 0.0f, 0.0f);
	b->inv_inertia_world = matrix3_diagonal(0.0f, 0.0f, 0.0f);
	b->mass              = 0.0f;
	b->inv_mass          = 0.0f;
	b->local_center      = vector3_zero();
	b->world_center      = def->position;
}


/* Fill the admin fields of a freshly-allocated PhysicsShape and link it into
   the body's list. Type-specific geometry is copied by the caller. */
static PhysicsShape *rigidBody_attachShape(RigidBody *b, PhysicsShape *shape,
                                            const Transform *local, float friction,
                                            float restitution, float density, int sensor)
{
	shape->local = *local;
	/* Defs declared as zero-initialised globals have a zero rotation matrix;
	   treat that as identity so the mass inversion doesn't blow up. */
	float rot_sum = vector3_squaredMagnitude(&shape->local.rotation.ex)
	              + vector3_squaredMagnitude(&shape->local.rotation.ey)
	              + vector3_squaredMagnitude(&shape->local.rotation.ez);
	if (rot_sum < 1.0e-6f) {
		shape->local.rotation = matrix3_identity();
	}

	shape->friction    = friction;
	shape->restitution = restitution;
	shape->density     = density;
	shape->sensor      = sensor;
	shape->body        = b;
	shape->owner       = NULL;
	shape->broadphase_index = -1;
	shape->next        = b->shapes;
	b->shapes          = shape;

	AABB aabb;
	physicsShape_computeAABB(shape, &b->tx, &aabb);

	rigidBody_calculateMassData(b);

	broadPhase_insertShape_fromWorld(b->world, shape, aabb);
	physicsWorld_markNewShape(b->world);

	return shape;
}


/* Type-agnostic entry point: the def carries its own geometry and offset, and
   the scale is applied on the way in. */
PhysicsShape *rigidBody_addShape(RigidBody *b, const PhysicsShapeDef *def, Vector3 scale)
{
	PhysicsShape *shape = NULL;
	physicsWorld_allocShape(b->world, &shape);

	if (!physicsShape_fromDef(shape, def, scale)) return NULL;

	Transform local = shape->local;

	return rigidBody_attachShape(b, shape, &local,
	                              shape->friction, shape->restitution,
	                              shape->density, shape->sensor);
}


PhysicsShape *rigidBody_addBox(RigidBody *b, const BoxDef *def)
{
	PhysicsShape *shape = NULL;
	physicsWorld_allocShape(b->world, &shape);

	shape->type    = SHAPE_BOX;
	shape->box.e   = def->e;

	return rigidBody_attachShape(b, shape, &def->tx,
	                              def->friction, def->restitution,
	                              def->density, def->sensor);
}


PhysicsShape *rigidBody_addSphere(RigidBody *b, const SphereDef *def)
{
	PhysicsShape *shape = NULL;
	physicsWorld_allocShape(b->world, &shape);

	shape->type           = SHAPE_SPHERE;
	shape->sphere.radius  = def->radius;

	return rigidBody_attachShape(b, shape, &def->tx,
	                              def->friction, def->restitution,
	                              def->density, def->sensor);
}


PhysicsShape *rigidBody_addCapsule(RigidBody *b, const CapsuleDef *def)
{
	PhysicsShape *shape = NULL;
	physicsWorld_allocShape(b->world, &shape);

	shape->type                 = SHAPE_CAPSULE;
	shape->capsule.radius       = def->radius;
	shape->capsule.half_height  = def->half_height;

	return rigidBody_attachShape(b, shape, &def->tx,
	                              def->friction, def->restitution,
	                              def->density, def->sensor);
}


void rigidBody_removeShape(RigidBody *b, const PhysicsShape *shape)
{
	assert(shape);
	assert(shape->body == b);

	PhysicsShape *node  = b->shapes;
	int           found = 0;

	if (node == shape) {
		b->shapes = node->next;
		found = 1;
	} else {
		while (node) {
			if (node->next == shape) {
				node->next = shape->next;
				found = 1;
				break;
			}
			node = node->next;
		}
	}
	assert(found);

	/* Remove all contacts associated with this shape. */
	/* Full traversal requires ContactEdge/ContactConstraint from contact.h — see
	   rigidBody_removeAllShapes below which uses the simpler body-wide purge. */
	(void)shape;

	broadPhase_removeShape_fromWorld(b->world, shape);
	rigidBody_calculateMassData(b);
	physicsWorld_freeShape(b->world, (PhysicsShape *)shape);
}


void rigidBody_removeAllShapes(RigidBody *b)
{
	while (b->shapes) {
		PhysicsShape *next = b->shapes->next;
		broadPhase_removeShape_fromWorld(b->world, b->shapes);
		physicsShape_release(b->shapes);
		physicsWorld_freeShape(b->world, b->shapes);
		b->shapes = next;
	}
	contactManager_removeContactsFromBody_fromWorld(b->world, b);
}


/* --- Forces / impulses. --- */

void rigidBody_applyLinearForce(RigidBody *b, Vector3 force)
{
	Vector3 scaled = vector3_scaled(&force, b->mass);
	b->force = vector3_sum(&b->force, &scaled);
	rigidBody_setToAwake(b);
}


void rigidBody_applyForceAtWorldPoint(RigidBody *b, Vector3 force, Vector3 point)
{
	Vector3 scaled = vector3_scaled(&force, b->mass);
	b->force       = vector3_sum(&b->force, &scaled);
	Vector3 arm    = vector3_difference(&point, &b->world_center);
	Vector3 cross  = vector3_cross(&arm, &force);
	b->torque      = vector3_sum(&b->torque, &cross);
	rigidBody_setToAwake(b);
}


void rigidBody_applyLinearImpulse(RigidBody *b, Vector3 impulse)
{
	Vector3 delta = vector3_scaled(&impulse, b->inv_mass);
	b->linear_velocity = vector3_sum(&b->linear_velocity, &delta);
	rigidBody_setToAwake(b);
}


void rigidBody_applyLinearImpulseAtWorldPoint(RigidBody *b, Vector3 impulse, Vector3 point)
{
	Vector3 delta_lin = vector3_scaled(&impulse, b->inv_mass);
	b->linear_velocity = vector3_sum(&b->linear_velocity, &delta_lin);

	Vector3 arm   = vector3_difference(&point, &b->world_center);
	Vector3 rxI   = vector3_cross(&arm, &impulse);
	Vector3 delta = matrix3_transformVector(&b->inv_inertia_world, &rxI);
	b->angular_velocity = vector3_sum(&b->angular_velocity, &delta);
	rigidBody_setToAwake(b);
}


void rigidBody_applyTorque(RigidBody *b, Vector3 torque)
{
	b->torque = vector3_sum(&b->torque, &torque);
}


void rigidBody_setToAwake(RigidBody *b)
{
	if (!(b->flags & BODY_FLAG_AWAKE)) {
		b->flags |= BODY_FLAG_AWAKE;
		b->sleep_time = 0.0f;
	}
}


void rigidBody_setToSleep(RigidBody *b)
{
	b->flags &= ~BODY_FLAG_AWAKE;
	b->sleep_time       = 0.0f;
	b->linear_velocity  = vector3_zero();
	b->angular_velocity = vector3_zero();
	b->force            = vector3_zero();
	b->torque           = vector3_zero();
}


int   rigidBody_isAwake(const RigidBody *b)           { return (b->flags & BODY_FLAG_AWAKE) ? 1 : 0; }
float rigidBody_getMass(const RigidBody *b)           { return b->mass; }
float rigidBody_getInvMass(const RigidBody *b)        { return b->inv_mass; }
float rigidBody_getGravityScale(const RigidBody *b)   { return b->gravity_scale; }
void  rigidBody_setGravityScale(RigidBody *b, float s){ b->gravity_scale = s; }


Vector3 rigidBody_getLocalPoint(const RigidBody *b, Vector3 p)
{
	return transform_mulVectorTransposed(&b->tx, &p);
}

Vector3 rigidBody_getLocalVector(const RigidBody *b, Vector3 v)
{
	return matrix3_transformVectorTransposed(&b->tx.rotation, &v);
}

Vector3 rigidBody_getWorldPoint(const RigidBody *b, Vector3 p)
{
	return transform_mulVector(&b->tx, &p);
}

Vector3 rigidBody_getWorldVector(const RigidBody *b, Vector3 v)
{
	return matrix3_transformVector(&b->tx.rotation, &v);
}


Vector3 rigidBody_getLinearVelocity(const RigidBody *b) { return b->linear_velocity; }

Vector3 rigidBody_getVelocityAtWorldPoint(const RigidBody *b, Vector3 p)
{
	Vector3 dir     = vector3_difference(&p, &b->world_center);
	Vector3 rel_ang = vector3_cross(&b->angular_velocity, &dir);
	return vector3_sum(&b->linear_velocity, &rel_ang);
}


void rigidBody_setLinearVelocity(RigidBody *b, Vector3 v)
{
	assert(!(b->flags & BODY_FLAG_STATIC));
	if (vector3_dot(&v, &v) > 0.0f) rigidBody_setToAwake(b);
	b->linear_velocity = v;
}


Vector3 rigidBody_getAngularVelocity(const RigidBody *b) { return b->angular_velocity; }

void rigidBody_setAngularVelocity(RigidBody *b, Vector3 v)
{
	assert(!(b->flags & BODY_FLAG_STATIC));
	if (vector3_dot(&v, &v) > 0.0f) rigidBody_setToAwake(b);
	b->angular_velocity = v;
}


int rigidBody_canCollide(const RigidBody *b, const RigidBody *other)
{
	if (b == other) return 0;
	if (!(b->flags & BODY_FLAG_DYNAMIC) && !(other->flags & BODY_FLAG_DYNAMIC)) return 0;
	if (!(b->layers & other->layers)) return 0;
	return 1;
}


Transform   rigidBody_getTransform (const RigidBody *b)        { return b->tx; }
int32_t     rigidBody_getFlags     (const RigidBody *b)        { return b->flags; }
void        rigidBody_setLayers    (RigidBody *b, int32_t l)   { b->layers = l; }
int32_t     rigidBody_getLayers    (const RigidBody *b)        { return b->layers; }
Quaternion  rigidBody_getQuaternion(const RigidBody *b)        { return b->q; }
void       *rigidBody_getOwner     (const RigidBody *b)        { return b->owner; }

void  rigidBody_setLinearDamping (RigidBody *b, float d)       { b->linear_damping = d; }
float rigidBody_getLinearDamping (const RigidBody *b)          { return b->linear_damping; }
void  rigidBody_setAngularDamping(RigidBody *b, float d)       { b->angular_damping = d; }
float rigidBody_getAngularDamping(const RigidBody *b)          { return b->angular_damping; }


void rigidBody_setTransformPosition(RigidBody *b, Vector3 position)
{
	b->world_center = position;
	rigidBody_synchronizeProxies(b);
}


void rigidBody_setTransformPositionAxisAngle(RigidBody *b, Vector3 position, Vector3 axis, float angle)
{
	b->world_center  = position;
	b->q             = quaternion_fromAxisAngle(&axis, angle);
	b->tx.rotation   = quaternion_toMatrix3(&b->q);
	rigidBody_synchronizeProxies(b);
}


void rigidBody_calculateMassData(RigidBody *b)
{
	Matrix3 inertia      = matrix3_diagonal(0.0f, 0.0f, 0.0f);
	b->inv_inertia_model = matrix3_diagonal(0.0f, 0.0f, 0.0f);
	b->inv_inertia_world = matrix3_diagonal(0.0f, 0.0f, 0.0f);
	b->inv_mass          = 0.0f;
	b->mass              = 0.0f;
	float mass           = 0.0f;

	if (b->flags & BODY_FLAG_STATIC || b->flags & BODY_FLAG_KINEMATIC) {
		b->local_center = vector3_zero();
		b->world_center = b->tx.position;
		return;
	}

	Vector3 lc = vector3_zero();

	for (PhysicsShape *shape = b->shapes; shape; shape = shape->next) {
		if (shape->density == 0.0f) continue;

		MassData md;
		physicsShape_computeMass(shape, &md);
		mass                 += md.mass;
		inertia               = matrix3_sum(&inertia, &md.inertia);
		Vector3 weighted_c    = vector3_scaled(&md.center, md.mass);
		lc                    = vector3_sum(&lc, &weighted_c);
	}

	if (mass > 0.0f) {
		b->mass     = mass;
		b->inv_mass = 1.0f / mass;
		lc          = vector3_scaled(&lc, b->inv_mass);

		Matrix3 identity = matrix3_identity();
		float   dot_lc   = vector3_dot(&lc, &lc);
		Matrix3 outer    = matrix3_outerProduct(&lc, &lc);
		Matrix3 scaled_id = matrix3_scaled(&identity, dot_lc);
		Matrix3 term     = matrix3_difference(&scaled_id, &outer);
		Matrix3 scaled_term = matrix3_scaled(&term, mass);
		inertia          = matrix3_difference(&inertia, &scaled_term);
		b->inv_inertia_model = matrix3_inverse(&inertia);

		if (b->flags & BODY_FLAG_LOCK_X) {
			/* Zero the row that governs X rotation. */
			b->inv_inertia_model.ex = vector3_zero();
		}
		if (b->flags & BODY_FLAG_LOCK_Y) {
			b->inv_inertia_model.ey = vector3_zero();
		}
		if (b->flags & BODY_FLAG_LOCK_Z) {
			b->inv_inertia_model.ez = vector3_zero();
		}
	}
	else {
		b->inv_mass          = 1.0f;
		b->inv_inertia_model = matrix3_diagonal(0.0f, 0.0f, 0.0f);
		b->inv_inertia_world = matrix3_diagonal(0.0f, 0.0f, 0.0f);
	}

	b->local_center = lc;
	b->world_center = transform_mulVector(&b->tx, &lc);
}


void rigidBody_synchronizeProxies(RigidBody *b)
{
	Vector3 rlc = matrix3_transformVector(&b->tx.rotation, &b->local_center);
	b->tx.position = vector3_difference(&b->world_center, &rlc);

	AABB aabb;
	Transform tx = b->tx;

	PhysicsShape *shape = b->shapes;
	while (shape) {
		physicsShape_computeAABB(shape, &tx, &aabb);
		broadPhase_updateShape(b->world, shape->broadphase_index, aabb);
		shape = shape->next;
	}
}
