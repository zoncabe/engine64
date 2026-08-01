/*
	character_physics.c — capsule collision response for the kinematic body.
	Collide and slide against static geometry, ported from the original
	actor_collision_response (libultra demo, commit aeb14e2).
*/
#include <math.h>
#include <stdint.h>

#include "character/character.h"
#include "physics/math/math_common.h"
#include "physics/math/math_functions.h"
#include "physics/collision/collision.h"
#include "physics/collision/collision_mesh.h"


#define CHARACTER_MAX_SLIDES         4      /* resolution iterations per frame */
#define CHARACTER_MAX_TRIANGLES      20     /* triangle candidates per query */
#define CHARACTER_FLOOR_SNAP_LENGTH  0.1f   /* downward probe, metres */
#define CHARACTER_FLOOR_MAX_SLOPE    50.0f


void characterCollider_init(CharacterCollider *collider, float radius, float half_height)
{
	collider->shape.radius      = radius;
	collider->shape.half_height = half_height;
	transform_init(&collider->world);
}

void characterCollider_setVertical(CharacterCollider *collider, const Vector3 *position)
{
	collider->world.position = (Vector3){
		position->x,
		position->y,
		position->z + collider->shape.radius + collider->shape.half_height,
	};
}


void characterContact_clear(CharacterContact *contact)
{
	contact->point                 = (Vector3){0.0f, 0.0f, 0.0f};
	contact->normal                = (Vector3){0.0f, 0.0f, 0.0f};
	contact->penetration           = 0.0f;
	contact->axis_closest_to_point = (Vector3){0.0f, 0.0f, 0.0f};
	contact->velocity_penetration  = (Vector3){0.0f, 0.0f, 0.0f};
	contact->slope                 = 1000.0f;   /* out of range value to indicate no contact */
	contact->angle_of_incidence    = 0.0f;
	contact->displacement          = 0.0f;
}

static void characterContact_setSlope(CharacterContact *contact)
{
	float magnitude = vector3_magnitude(&contact->normal);
	float cos_slope = clampf(contact->normal.z / magnitude, -1.0f, 1.0f);
	contact->slope  = rad_to_deg(acosf(cos_slope));
}

static void characterContact_setAngleOfIncidence(CharacterContact *contact, const Vector3 *velocity)
{
	float magnitude = vector3_magnitude(velocity);
	if (magnitude < 1.0e-6f) {
		contact->angle_of_incidence = 0.0f;
		return;
	}
	contact->angle_of_incidence = -rad_to_deg((PI * 0.5f) - acosf(vector3_dot(velocity, &contact->normal) / magnitude));
}

static void characterContact_setDisplacement(CharacterContact *contact)
{
	contact->displacement = vector3_dot(&contact->point, &contact->normal);
}

static void characterContact_setAxisClosestToPoint(CharacterContact *contact, const CharacterCollider *collider)
{
	Vector3 a, b;
	capsule_getSegment(&collider->shape, &collider->world, &a, &b);
	contact->axis_closest_to_point = segment_closestToPoint(&a, &b, &contact->point);
}

/* The manifold normal points from the capsule (A) toward the surface (B) and
   its penetration is dist - radius, negative when touching. This side uses
   the opposite convention: normal toward the character, positive depth. */
void characterContact_set(CharacterContact *contact, const ContactManifold *m, const CharacterCollider *collider)
{
	const ContactPoint *c = &m->contacts[0];

	contact->point = c->position;
	vector3_addScaledVector(&contact->point, &m->normal, c->penetration);
	contact->normal      = vector3_inverted(&m->normal);
	contact->penetration = -c->penetration;

	characterContact_setSlope(contact);
	characterContact_setDisplacement(contact);
	characterContact_setAxisClosestToPoint(contact, collider);
}


static void characterCollision_pushTowardsNormal(Character *character, CharacterContact *contact)
{
	vector3_addScaledVector(&character->body.position, &contact->normal, contact->penetration);
}

static void characterCollision_projectVelocity(Character *character, CharacterContact *contact)
{
	float t = vector3_dot(&character->body.velocity, &contact->normal);
	vector3_addScaledVector(&character->body.velocity, &contact->normal, -t);
}

