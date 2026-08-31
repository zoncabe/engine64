#ifndef ENGINE64_CHARACTER_PHYSICS_H
#define ENGINE64_CHARACTER_PHYSICS_H

#include <stdbool.h>

#include "physics/math/e64_vector3.h"
#include "physics/math/e64_transform.h"
#include "physics/shapes/e64_physics_shape.h"


typedef struct Character Character;
typedef struct CollisionMesh CollisionMesh;
typedef struct PhysicsWorld PhysicsWorld;


typedef struct KinematicBody {
	Vector3 position;
	Vector3 velocity;
	Vector3 acceleration;
	Vector3 rotation;

	/* Its standing in the physics world. The solver never moves it — the fields
	   above do — but registering it is what makes the broadphase pair it with
	   rigid bodies, so the character can shove them. */
	struct RigidBody *rigid;
} KinematicBody;


typedef struct CharacterColliderSettings {
	float radius;
	float height;
} CharacterColliderSettings;


typedef struct CharacterCollider {
	Capsule   shape;
	Transform world;    /* vertical capsule, position at the capsule center */
} CharacterCollider;


void characterCollider_init       (CharacterCollider *collider, float radius, float half_height);
void characterCollider_setVertical(CharacterCollider *collider, const Vector3 *position);

/* Depenetrate against the world's static bodies — every contact classified as
   floor, wall or ceiling, one combined recovery per pass — then snap to the
   floor. Runs after the movement update, before the render sync. The character
   is not simulated by the solver: it reads those shapes and resolves on its
   own. */
void characterPhysics_collide(Character *character, const PhysicsWorld *world);

/* The character's standing in the world: created once, written every frame
   after collide, so the solver sees where it ended up and how fast it got
   there. Without this the character passes through every rigid body. */
void characterPhysics_createBody(Character *character, PhysicsWorld *world);
void characterPhysics_syncBody  (Character *character);


#endif
