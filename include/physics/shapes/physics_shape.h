/*
	physics_shape.h — polymorphic shape attached to a RigidBody.

	A RigidBody keeps one linked list of PhysicsShape. Each shape carries its
	local transform and admin fields (friction, restitution, etc.), and a
	tagged union with the concrete geometry (Box / Sphere / Capsule). The
	narrowphase dispatches on `type`.
*/
#ifndef PHYSICS_SHAPE_H
#define PHYSICS_SHAPE_H

#include <stdint.h>

#include "physics/math/matrix3.h"
#include "physics/math/transform.h"
#include "physics/math/vector3.h"
#include "physics/geometry/aabb.h"
#include "physics/geometry/raycast.h"
#include "physics/shapes/box.h"
#include "physics/shapes/sphere.h"
#include "physics/shapes/capsule.h"


struct RigidBody;
struct CollisionMesh;


typedef enum ShapeType {
	SHAPE_BOX     = 0,
	SHAPE_SPHERE  = 1,
	SHAPE_CAPSULE = 2,
	SHAPE_MESH    = 3,   /* static-only: no rigid body simulation */
} ShapeType;


typedef struct MassData {
	Matrix3 inertia;
	Vector3 center;
	float   mass;
} MassData;


typedef struct PhysicsShape {
	ShapeType            type;
	Transform            local;

	struct PhysicsShape *next;
	struct RigidBody    *body;
	float                friction;
	float                restitution;
	float                density;
	int32_t              broadphase_index;
	void                *owner;
	int                  sensor;

	union {
		Box     box;
		Sphere  sphere;
		Capsule capsule;
		struct CollisionMesh *mesh;
	};
} PhysicsShape;


/* Narrowphase / body dispatch — switches on shape->type. */
int   physicsShape_testPoint  (const PhysicsShape *shape, const Transform *body_tx, const Vector3 *p);
int   physicsShape_raycast    (const PhysicsShape *shape, const Transform *body_tx, RaycastData *raycast);
void  physicsShape_computeAABB(const PhysicsShape *shape, const Transform *body_tx, AABB *aabb);
void  physicsShape_computeMass(const PhysicsShape *shape, MassData *md);


#endif
