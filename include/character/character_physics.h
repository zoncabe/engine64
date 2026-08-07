#ifndef CHARACTER_PHYSICS_H
#define CHARACTER_PHYSICS_H

#include "physics/math/vector3.h"
#include "physics/math/transform.h"
#include "physics/shapes/physics_shape.h"
#include "physics/collision/contact.h"


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


typedef struct CharacterContact {
	Vector3 point;                   /* contact point on the surface */
	Vector3 normal;                  /* from the surface toward the character */
	float   penetration;

	Vector3 axis_closest_to_point;   /* closest point on the capsule axis to the contact point */
	Vector3 velocity_penetration;    /* penetration vector in the direction of the velocity */
	float   slope;                   /* angle of inclination of the plane of contact */
	float   angle_of_incidence;      /* angle between the velocity and the plane of contact */
	float   displacement;            /* distance from the origin to the plane of contact */
} CharacterContact;


void characterCollider_init       (CharacterCollider *collider, float radius, float half_height);
void characterCollider_setVertical(CharacterCollider *collider, const Vector3 *position);

void characterContact_clear(CharacterContact *contact);
void characterContact_set  (CharacterContact *contact, const ContactManifold *m, const CharacterCollider *collider);

void characterCollision_setResponse(Character *character, CharacterContact *contact, CharacterCollider *collider);

/* Collide and slide against the world's static bodies, then snap to the floor.
   Runs after the movement update, before the render sync. The character is not
   simulated by the solver: it reads those shapes and resolves on its own. */
void characterPhysics_collide(Character *character, const PhysicsWorld *world);

/* The character's standing in the world: created once, written every frame
   after collide, so the solver sees where it ended up and how fast it got
   there. Without this the character passes through every rigid body. */
void characterPhysics_createBody(Character *character, PhysicsWorld *world);
void characterPhysics_syncBody  (Character *character);


#endif
