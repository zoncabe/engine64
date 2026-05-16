/*
	sphere.h — sphere geometry (radius). Admin fields live in PhysicsShape.
*/
#ifndef PHYSICS_SHAPE_SPHERE_H
#define PHYSICS_SHAPE_SPHERE_H

#include "physics/math/vector3.h"
#include "physics/math/transform.h"
#include "physics/geometry/aabb.h"
#include "physics/geometry/raycast.h"


struct MassData;


typedef struct Sphere {
	float radius;
} Sphere;


typedef struct SphereDef {
	Transform tx;
	float     radius;
	float     friction;
	float     restitution;
	float     density;
	int       sensor;
} SphereDef;


int   sphere_testPoint(const Sphere *s, const Transform *world, const Vector3 *p);
int   sphere_raycast(const Sphere *s, const Transform *world, RaycastData *raycast);
void  sphere_computeAABB(const Sphere *s, const Transform *world, AABB *aabb);
void  sphere_computeMass(const Sphere *s, const Transform *local, float density, struct MassData *md);


void  sphereDef_init(SphereDef *d);
void  sphereDef_set(SphereDef *d, const Transform *tx, float radius);
void  sphereDef_setFriction(SphereDef *d, float f);
void  sphereDef_setRestitution(SphereDef *d, float r);
void  sphereDef_setDensity(SphereDef *d, float rho);
void  sphereDef_setSensor(SphereDef *d, int s);


#endif
