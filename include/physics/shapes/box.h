/*
	box.h — OBB geometry (half-extents on each local axis).
	Admin fields live in PhysicsShape.
*/
#ifndef PHYSICS_SHAPE_BOX_H
#define PHYSICS_SHAPE_BOX_H

#include "physics/math/vector3.h"
#include "physics/math/transform.h"
#include "physics/geometry/aabb.h"
#include "physics/geometry/raycast.h"


struct MassData;


typedef struct Box {
	Vector3 e;   /* half-extents on each OBB axis */
} Box;


typedef struct BoxDef {
	Transform tx;
	Vector3   e;
	float     friction;
	float     restitution;
	float     density;
	int       sensor;
} BoxDef;


int   box_testPoint(const Box *b, const Transform *world, const Vector3 *p);
int   box_raycast(const Box *b, const Transform *world, RaycastData *raycast);
void  box_computeAABB(const Box *b, const Transform *world, AABB *aabb);
void  box_computeMass(const Box *b, const Transform *local, float density, struct MassData *md);


void  boxDef_init(BoxDef *d);
void  boxDef_set(BoxDef *d, const Transform *tx, const Vector3 *full_extents);
void  boxDef_setFriction(BoxDef *d, float f);
void  boxDef_setRestitution(BoxDef *d, float r);
void  boxDef_setDensity(BoxDef *d, float rho);
void  boxDef_setSensor(BoxDef *d, int s);


#endif
