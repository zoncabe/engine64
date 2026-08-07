/*
	physics_shape.h — polymorphic shape attached to a RigidBody.

	A RigidBody keeps one linked list of PhysicsShape. Each shape carries its
	local transform and admin fields (friction, restitution, etc.), and a
	tagged union with the concrete geometry (Box / Sphere / Capsule). The
	narrowphase dispatches on `type`.
*/
#ifndef PHYSICS_SHAPE_H
#define PHYSICS_SHAPE_H

#include <stdbool.h>
#include <stdint.h>

#include "physics/math/matrix3.h"
#include "physics/math/transform.h"
#include "physics/math/vector3.h"
#include "physics/geometry/aabb.h"
#include "physics/geometry/raycast.h"
#include "physics/shapes/box.h"
#include "physics/shapes/sphere.h"
#include "physics/shapes/capsule.h"
#include "physics/collision/collision_mesh.h"


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


/* Authoring-side counterpart of PhysicsShape: the geometry plus its offset,
   before it is placed in a body or in the world. */
typedef struct PhysicsShapeDef {
	ShapeType type;
	union {
		BoxDef           box;
		SphereDef        sphere;
		CapsuleDef       capsule;
		CollisionMeshDef mesh;
	};
} PhysicsShapeDef;


/* Shape defs usually initialise .tx with a position only, leaving the rotation
   matrix zeroed: this reads an all-zero rotation as identity. */
Transform shapeDef_localTransform(const Transform *tx);

/* Builds a shape from its def, scaling both geometry and offset so one def
   serves any prop size. The offset lands in shape->local, for the caller to
   compose with wherever the shape ends up. Returns false for types that are
   not built from defs, so callers can skip them. */
bool physicsShape_fromDef(PhysicsShape *shape, const PhysicsShapeDef *def, Vector3 scale);

/* Frees whatever the shape owns. Only the mesh case owns anything. */
void physicsShape_release(PhysicsShape *shape);


/* Narrowphase / body dispatch — switches on shape->type. */
int   physicsShape_testPoint  (const PhysicsShape *shape, const Transform *body_tx, const Vector3 *p);
int   physicsShape_raycast    (const PhysicsShape *shape, const Transform *body_tx, RaycastData *raycast);
void  physicsShape_computeAABB(const PhysicsShape *shape, const Transform *body_tx, AABB *aabb);
void  physicsShape_computeMass(const PhysicsShape *shape, MassData *md);


#endif