static void characterCollision_solvePenetration(Character *character, CharacterContact *contact, CharacterCollider *collider)
{
	Vector3 velocity_normal = vector3_normalized(&character->body.velocity);

	/* Intersection of the ray (contact point + velocity normal) with the plane.
	   Standing still or moving parallel to the plane the ray never reaches it:
	   fall back to the plain push along the normal. */
	float denominator = vector3_dot(&velocity_normal, &contact->normal);
	if (fabsf(denominator) < 0.01f) {
		characterCollision_pushTowardsNormal(character, contact);
		return;
	}

	float numerator = contact->displacement + collider->shape.radius - vector3_dot(&contact->point, &contact->normal);
	float t = numerator / denominator;

	Vector3 axis_closest_at_contact = contact->point;
	vector3_addScaledVector(&axis_closest_at_contact, &velocity_normal, t);

	Vector3 displacement_vector = axis_closest_at_contact;
	vector3_sub(&displacement_vector, &contact->axis_closest_to_point);

	contact->velocity_penetration = vector3_inverted(&displacement_vector);

	vector3_add(&character->body.position, &displacement_vector);
}

static void characterCollision_collideAndSlide(Character *character, CharacterContact *contact)
{
	float t = vector3_dot(&contact->velocity_penetration, &contact->normal);
	Vector3 projection = contact->velocity_penetration;
	vector3_addScaledVector(&projection, &contact->normal, -t);

	vector3_add(&character->body.position, &projection);
}

static void characterCollision_setGroundResponse(Character *character)
{
	KinematicBody *body = &character->body;
	CharacterMovementData *data = &character->movement.data;

	/* Moving up (jump takeoff) — touching the ground must not cancel it. */
	if (body->velocity.z > 0.0f) return;

	data->is_grounded = 1;
	body->acceleration.z = 0.0f;
	body->velocity.z = 0.0f;
	if (character->movement.current == MOVEMENT_STATE_FALLING)
		characterMovement_setMode(&character->movement, character->movement.locomotion);
}

static void characterCollision_setCeilingResponse(Character *character)
{
	KinematicBody *body = &character->body;

	if (body->velocity.z > 0.0f) body->velocity.z = 0.0f;
	characterMovement_setMode(&character->movement, MOVEMENT_STATE_FALLING);
}

void characterCollision_setResponse(Character *character, CharacterContact *contact, CharacterCollider *collider)
{
	characterContact_setAngleOfIncidence(contact, &character->body.velocity);
	characterCollision_solvePenetration(character, contact, collider);

	if (contact->slope < CHARACTER_FLOOR_MAX_SLOPE) {
		characterCollision_setGroundResponse(character);
		characterCollision_collideAndSlide(character, contact);
	}
	else if (contact->slope > 95.0f && !character->movement.data.is_grounded) {
		characterCollision_collideAndSlide(character, contact);
		characterCollision_setCeilingResponse(character);
	}
	else {
		characterCollision_collideAndSlide(character, contact);
		characterCollision_projectVelocity(character, contact);
	}

	characterCollider_setVertical(collider, &character->body.position);
}


/* --- Frame driver: collide and slide, then snap to the floor. --- */

typedef struct TriangleQuery {
	const CollisionMesh *mesh;
	int32_t triangle[CHARACTER_MAX_TRIANGLES];
	int     count;
} TriangleQuery;

static int characterPhysics_collectTriangle(void *cb, int32_t id)
{
	TriangleQuery *query = cb;
	if (query->count >= CHARACTER_MAX_TRIANGLES) return 0;
	query->triangle[query->count++] = (int32_t)(intptr_t)dynamicAABBTree_getUserData(&query->mesh->tree, id);
	return 1;
}

/* Deepest contact between the capsule and one mesh. The tree lives in
   mesh-local space: the capsule shifts by -origin for the query and the
   contact point shifts back afterwards. */
