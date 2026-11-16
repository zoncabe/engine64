/*
	Ported from qu3e q3Body.cpp — altered source, not the original software.

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
	Owns a linked list of physics::Shape (Box / Sphere / Capsule via tagged
	union).
*/
#include <assert.h>
#include <stddef.h>
#include <fmath.h>

#include "physics3d/e64_rigid_body.h"
#include "physics3d/e64_physics_world.h"
#include "physics3d/shapes/e64_physics_shape.h"
#include "physics3d/collision/e64_broad_phase.h"
#include "physics3d/collision/e64_contact_manager.h"

namespace e64 {
namespace rigidBody {

namespace def {

void init(RigidBody::Def *d)
{
	d->axis = vector3::zero();
	d->angle = 0.0f;
	d->position = vector3::zero();
	d->linear_velocity = vector3::zero();
	d->angular_velocity = vector3::zero();

	d->gravity_scale = 1.0f;
	d->body_type = RigidBody::BODY_STATIC;
	d->layers = 0x00000001;
	d->owner = NULL;
	d->allow_sleep = 1;
	d->awake = 1;
	d->active = 1;
	d->lock_axis_x = 0;
	d->lock_axis_y = 0;
	d->lock_axis_z = 0;
	d->linear_damping = 0.0f;
	d->angular_damping = 0.1f;
	d->shape = NULL;
	d->shape_count = 0;
}

}


void init(RigidBody *b, const RigidBody::Def *def, physics::World *world)
{
	b->linear_velocity = def->linear_velocity;
	b->angular_velocity = def->angular_velocity;
	b->force = vector3::zero();
	b->torque = vector3::zero();

	Vector3 axis_n = vector3::normalized(&def->axis);
	b->q = quaternion::fromAxisAngle(&axis_n, def->angle);
	b->tx.rotation = quaternion::toMatrix3(&b->q);
	b->tx.position = def->position;

	b->sleep_time = 0.0f;
	b->gravity_scale = def->gravity_scale;
	b->layers = def->layers;
	b->owner = def->owner;
	b->world = world;
	b->flags = 0;
	b->linear_damping = def->linear_damping;
	b->angular_damping = def->angular_damping;

	if (def->body_type == RigidBody::BODY_DYNAMIC) {
		b->flags |= RigidBody::BODY_FLAG_DYNAMIC;
	}
	else if (def->body_type == RigidBody::BODY_STATIC) {
		b->flags |= RigidBody::BODY_FLAG_STATIC;
		b->linear_velocity = vector3::zero();
		b->angular_velocity = vector3::zero();
		b->force = vector3::zero();
		b->torque = vector3::zero();
	}
	else if (def->body_type == RigidBody::BODY_KINEMATIC) {
		b->flags |= RigidBody::BODY_FLAG_KINEMATIC;
	}

	if (def->allow_sleep) b->flags |= RigidBody::BODY_FLAG_ALLOW_SLEEP;
	if (def->awake) b->flags |= RigidBody::BODY_FLAG_AWAKE;
	if (def->active) b->flags |= RigidBody::BODY_FLAG_ACTIVE;
	if (def->lock_axis_x) b->flags |= RigidBody::BODY_FLAG_LOCK_X;
	if (def->lock_axis_y) b->flags |= RigidBody::BODY_FLAG_LOCK_Y;
	if (def->lock_axis_z) b->flags |= RigidBody::BODY_FLAG_LOCK_Z;

	b->shapes = NULL;
	b->contact_list = NULL;
	b->next = NULL;
	b->prev = NULL;
	b->island_index = 0;

	b->inv_inertia_model = matrix3::diagonal(0.0f, 0.0f, 0.0f);
	b->inv_inertia_world = matrix3::diagonal(0.0f, 0.0f, 0.0f);
	b->mass = 0.0f;
	b->inv_mass = 0.0f;
	b->local_center = vector3::zero();
	b->world_center = def->position;
}


/* Fill the admin fields of a freshly-allocated shape and link it into the
   body's list. Type-specific geometry is copied by the caller. */
static physics::Shape *attachShape(RigidBody *b, physics::Shape *shape,
                                   const Transform *local, float friction,
                                   float restitution, float density, int sensor)
{
	shape->local = *local;
	/* Defs declared as zero-initialised globals have a zero rotation matrix;
	   treat that as identity so the mass inversion doesn't blow up. */
	float rot_sum = vector3::squaredMagnitude(&shape->local.rotation.ex)
	              + vector3::squaredMagnitude(&shape->local.rotation.ey)
	              + vector3::squaredMagnitude(&shape->local.rotation.ez);
	if (rot_sum < 1.0e-6f) {
		shape->local.rotation = matrix3::identity();
	}

	shape->friction = friction;
	shape->restitution = restitution;
	shape->density = density;
	shape->sensor = sensor;
	shape->body = b;
	shape->owner = NULL;
	shape->broadphase_index = -1;
	shape->next = b->shapes;
	b->shapes = shape;
	shape->world = transform::product(&b->tx, &shape->local);

	AABB aabb;
	physics::shape::computeAABB(shape, &aabb);

	calculateMassData(b);

	collision::broadPhase::insertShape(&b->world->contact_manager.broadphase, shape, aabb);
	physics::world::markNewShape(b->world);

	return shape;
}


/* Type-agnostic entry point: the def carries its own geometry and offset, and
   the scale is applied on the way in. */
physics::Shape *addShape(RigidBody *b, const physics::Shape::Def *def, Vector3 scale)
{
	physics::Shape *shape = NULL;
	physics::world::allocShape(b->world, &shape);

	if (!physics::shape::fromDef(shape, def, scale)) return NULL;

	Transform local = shape->local;

	return attachShape(b, shape, &local,
	                   shape->friction, shape->restitution,
	                   shape->density, shape->sensor);
}


physics::Shape *addBox(RigidBody *b, const Box::Def *def)
{
	physics::Shape *shape = NULL;
	physics::world::allocShape(b->world, &shape);

	shape->type = physics::Shape::SHAPE_BOX;
	shape->box.e = def->e;

	return attachShape(b, shape, &def->tx,
	                   def->friction, def->restitution,
	                   def->density, def->sensor);
}


physics::Shape *addSphere(RigidBody *b, const Sphere::Def *def)
{
	physics::Shape *shape = NULL;
	physics::world::allocShape(b->world, &shape);

	shape->type = physics::Shape::SHAPE_SPHERE;
	shape->sphere.radius = def->radius;

	return attachShape(b, shape, &def->tx,
	                   def->friction, def->restitution,
	                   def->density, def->sensor);
}


physics::Shape *addCapsule(RigidBody *b, const Capsule::Def *def)
{
	physics::Shape *shape = NULL;
	physics::world::allocShape(b->world, &shape);

	shape->type = physics::Shape::SHAPE_CAPSULE;
	shape->capsule.radius = def->radius;
	shape->capsule.half_height = def->half_height;

	return attachShape(b, shape, &def->tx,
	                   def->friction, def->restitution,
	                   def->density, def->sensor);
}


void removeShape(RigidBody *b, const physics::Shape *shape)
{
	assert(shape);
	assert(shape->body == b);

	physics::Shape *node = b->shapes;
	int found = 0;

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

	/* Contacts of this shape are purged body-wide by removeAllShapes; a
	   per-shape purge would walk the body's Contact::Edge list here. */
	(void)shape;

	collision::broadPhase::removeShape(&b->world->contact_manager.broadphase, shape);
	calculateMassData(b);
	physics::world::freeShape(b->world, (physics::Shape *)shape);
}


void removeAllShapes(RigidBody *b)
{
	while (b->shapes) {
		physics::Shape *next = b->shapes->next;
		collision::broadPhase::removeShape(&b->world->contact_manager.broadphase, b->shapes);
		physics::shape::release(b->shapes);
		physics::world::freeShape(b->world, b->shapes);
		b->shapes = next;
	}
	contact::manager::removeContactsFromBody(&b->world->contact_manager, b);
}


void applyLinearForce(RigidBody *b, Vector3 force)
{
	Vector3 scaled = vector3::scaled(&force, b->mass);
	b->force = vector3::sum(&b->force, &scaled);
	setToAwake(b);
}


void applyForceAtWorldPoint(RigidBody *b, Vector3 force, Vector3 point)
{
	Vector3 scaled = vector3::scaled(&force, b->mass);
	b->force = vector3::sum(&b->force, &scaled);
	Vector3 arm = vector3::difference(&point, &b->world_center);
	Vector3 cross = vector3::cross(&arm, &force);
	b->torque = vector3::sum(&b->torque, &cross);
	setToAwake(b);
}


void applyLinearImpulse(RigidBody *b, Vector3 impulse)
{
	Vector3 delta = vector3::scaled(&impulse, b->inv_mass);
	b->linear_velocity = vector3::sum(&b->linear_velocity, &delta);
	setToAwake(b);
}


void applyLinearImpulseAtWorldPoint(RigidBody *b, Vector3 impulse, Vector3 point)
{
	Vector3 delta_lin = vector3::scaled(&impulse, b->inv_mass);
	b->linear_velocity = vector3::sum(&b->linear_velocity, &delta_lin);

	Vector3 arm = vector3::difference(&point, &b->world_center);
	Vector3 rxI = vector3::cross(&arm, &impulse);
	Vector3 delta = matrix3::transformVector(&b->inv_inertia_world, &rxI);
	b->angular_velocity = vector3::sum(&b->angular_velocity, &delta);
	setToAwake(b);
}


void applyTorque(RigidBody *b, Vector3 torque)
{
	b->torque = vector3::sum(&b->torque, &torque);
}


void setToAwake(RigidBody *b)
{
	if (!(b->flags & RigidBody::BODY_FLAG_AWAKE)) {
		b->flags |= RigidBody::BODY_FLAG_AWAKE;
		b->sleep_time = 0.0f;
	}
}


void setToSleep(RigidBody *b)
{
	b->flags &= ~RigidBody::BODY_FLAG_AWAKE;
	b->sleep_time = 0.0f;
	b->linear_velocity = vector3::zero();
	b->angular_velocity = vector3::zero();
	b->force = vector3::zero();
	b->torque = vector3::zero();
}


int isAwake(const RigidBody *b) { return (b->flags & RigidBody::BODY_FLAG_AWAKE) ? 1 : 0; }
float getMass(const RigidBody *b) { return b->mass; }
float getInvMass(const RigidBody *b) { return b->inv_mass; }
float getGravityScale(const RigidBody *b) { return b->gravity_scale; }
void setGravityScale(RigidBody *b, float s){ b->gravity_scale = s; }


Vector3 getLocalPoint(const RigidBody *b, Vector3 p)
{
	return transform::mulVectorTransposed(&b->tx, &p);
}

Vector3 getLocalVector(const RigidBody *b, Vector3 v)
{
	return matrix3::transformVectorTransposed(&b->tx.rotation, &v);
}

Vector3 getWorldPoint(const RigidBody *b, Vector3 p)
{
	return transform::mulVector(&b->tx, &p);
}

Vector3 getWorldVector(const RigidBody *b, Vector3 v)
{
	return matrix3::transformVector(&b->tx.rotation, &v);
}


Vector3 getLinearVelocity(const RigidBody *b) { return b->linear_velocity; }

Vector3 getVelocityAtWorldPoint(const RigidBody *b, Vector3 p)
{
	Vector3 dir = vector3::difference(&p, &b->world_center);
	Vector3 rel_ang = vector3::cross(&b->angular_velocity, &dir);
	return vector3::sum(&b->linear_velocity, &rel_ang);
}


void setLinearVelocity(RigidBody *b, Vector3 v)
{
	assert(!(b->flags & RigidBody::BODY_FLAG_STATIC));
	if (vector3::dot(&v, &v) > 0.0f) setToAwake(b);
	b->linear_velocity = v;
}


Vector3 getAngularVelocity(const RigidBody *b) { return b->angular_velocity; }

void setAngularVelocity(RigidBody *b, Vector3 v)
{
	assert(!(b->flags & RigidBody::BODY_FLAG_STATIC));
	if (vector3::dot(&v, &v) > 0.0f) setToAwake(b);
	b->angular_velocity = v;
}


int canCollide(const RigidBody *b, const RigidBody *other)
{
	if (b == other) return 0;
	if (!(b->flags & RigidBody::BODY_FLAG_DYNAMIC) && !(other->flags & RigidBody::BODY_FLAG_DYNAMIC)) return 0;
	if (!(b->layers & other->layers)) return 0;
	return 1;
}


Transform getTransform (const RigidBody *b) { return b->tx; }
int32_t getFlags (const RigidBody *b) { return b->flags; }
void setLayers (RigidBody *b, int32_t l) { b->layers = l; }
int32_t getLayers (const RigidBody *b) { return b->layers; }
Quaternion getQuaternion(const RigidBody *b) { return b->q; }
void *getOwner (const RigidBody *b) { return b->owner; }

void setLinearDamping (RigidBody *b, float d) { b->linear_damping = d; }
float getLinearDamping (const RigidBody *b) { return b->linear_damping; }
void setAngularDamping(RigidBody *b, float d) { b->angular_damping = d; }
float getAngularDamping(const RigidBody *b) { return b->angular_damping; }


void setTransformPosition(RigidBody *b, Vector3 position)
{
	b->world_center = position;
	synchronizeProxies(b);
}


void setTransformPositionAxisAngle(RigidBody *b, Vector3 position, Vector3 axis, float angle)
{
	b->world_center = position;
	b->q = quaternion::fromAxisAngle(&axis, angle);
	b->tx.rotation = quaternion::toMatrix3(&b->q);
	synchronizeProxies(b);
}


/* Both forms of the rotation handed in: nothing is converted here. */
void setTransformPositionRotation(RigidBody *b, Vector3 position, const Matrix3 *rotation, const Quaternion *q)
{
	b->world_center = position;
	b->q = *q;
	b->tx.rotation = *rotation;
	synchronizeProxies(b);
}


void calculateMassData(RigidBody *b)
{
	Matrix3 inertia = matrix3::diagonal(0.0f, 0.0f, 0.0f);
	b->inv_inertia_model = matrix3::diagonal(0.0f, 0.0f, 0.0f);
	b->inv_inertia_world = matrix3::diagonal(0.0f, 0.0f, 0.0f);
	b->inv_mass = 0.0f;
	b->mass = 0.0f;
	float mass = 0.0f;

	if (b->flags & RigidBody::BODY_FLAG_STATIC || b->flags & RigidBody::BODY_FLAG_KINEMATIC) {
		b->local_center = vector3::zero();
		b->world_center = b->tx.position;
		return;
	}

	Vector3 lc = vector3::zero();

	for (physics::Shape *shape = b->shapes; shape; shape = shape->next) {
		if (shape->density == 0.0f) continue;

		physics::shape::MassData md;
		physics::shape::computeMass(shape, &md);
		mass += md.mass;
		inertia = matrix3::sum(&inertia, &md.inertia);
		Vector3 weighted_c = vector3::scaled(&md.center, md.mass);
		lc = vector3::sum(&lc, &weighted_c);
	}

	if (mass > 0.0f) {
		b->mass = mass;
		b->inv_mass = 1.0f / mass;
		lc = vector3::scaled(&lc, b->inv_mass);

		Matrix3 identity = matrix3::identity();
		float dot_lc = vector3::dot(&lc, &lc);
		Matrix3 outer = matrix3::outerProduct(&lc, &lc);
		Matrix3 scaled_id = matrix3::scaled(&identity, dot_lc);
		Matrix3 term = matrix3::difference(&scaled_id, &outer);
		Matrix3 scaled_term = matrix3::scaled(&term, mass);
		inertia = matrix3::difference(&inertia, &scaled_term);
		b->inv_inertia_model = matrix3::inverse(&inertia);

		if (b->flags & RigidBody::BODY_FLAG_LOCK_X) {
			/* Zero the row that governs X rotation. */
			b->inv_inertia_model.ex = vector3::zero();
		}
		if (b->flags & RigidBody::BODY_FLAG_LOCK_Y) {
			b->inv_inertia_model.ey = vector3::zero();
		}
		if (b->flags & RigidBody::BODY_FLAG_LOCK_Z) {
			b->inv_inertia_model.ez = vector3::zero();
		}
	}
	else {
		b->inv_mass = 1.0f;
		b->inv_inertia_model = matrix3::diagonal(0.0f, 0.0f, 0.0f);
		b->inv_inertia_world = matrix3::diagonal(0.0f, 0.0f, 0.0f);
	}

	b->local_center = lc;
	b->world_center = transform::mulVector(&b->tx, &lc);
}


void synchronizeProxies(RigidBody *b)
{
	Vector3 rlc = matrix3::transformVector(&b->tx.rotation, &b->local_center);
	b->tx.position = vector3::difference(&b->world_center, &rlc);

	AABB aabb;

	physics::Shape *shape = b->shapes;
	while (shape) {
		shape->world = transform::product(&b->tx, &shape->local);
		physics::shape::computeAABB(shape, &aabb);
		collision::broadPhase::update(&b->world->contact_manager.broadphase, shape->broadphase_index, aabb);
		shape = shape->next;
	}
}

}
}
