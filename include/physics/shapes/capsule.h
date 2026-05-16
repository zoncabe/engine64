/*
	capsule.h — capsule geometry (local Z axis): cylinder + hemispheres.
	Admin fields live in PhysicsShape.
*/
#ifndef PHYSICS_SHAPE_CAPSULE_H
#define PHYSICS_SHAPE_CAPSULE_H

#include "physics/math/vector3.h"
#include "physics/math/transform.h"
#include "physics/geometry/aabb.h"
#include "physics/geometry/raycast.h"


struct MassData;


typedef struct Capsule {
	float radius;
	float half_height;   /* half-height along local Z, excluding caps */
} Capsule;


typedef struct CapsuleDef {
	Transform tx;
	float     radius;
	float     half_height;
	float     friction;
	float     restitution;
	float     density;
	int       sensor;
} CapsuleDef;


/* Endpoints of the inner segment (center ± half_height along Z), in world space. */
void  capsule_getSegment(const Capsule *c, const Transform *world, Vector3 *a, Vector3 *b);

int   capsule_testPoint(const Capsule *c, const Transform *world, const Vector3 *p);
int   capsule_raycast(const Capsule *c, const Transform *world, RaycastData *raycast);
void  capsule_computeAABB(const Capsule *c, const Transform *world, AABB *aabb);
void  capsule_computeMass(const Capsule *c, const Transform *local, float density, struct MassData *md);


void  capsuleDef_init(CapsuleDef *d);
void  capsuleDef_set(CapsuleDef *d, const Transform *tx, float radius, float half_height);
void  capsuleDef_setFriction(CapsuleDef *d, float f);
void  capsuleDef_setRestitution(CapsuleDef *d, float r);
void  capsuleDef_setDensity(CapsuleDef *d, float rho);
void  capsuleDef_setSensor(CapsuleDef *d, int s);


#endif
