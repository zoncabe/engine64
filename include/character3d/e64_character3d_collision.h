#ifndef ENGINE64_CHARACTER3D_COLLISION_H
#define ENGINE64_CHARACTER3D_COLLISION_H

#include <stdbool.h>

#include "math/e64_vector3.h"
#include "math/e64_quaternion.h"
#include "math/e64_transform.h"
#include "physics3d/shapes/e64_physics_shape.h"

namespace e64 {

class Character3D;
namespace physics { class World; }


namespace character3d {

typedef struct ColliderSettings {
	float radius;
	float height;
	/* How far the capsule hangs below the model's feet: the model is drawn
	   this much above the capsule's bottom. Zero draws the feet on the
	   capsule's bottom. */
	float foot_offset;
} ColliderSettings;


/* The character's collision primitive and the owner of where it is, the way
   Unreal's capsule is the character's root: the movement component moves this
   and the mesh follows it. transform is the capsule in the scene: position at
   the capsule center, rotation the facing in the renderer's convention, always
   upright. */
typedef struct Collider {
	Capsule shape;
	Transform transform;
	float foot_offset;
} Collider;


namespace collider {

/* The capsule centered on center, facing yaw degrees. */
void init(Collider *collider, float radius, float half_height, const Vector3 *center, float yaw);

/* The facing, degrees, read from and written to the rotation matrix. The
   write rebuilds the whole matrix from the angle, so it never drifts. */
float getYaw(const Collider *collider);
void setYaw(Collider *collider, float yaw);

}


namespace collision {

/* Depenetrate against the world's static bodies — every contact classified as
   floor, wall or ceiling, one combined recovery per pass — then snap to the
   floor. Runs after the movement update, before the render sync. The character
   is not simulated by the solver: it reads those shapes and resolves on its
   own. */
void collide(Character3D *character, const e64::physics::World *world);

/* The character's standing in the world, as the entity's body: created once,
   kinematic, with the capsule; written every frame after collide, so the
   solver sees where it ended up and how fast it got there. Without this the
   character passes through every rigid body. rotation is the collider's
   rotation matrix as a quaternion, computed once per frame and shared with
   the meshes. */
void createBody(Character3D *character, e64::physics::World *world);
void syncBody(Character3D *character, const Quaternion *rotation);

}

}


}

#endif
