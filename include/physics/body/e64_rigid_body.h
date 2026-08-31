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
	Dynamic/static/kinematic body. Owns a linked list of PhysicsShape (Box /
	Sphere / Capsule).
*/
#ifndef ENGINE64_RIGID_BODY_H
#define ENGINE64_RIGID_BODY_H

#include <stdint.h>

#include "physics/math/e64_vector3.h"
#include "physics/math/e64_matrix3.h"
#include "physics/math/e64_transform.h"
#include "physics/math/e64_quaternion.h"
#include "physics/shapes/e64_physics_shape.h"
#include "physics/shapes/e64_box.h"
#include "physics/shapes/e64_sphere.h"
#include "physics/shapes/e64_capsule.h"


struct PhysicsWorld;
struct ContactEdge;


typedef enum BodyType {
	BODY_STATIC,
	BODY_DYNAMIC,
	BODY_KINEMATIC,
} BodyType;


enum {
	BODY_FLAG_AWAKE       = 0x001,
	BODY_FLAG_ACTIVE      = 0x002,
	BODY_FLAG_ALLOW_SLEEP = 0x004,
	BODY_FLAG_ISLAND      = 0x010,
	BODY_FLAG_STATIC      = 0x020,
	BODY_FLAG_DYNAMIC     = 0x040,
	BODY_FLAG_KINEMATIC   = 0x080,
	BODY_FLAG_LOCK_X      = 0x100,
	BODY_FLAG_LOCK_Y      = 0x200,
	BODY_FLAG_LOCK_Z      = 0x400,
};


typedef struct RigidBody {
	Matrix3       inv_inertia_model;
	Matrix3       inv_inertia_world;
	float         mass;
	float         inv_mass;
	Vector3       linear_velocity;
	Vector3       angular_velocity;
	Vector3       force;
	Vector3       torque;
	Transform     tx;
	Quaternion    q;
	Vector3       local_center;
	Vector3       world_center;
	float         sleep_time;
	float         gravity_scale;
	int32_t       layers;
	int32_t       flags;

	PhysicsShape *shapes;
	void         *owner;
	struct PhysicsWorld *world;
	struct RigidBody    *next;
	struct RigidBody    *prev;
	int32_t       island_index;

	float         linear_damping;
	float         angular_damping;

	struct ContactEdge  *contact_list;
} RigidBody;


typedef struct RigidBodyDef {
	Vector3     axis;
	float       angle;
	Vector3     position;
	Vector3     linear_velocity;
	Vector3     angular_velocity;
	float       gravity_scale;
	int32_t     layers;
	void       *owner;

	float       linear_damping;
	float       angular_damping;

	BodyType    body_type;

	int         allow_sleep;
	int         awake;
	int         active;
	int         lock_axis_x;
	int         lock_axis_y;
	int         lock_axis_z;
} RigidBodyDef;


void rigidBodyDef_init(RigidBodyDef *d);

void rigidBody_init(RigidBody *b, const RigidBodyDef *def, struct PhysicsWorld *world);

PhysicsShape *rigidBody_addShape  (RigidBody *b, const PhysicsShapeDef *def, Vector3 scale);
PhysicsShape *rigidBody_addBox    (RigidBody *b, const BoxDef     *def);
PhysicsShape *rigidBody_addSphere (RigidBody *b, const SphereDef  *def);
PhysicsShape *rigidBody_addCapsule(RigidBody *b, const CapsuleDef *def);
void          rigidBody_removeShape    (RigidBody *b, const PhysicsShape *shape);
void          rigidBody_removeAllShapes(RigidBody *b);

void   rigidBody_applyLinearForce            (RigidBody *b, Vector3 force);
void   rigidBody_applyForceAtWorldPoint      (RigidBody *b, Vector3 force, Vector3 point);
void   rigidBody_applyLinearImpulse          (RigidBody *b, Vector3 impulse);
void   rigidBody_applyLinearImpulseAtWorldPoint(RigidBody *b, Vector3 impulse, Vector3 point);
void   rigidBody_applyTorque                 (RigidBody *b, Vector3 torque);

void   rigidBody_setToAwake(RigidBody *b);
void   rigidBody_setToSleep(RigidBody *b);
int    rigidBody_isAwake   (const RigidBody *b);

float  rigidBody_getGravityScale(const RigidBody *b);
void   rigidBody_setGravityScale(RigidBody *b, float scale);

Vector3 rigidBody_getLocalPoint (const RigidBody *b, Vector3 p);
Vector3 rigidBody_getLocalVector(const RigidBody *b, Vector3 v);
Vector3 rigidBody_getWorldPoint (const RigidBody *b, Vector3 p);
Vector3 rigidBody_getWorldVector(const RigidBody *b, Vector3 v);

Vector3 rigidBody_getLinearVelocity (const RigidBody *b);
Vector3 rigidBody_getVelocityAtWorldPoint(const RigidBody *b, Vector3 p);
void    rigidBody_setLinearVelocity (RigidBody *b, Vector3 v);
Vector3 rigidBody_getAngularVelocity(const RigidBody *b);
void    rigidBody_setAngularVelocity(RigidBody *b, Vector3 v);

int     rigidBody_canCollide(const RigidBody *b, const RigidBody *other);

Transform  rigidBody_getTransform(const RigidBody *b);
int32_t    rigidBody_getFlags    (const RigidBody *b);
void       rigidBody_setLayers   (RigidBody *b, int32_t layers);
int32_t    rigidBody_getLayers   (const RigidBody *b);
Quaternion rigidBody_getQuaternion(const RigidBody *b);
void      *rigidBody_getOwner    (const RigidBody *b);

void    rigidBody_setLinearDamping (RigidBody *b, float damping);
float   rigidBody_getLinearDamping (const RigidBody *b);
void    rigidBody_setAngularDamping(RigidBody *b, float damping);
float   rigidBody_getAngularDamping(const RigidBody *b);

void    rigidBody_setTransformPosition         (RigidBody *b, Vector3 position);
void    rigidBody_setTransformPositionAxisAngle(RigidBody *b, Vector3 position, Vector3 axis, float angle);

float   rigidBody_getMass   (const RigidBody *b);
float   rigidBody_getInvMass(const RigidBody *b);

void    rigidBody_calculateMassData  (RigidBody *b);
void    rigidBody_synchronizeProxies (RigidBody *b);


#endif
