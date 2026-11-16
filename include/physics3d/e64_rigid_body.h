/*
	Ported from qu3e q3Body.h — altered source, not the original software.

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
	Rigid body: mass, velocity, transform, flags, shape list.
*/
#ifndef ENGINE64_RIGID_BODY_H
#define ENGINE64_RIGID_BODY_H

#include <stdint.h>

#include "math/e64_vector3.h"
#include "math/e64_matrix3.h"
#include "math/e64_quaternion.h"
#include "math/e64_transform.h"
#include "physics3d/shapes/e64_physics_shape.h"
#include "physics3d/collision/e64_contact.h"

namespace e64 {

namespace physics { class World; }


class RigidBody {
public:

	enum Type {
		BODY_STATIC,
		BODY_DYNAMIC,
		BODY_KINEMATIC,
	};


	enum {
		BODY_FLAG_AWAKE = 0x001,
		BODY_FLAG_ACTIVE = 0x002,
		BODY_FLAG_ALLOW_SLEEP = 0x004,
		BODY_FLAG_ISLAND = 0x010,
		BODY_FLAG_STATIC = 0x020,
		BODY_FLAG_DYNAMIC = 0x040,
		BODY_FLAG_KINEMATIC = 0x080,
		BODY_FLAG_LOCK_X = 0x100,
		BODY_FLAG_LOCK_Y = 0x200,
		BODY_FLAG_LOCK_Z = 0x400,
	};


	struct Def {
		Vector3 axis;
		float angle;
		Vector3 position;
		Vector3 linear_velocity;
		Vector3 angular_velocity;
		float gravity_scale;
		int32_t layers;
		void *owner;

		float linear_damping;
		float angular_damping;

		Type body_type;

		int allow_sleep;
		int awake;
		int active;
		int lock_axis_x;
		int lock_axis_y;
		int lock_axis_z;

		/* The collision the body carries, each shape with its own offset in
		   its .tx. rigidBody::init does not read them: whoever creates the
		   body adds them, with the scale it stands at. */
		const physics::Shape::Def *shape;
		uint8_t shape_count;
	};


	Matrix3 inv_inertia_model;
	Matrix3 inv_inertia_world;
	float mass;
	float inv_mass;
	Vector3 linear_velocity;
	Vector3 angular_velocity;
	Vector3 force;
	Vector3 torque;
	Transform tx;
	Quaternion q;
	Vector3 local_center;
	Vector3 world_center;
	float sleep_time;
	float gravity_scale;
	int32_t layers;
	int32_t flags;

	physics::Shape *shapes;
	void *owner;
	physics::World *world;
	RigidBody *next;
	RigidBody *prev;
	int32_t island_index;

	float linear_damping;
	float angular_damping;

	Contact::Edge *contact_list;
};


namespace rigidBody {

namespace def {

void init(RigidBody::Def *d);

}

void init(RigidBody *b, const RigidBody::Def *def, physics::World *world);

physics::Shape *addShape (RigidBody *b, const physics::Shape::Def *def, Vector3 scale);
physics::Shape *addBox (RigidBody *b, const Box::Def *def);
physics::Shape *addSphere (RigidBody *b, const Sphere::Def *def);
physics::Shape *addCapsule(RigidBody *b, const Capsule::Def *def);
void removeShape (RigidBody *b, const physics::Shape *shape);
void removeAllShapes(RigidBody *b);

void applyLinearForce (RigidBody *b, Vector3 force);
void applyForceAtWorldPoint (RigidBody *b, Vector3 force, Vector3 point);
void applyLinearImpulse (RigidBody *b, Vector3 impulse);
void applyLinearImpulseAtWorldPoint(RigidBody *b, Vector3 impulse, Vector3 point);
void applyTorque (RigidBody *b, Vector3 torque);

void setToAwake(RigidBody *b);
void setToSleep(RigidBody *b);
int isAwake (const RigidBody *b);

float getGravityScale(const RigidBody *b);
void setGravityScale(RigidBody *b, float scale);

Vector3 getLocalPoint (const RigidBody *b, Vector3 p);
Vector3 getLocalVector(const RigidBody *b, Vector3 v);
Vector3 getWorldPoint (const RigidBody *b, Vector3 p);
Vector3 getWorldVector(const RigidBody *b, Vector3 v);

Vector3 getLinearVelocity (const RigidBody *b);
Vector3 getVelocityAtWorldPoint(const RigidBody *b, Vector3 p);
void setLinearVelocity (RigidBody *b, Vector3 v);
Vector3 getAngularVelocity(const RigidBody *b);
void setAngularVelocity(RigidBody *b, Vector3 v);

int canCollide(const RigidBody *b, const RigidBody *other);

Transform getTransform(const RigidBody *b);
int32_t getFlags (const RigidBody *b);
void setLayers (RigidBody *b, int32_t layers);
int32_t getLayers (const RigidBody *b);
Quaternion getQuaternion(const RigidBody *b);
void *getOwner (const RigidBody *b);

void setLinearDamping (RigidBody *b, float damping);
float getLinearDamping (const RigidBody *b);
void setAngularDamping(RigidBody *b, float damping);
float getAngularDamping(const RigidBody *b);

void setTransformPosition (RigidBody *b, Vector3 position);
void setTransformPositionAxisAngle(RigidBody *b, Vector3 position, Vector3 axis, float angle);
/* rotation and q are the same rotation, matrix and quaternion: the caller
   already has both, so neither is derived here. */
void setTransformPositionRotation (RigidBody *b, Vector3 position, const Matrix3 *rotation, const Quaternion *q);

float getMass (const RigidBody *b);
float getInvMass(const RigidBody *b);

void calculateMassData (RigidBody *b);
void synchronizeProxies (RigidBody *b);

}

}

#endif