static int characterPhysics_deepestContact(const CharacterCollider *collider,
                                           const CollisionMesh *mesh, const Vector3 *origin,
                                           ContactManifold *out)
{
	Transform local = collider->world;
	vector3_sub(&local.position, origin);

	AABB aabb;
	capsule_computeAABB(&collider->shape, &local, &aabb);

	TriangleQuery query = { .mesh = mesh };
	collisionMesh_queryAABB(mesh, &query, characterPhysics_collectTriangle, aabb);

	int found = 0;
	for (int i = 0; i < query.count; i++) {
		Triangle triangle;
		collisionMesh_getTriangle(mesh, query.triangle[i], &triangle);

		ContactManifold m = {0};
		capsuleToTriangle(&m, &collider->shape, &local, &triangle);
		if (!m.contact_count) continue;

		if (!found || m.contacts[0].penetration < out->contacts[0].penetration) {
			*out  = m;
			found = 1;
		}
	}

	if (found) vector3_add(&out->contacts[0].position, origin);
	return found;
}

static int characterPhysics_deepestContactAll(const CharacterCollider *collider,
                                              const StaticColliders *statics,
                                              ContactManifold *out)
{
	int found = 0;

	for (int i = 0; i < statics->mesh_count; i++) {
		ContactManifold m;
		if (!characterPhysics_deepestContact(collider, statics->mesh[i], &statics->mesh_origin[i], &m)) continue;
		if (!found || m.contacts[0].penetration < out->contacts[0].penetration) {
			*out  = m;
			found = 1;
		}
	}

	for (int i = 0; i < statics->box_count; i++) {
		ContactManifold m = {0};
		capsuleToStaticBox(&m, &collider->shape, &collider->world, &statics->box[i], &statics->box_transform[i]);
		if (!m.contact_count) continue;
		if (!found || m.contacts[0].penetration < out->contacts[0].penetration) {
			*out  = m;
			found = 1;
		}
	}

	for (int i = 0; i < statics->sphere_count; i++) {
		ContactManifold m = {0};
		capsuleToStaticSphere(&m, &collider->shape, &collider->world, &statics->sphere[i], &statics->sphere_transform[i]);
		if (!m.contact_count) continue;
		if (!found || m.contacts[0].penetration < out->contacts[0].penetration) {
			*out  = m;
			found = 1;
		}
	}

	for (int i = 0; i < statics->capsule_count; i++) {
		ContactManifold m = {0};
		capsuleToStaticCapsule(&m, &collider->shape, &collider->world, &statics->capsule[i], &statics->capsule_transform[i]);
		if (!m.contact_count) continue;
		if (!found || m.contacts[0].penetration < out->contacts[0].penetration) {
			*out  = m;
			found = 1;
		}
	}

	return found;
}

/* --- Floor query: the capsule's bottom sphere swept down by the snap
   length. Answers whether there is walkable floor under the character,
   how deep the probe sinks into it and with which normal. Contacts whose
   normal is steeper than the walkable limit (walls) are ignored. --- */

typedef struct FloorProbe {
	int     found;
	float   penetration;   /* deepest floor contact, along its normal */
	Vector3 normal;        /* from the floor toward the character */
} FloorProbe;

/* closest: nearest point of the surface to the sphere center, world space. */
static void floorProbe_consider(FloorProbe *probe, const Vector3 *center, float radius, const Vector3 *closest)
{
	Vector3 d     = vector3_difference(center, closest);
	float   dist2 = vector3_dot(&d, &d);
	if (dist2 > radius * radius) return;

	float dist = sqrtf(dist2);
	Vector3 normal = (dist > 1.0e-6f)
		? vector3_scaled(&d, 1.0f / dist)
		: vector3_create(0.0f, 0.0f, 1.0f);

	/* Walkable floor only: cos(CHARACTER_FLOOR_MAX_SLOPE). */
	if (normal.z < 0.6428f) return;

	float penetration = radius - dist;
	if (!probe->found || penetration > probe->penetration) {
		probe->found       = 1;
		probe->penetration = penetration;
		probe->normal      = normal;
	}
}

