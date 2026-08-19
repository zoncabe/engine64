/*
	Archimedes buoyancy over the bodies inside a water sensor.

	The water is a sensor shape on a static body: the broadphase keeps its
	overlap pairs alive and the island solver already skips sensor contacts,
	so the sensor's contact list is exactly the set of bodies in the water.
	Each step, every dynamic body on that list gets the buoyant force of its
	submerged volume, plus linear and angular drag.

	The surface height is a callback so the wave animation stays the single
	source of truth: the water module owns the wave sum, this module only
	asks how high the surface sits over a given (x, y).
*/
#ifndef PHYSICS_BUOYANCY_H
#define PHYSICS_BUOYANCY_H

#include "physics/math/vector3.h"


struct PhysicsWorld;
struct RigidBody;


typedef float (*BuoyancySurfaceHeightFn)(const void *surface, float x, float y);


typedef struct BuoyancyVolume {
	struct RigidBody *body;      /* static body carrying the water sensor shape */

	float density;               /* water, kg/m3 */
	float linear_drag;           /* per-second rate on the linear velocity */
	float angular_drag;          /* per-second rate on the angular velocity */

	BuoyancySurfaceHeightFn surface_height;
	const void             *surface;
} BuoyancyVolume;


void buoyancy_apply(struct PhysicsWorld *world, const BuoyancyVolume *volume);


#endif
