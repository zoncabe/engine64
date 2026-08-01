#ifndef CHARACTER_PHYSICS_H
#define CHARACTER_PHYSICS_H

#include "physics/math/vector3.h"
#include "physics/math/transform.h"
#include "physics/shapes/box.h"
#include "physics/shapes/sphere.h"
#include "physics/shapes/capsule.h"
#include "physics/collision/contact.h"


typedef struct Character Character;
typedef struct CollisionMesh CollisionMesh;


typedef struct KinematicBody {
	Vector3 position;
	Vector3 velocity;
	Vector3 acceleration;
	Vector3 rotation;
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


/* View over the scene's static colliders, grouped to keep the driver
   signature flat. Pointers reference the scene arrays, nothing is copied. */
typedef struct StaticColliders {
	CollisionMesh *const *mesh;
	const Vector3        *mesh_origin;
	int                   mesh_count;

	const Box       *box;
	const Transform *box_transform;
	int              box_count;

	const Sphere    *sphere;
	const Transform *sphere_transform;
	int              sphere_count;

	const Capsule   *capsule;
	const Transform *capsule_transform;
	int              capsule_count;
} StaticColliders;

/* Collide and slide against the static colliders, then snap to the floor.
   Runs after the movement update, before the render sync. */
void characterPhysics_collide(Character *character, const StaticColliders *statics);


#endif