static void characterPhysics_findFloor(const Character *character, const StaticColliders *statics, FloorProbe *probe)
{
	float radius = character->collider.shape.radius;

	/* Bottom-sphere center, swept down by the snap length. */
	Vector3 center = character->body.position;
	center.z += radius - CHARACTER_FLOOR_SNAP_LENGTH;

	*probe = (FloorProbe){0};

	for (int i = 0; i < statics->mesh_count; i++) {
		const CollisionMesh *mesh = statics->mesh[i];
		Vector3 local_center = vector3_difference(&center, &statics->mesh_origin[i]);

		AABB aabb = {
			{ local_center.x - radius, local_center.y - radius, local_center.z - radius },
			{ local_center.x + radius, local_center.y + radius, local_center.z + radius },
		};

		TriangleQuery query = { .mesh = mesh };
		collisionMesh_queryAABB(mesh, &query, characterPhysics_collectTriangle, aabb);

		for (int t = 0; t < query.count; t++) {
			Triangle triangle;
			collisionMesh_getTriangle(mesh, query.triangle[t], &triangle);
			Vector3 closest = triangle_closestToPoint(&triangle.vertices[0], &triangle.vertices[1], &triangle.vertices[2], &local_center);
			floorProbe_consider(probe, &local_center, radius, &closest);
		}
	}

	for (int i = 0; i < statics->box_count; i++) {
		const Transform *world = &statics->box_transform[i];
		Vector3 local_center = transform_mulVectorTransposed(world, &center);

		Vector3 e = statics->box[i].e;
		AABB box_local = { { -e.x, -e.y, -e.z }, { e.x, e.y, e.z } };
		Vector3 closest_local = aabb_closestToPoint(&box_local, &local_center);
		Vector3 closest = transform_mulVector(world, &closest_local);
		floorProbe_consider(probe, &center, radius, &closest);
	}

	for (int i = 0; i < statics->sphere_count; i++) {
		const Vector3 *pos = &statics->sphere_transform[i].position;
		Vector3 d = vector3_difference(&center, pos);
		Vector3 dir = vector3_normalized(&d);
		Vector3 closest = *pos;
		vector3_addScaledVector(&closest, &dir, statics->sphere[i].radius);
		floorProbe_consider(probe, &center, radius, &closest);
	}

	for (int i = 0; i < statics->capsule_count; i++) {
		Vector3 a, b;
		capsule_getSegment(&statics->capsule[i], &statics->capsule_transform[i], &a, &b);
		Vector3 on_seg = segment_closestToPoint(&a, &b, &center);
		Vector3 d = vector3_difference(&center, &on_seg);
		Vector3 dir = vector3_normalized(&d);
		Vector3 closest = on_seg;
		vector3_addScaledVector(&closest, &dir, statics->capsule[i].radius);
		floorProbe_consider(probe, &center, radius, &closest);
	}
}

/* Godot's _snap_on_floor conditions: only when the character was on the
   floor, is not on it now, and is not moving up. Vertical-only correction,
   fed by the bottom-sphere floor probe. */
static void characterPhysics_snapToFloor(Character *character, const FloorProbe *floor,
                                         bool was_on_floor, bool velocity_facing_up)
{
	if (character->movement.data.is_grounded || !was_on_floor || velocity_facing_up) return;
	if (!floor->found) return;

	float drop = CHARACTER_FLOOR_SNAP_LENGTH - floor->penetration / floor->normal.z;
	if (drop > 0.0f) character->body.position.z -= drop;

	characterCollider_setVertical(&character->collider, &character->body.position);
	characterCollision_setGroundResponse(character);
}

void characterPhysics_collide(Character *character, const StaticColliders *statics)
{
	CharacterMovementData *data = &character->movement.data;

	bool was_on_floor       = data->is_grounded;
	bool velocity_facing_up = character->body.velocity.z > 0.0f;
	data->is_grounded = 0;

	characterCollider_setVertical(&character->collider, &character->body.position);

	for (int slide = 0; slide < CHARACTER_MAX_SLIDES; slide++) {
		ContactManifold m;
		if (!characterPhysics_deepestContactAll(&character->collider, statics, &m)) break;

		CharacterContact contact;
		characterContact_clear(&contact);
		characterContact_set(&contact, &m, &character->collider);
		characterCollision_setResponse(character, &contact, &character->collider);
	}

	FloorProbe floor;
	characterPhysics_findFloor(character, statics, &floor);

	characterPhysics_snapToFloor(character, &floor, was_on_floor, velocity_facing_up);

	/* Nothing walkable under the probe and no state owns the vertical motion. */
	if (!floor.found && characterMovement_isLocomotion(character->movement.current))
		characterMovement_setMode(&character->movement, MOVEMENT_STATE_FALLING);
}
