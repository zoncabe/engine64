/*
	Ported from qu3e q3Box.h — altered source, not the original software.

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
	OBB geometry (half-extents on each local axis). Admin fields live in
	PhysicsShape.
*/
#ifndef ENGINE64_BOX_H
#define ENGINE64_BOX_H

#include "physics/math/e64_vector3.h"
#include "physics/math/e64_transform.h"
#include "physics/geometry/e64_aabb.h"
#include "physics/geometry/e64_raycast.h"


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
