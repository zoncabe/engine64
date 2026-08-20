/*
	Capsule collision response for the kinematic body, against static geometry.

	Depenetration and floor detection ported from Godot: the recovery step of
	GodotSpace3D::test_body_motion (modules/godot_physics_3d/godot_space_3d.cpp)
	and the contact classification of CharacterBody3D::_set_collision_direction
	(scene/3d/physics/character_body_3d.cpp). No swept motion: the body moves
	the full step first and is recovered out of penetration here.
*/
#include <math.h>
#include <stdint.h>

#include "character/character.h"
#include "physics/math/math_common.h"
#include "physics/math/math_functions.h"
#include "physics/collision/collision.h"
#include "physics/collision/collision_mesh.h"


#define CHARACTER_MAX_CONTACTS           16      /* contacts kept per recovery pass */
#define CHARACTER_MAX_TRIANGLES          20      /* triangle candidates per query */
#define CHARACTER_RECOVERY_ATTEMPTS      4       /* Godot: recover_attempts */
#define CHARACTER_RECOVERY_MARGIN        0.001f  /* Godot: default safe margin */
#define CHARACTER_MIN_CONTACT_DEPTH      (CHARACTER_RECOVERY_MARGIN * 0.05f)  /* Godot: TEST_MOTION_MIN_CONTACT_DEPTH_FACTOR */
#define CHARACTER_RECOVERY_FACTOR        0.4f    /* Godot: fraction of the depth recovered per pass */
#define CHARACTER_FLOOR_SNAP_LENGTH      0.15f    /* downward probe, metres */
#define CHARACTER_FLOOR_MAX_SLOPE        50.0f   /* degrees */
#define CHARACTER_FLOOR_ANGLE_THRESHOLD  0.01f   /* radians, Godot: FLOOR_ANGLE_THRESHOLD */


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


/* Registers the kinematic body in the world. Infinite mass and no gravity: the
   solver reads it to push rigid bodies and never writes back. It must not
   sleep, or the boxes resting on it would miss the moment it starts moving. */
void characterPhysics_createBody(Character *character, PhysicsWorld *world)
{
	RigidBodyDef def;
	rigidBodyDef_init(&def);

	def.body_type   = BODY_KINEMATIC;
	def.position    = character->collider.world.position;
	def.axis        = (Vector3){ 0.0f, 0.0f, 1.0f };
	def.angle       = 0.0f;
	def.allow_sleep = 0;

	RigidBody *rigid = physicsWorld_createBody(world, &def);
	if (rigid == NULL) return;

	rigid->owner = character->entity;

	CapsuleDef shape = {
		.radius      = character->collider.shape.radius,
		.half_height = character->collider.shape.half_height,
		.friction    = 0.5f,
		.restitution = 0.0f,
		.density     = 0.0f,
	};
	transform_init(&shape.tx);

	rigidBody_addCapsule(rigid, &shape);
	character->body.rigid = rigid;
}


/* Hands the frame's outcome to the world. Velocity matters as much as the
   position: it is what tells a contact how hard the character is pushing. */
void characterPhysics_syncBody(Character *character)
{
	RigidBody *rigid = character->body.rigid;
	if (rigid == NULL) return;

	rigidBody_setTransformPositionAxisAngle(rigid, character->collider.world.position,
	                                        (Vector3){ 0.0f, 0.0f, 1.0f },
	                                        deg_to_rad(character->body.rotation.z));
	rigidBody_setLinearVelocity(rigid, character->body.velocity);
	rigidBody_setToAwake(rigid);
}


/* Contact gathering: every touching manifold of the frame, not just one. */

typedef struct CharacterContact {
	Vector3 normal;   /* unit, from the surface toward the character */
	float   depth;    /* positive penetration */
} CharacterContact;

typedef struct CharacterCollisionState {
	bool    floor;
	bool    wall;
	bool    ceiling;
	Vector3 wall_normal;   /* deepest wall contact, toward the character */
	float   wall_depth;
} CharacterCollisionState;

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

/* The manifold normal points from the capsule toward the surface and its
   penetration is dist - radius, negative when touching. The contact keeps the
   opposite convention: normal toward the character, positive depth. Degenerate
   normals are dropped: they cannot recover anything. */
static int characterPhysics_appendContact(CharacterContact *contacts, int count, const ContactManifold *m)
{
	if (count >= CHARACTER_MAX_CONTACTS) return count;

	float depth = -m->contacts[0].penetration;
	if (depth <= 0.0f) return count;

	float magnitude = vector3_magnitude(&m->normal);
	if (magnitude < 1.0e-6f) return count;

	contacts[count].normal = vector3_scaled(&m->normal, -1.0f / magnitude);
	contacts[count].depth  = depth;
	return count + 1;
}

/* All touching triangles of one mesh. The tree lives in mesh-local space:
   the capsule shifts by -origin for the query. */
static int characterPhysics_collectMeshContacts(const CharacterCollider *collider,
                                                const CollisionMesh *mesh, const Vector3 *origin,
                                                const Vector3 *velocity,
                                                CharacterContact *contacts, int count)
{
	Transform local = collider->world;
	vector3_sub(&local.position, origin);

	AABB aabb;
	capsule_computeAABB(&collider->shape, &local, &aabb);

	TriangleQuery query = { .mesh = mesh };
	collisionMesh_queryAABB(mesh, &query, characterPhysics_collectTriangle, aabb);

	for (int i = 0; i < query.count; i++) {
		Triangle triangle;
		collisionMesh_getTriangle(mesh, query.triangle[i], &triangle);

		ContactManifold m = {0};
		capsuleToTriangle(&m, &collider->shape, &local, &triangle);
		if (!m.contact_count) continue;

		/* Contact point on the triangle: the manifold stores the point on the
		   capsule surface, the triangle sits penetration further along the
		   normal. Directions survive the mesh-local shift, so the velocity
		   can be passed as is. */
		Vector3 tri_point = m.contacts[0].position;
		vector3_addScaledVector(&tri_point, &m.normal, m.contacts[0].penetration);

		m.normal = collision_fixTriangleNormal(&triangle, &tri_point, &m.normal, velocity);

		count = characterPhysics_appendContact(contacts, count, &m);
	}

	return count;
}

/* Walks the world's bodies instead of a copied list: their shapes are stored
   relative to their body, so each one is composed into world space here
   before it is tested. Obstacles are the static geometry plus every other
   kinematic body — the capsule another character keeps in the world. The
   solver cannot resolve two kinematic bodies against each other, so the
   controllers do it: each one walks out of the other. Dynamic bodies stay
   out, they are pushed by the solver instead. */
static int characterPhysics_collectContacts(const CharacterCollider *collider,
                                            const PhysicsWorld *world,
                                            const RigidBody *self,
                                            const Vector3 *velocity,
                                            CharacterContact *contacts)
{
	int count = 0;

	for (const RigidBody *body = world->body_list; body; body = body->next) {
		if (body == self) continue;
		if (!(body->flags & (BODY_FLAG_STATIC | BODY_FLAG_KINEMATIC))) continue;

		for (const PhysicsShape *shape = body->shapes; shape; shape = shape->next) {
			if (shape->sensor) continue;   /* volumes (water), not obstacles */

			Transform tx = transform_product(&body->tx, &shape->local);
			ContactManifold m = {0};

			switch (shape->type) {
				case SHAPE_MESH:
					count = characterPhysics_collectMeshContacts(collider, shape->mesh, &tx.position, velocity, contacts, count);
					continue;
				case SHAPE_BOX:
					capsuleToStaticBox(&m, &collider->shape, &collider->world, &shape->box, &tx);
					break;
				case SHAPE_SPHERE:
					capsuleToStaticSphere(&m, &collider->shape, &collider->world, &shape->sphere, &tx);
					break;
				case SHAPE_CAPSULE:
					capsuleToStaticCapsule(&m, &collider->shape, &collider->world, &shape->capsule, &tx);
					break;
			}
			if (!m.contact_count) continue;

			count = characterPhysics_appendContact(contacts, count, &m);
		}
	}

	return count;
}


/* Port of CharacterBody3D::_set_collision_direction: every contact of the
   pass is classified by its angle against the up axis — floor, ceiling or
   wall — and the frame state accumulates them. Floor detection can never be
   masked by a wall contact. */
static void characterPhysics_classifyContacts(const CharacterContact *contacts, int count, CharacterCollisionState *state)
{
	const float floor_max_angle = deg_to_rad(CHARACTER_FLOOR_MAX_SLOPE);

	bool was_wall   = state->wall;
	bool pass_floor = false;
	bool pass_wall  = false;

	int     wall_collision_count = 0;
	Vector3 combined_wall_normal = { 0.0f, 0.0f, 0.0f };
	Vector3 tmp_wall_col         = { 0.0f, 0.0f, 0.0f };

	for (int i = count - 1; i >= 0; i--) {
		const CharacterContact *c = &contacts[i];

		/* dot(normal, up) == normal.z */
		float floor_angle = acosf(clampf(c->normal.z, -1.0f, 1.0f));
		if (floor_angle <= floor_max_angle + CHARACTER_FLOOR_ANGLE_THRESHOLD) {
			pass_floor   = true;
			state->floor = true;
			continue;
		}

		float ceiling_angle = acosf(clampf(-c->normal.z, -1.0f, 1.0f));
		if (ceiling_angle <= floor_max_angle + CHARACTER_FLOOR_ANGLE_THRESHOLD) {
			state->ceiling = true;
			continue;
		}

		/* Collision is wall by default. */
		pass_wall   = true;
		state->wall = true;

		if (c->depth > state->wall_depth) {
			state->wall_depth  = c->depth;
			state->wall_normal = c->normal;
		}

		/* Collect normal for calculating average. */
		Vector3 d = vector3_difference(&c->normal, &tmp_wall_col);
		if (vector3_dot(&d, &d) > 1.0e-6f) {
			tmp_wall_col = c->normal;
			vector3_add(&combined_wall_normal, &c->normal);
			wall_collision_count++;
		}
	}

	/* Two steep walls can add up to walkable support (a wedge): their combined
	   normal points up within the floor limit even though neither does. */
	if (pass_wall && wall_collision_count > 1 && !pass_floor) {
		float magnitude = vector3_magnitude(&combined_wall_normal);
		if (magnitude > 1.0e-6f) {
			float floor_angle = acosf(clampf(combined_wall_normal.z / magnitude, -1.0f, 1.0f));
			if (floor_angle <= floor_max_angle + CHARACTER_FLOOR_ANGLE_THRESHOLD) {
				state->floor = true;
				state->wall  = was_wall;
			}
		}
	}
}

/* Port of test_body_motion STEP 1 (free body if stuck): one recovery vector
   accumulated over every contact and applied whole, up to four attempts. Each
   contact's depth is re-measured against the motion accumulated so far in the
   pass, so stacked contacts on the same plane do not over-correct. */
static void characterPhysics_recover(Character *character, const PhysicsWorld *world, CharacterCollisionState *state)
{
	int recover_attempts = CHARACTER_RECOVERY_ATTEMPTS;

	do {
		CharacterContact contacts[CHARACTER_MAX_CONTACTS];
		int count = characterPhysics_collectContacts(&character->collider, world, character->body.rigid,
		                                             &character->body.velocity, contacts);
		if (!count) break;

		characterPhysics_classifyContacts(contacts, count, state);

		Vector3 recover_motion = { 0.0f, 0.0f, 0.0f };
		for (int i = 0; i < count; i++) {
			float depth = contacts[i].depth - vector3_dot(&contacts[i].normal, &recover_motion);
			if (depth > CHARACTER_MIN_CONTACT_DEPTH + 1.0e-5f)
				vector3_addScaledVector(&recover_motion, &contacts[i].normal, (depth - CHARACTER_MIN_CONTACT_DEPTH) * CHARACTER_RECOVERY_FACTOR);
		}

		if (vector3_dot(&recover_motion, &recover_motion) == 0.0f) break;

		vector3_add(&character->body.position, &recover_motion);
		characterCollider_setVertical(&character->collider, &character->body.position);
	} while (--recover_attempts);
}


/* Velocity responses, once per frame from the classified state. */

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
}

/* Slide: remove the velocity component pushing into the wall, keep the rest.
   On the floor only the horizontal part of the wall normal is used — edge
   normals of the mesh carry a vertical component, and letting it through
   injects upward velocity that kills the floor detection. */
static void characterCollision_setWallResponse(Character *character, const CharacterCollisionState *state, bool was_on_floor)
{
	KinematicBody *body = &character->body;

	if (was_on_floor || character->movement.data.is_grounded) {
		Vector3 n = state->wall_normal;
		n.z = 0.0f;

		float magnitude = vector3_magnitude(&n);
		if (magnitude < 1.0e-6f) return;
		vector3_scale(&n, 1.0f / magnitude);

		float t = body->velocity.x * n.x + body->velocity.y * n.y;
		if (t < 0.0f) {
			body->velocity.x -= t * n.x;
			body->velocity.y -= t * n.y;
		}
	}
	else {
		float t = vector3_dot(&body->velocity, &state->wall_normal);
		if (t < 0.0f) vector3_addScaledVector(&body->velocity, &state->wall_normal, -t);
	}
}

static void characterCollision_respond(Character *character, const CharacterCollisionState *state, bool was_on_floor)
{
	if (state->floor)   characterCollision_setGroundResponse(character);
	if (state->ceiling) characterCollision_setCeilingResponse(character);
	if (state->wall)    characterCollision_setWallResponse(character, state, was_on_floor);
}


/* Floor query: the capsule's bottom sphere swept down by the snap
   length. Answers whether there is walkable floor under the character,
   how deep the probe sinks into it and with which normal. Contacts whose
   normal is steeper than the walkable limit (walls) are ignored. */

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

static void characterPhysics_probeFloor(const PhysicsWorld *world, const Vector3 *center, float radius, FloorProbe *probe)
{
	*probe = (FloorProbe){0};

	for (const RigidBody *body = world->body_list; body; body = body->next) {
	if (!(body->flags & BODY_FLAG_STATIC)) continue;

	for (const PhysicsShape *shape = body->shapes; shape; shape = shape->next) {
		if (shape->sensor) continue;   /* volumes (water), not floor */

		Transform tx = transform_product(&body->tx, &shape->local);

		switch (shape->type) {
			case SHAPE_MESH: {
				Vector3 local_center = vector3_difference(center, &tx.position);

				AABB aabb = {
					{ local_center.x - radius, local_center.y - radius, local_center.z - radius },
					{ local_center.x + radius, local_center.y + radius, local_center.z + radius },
				};

				TriangleQuery query = { .mesh = shape->mesh };
				collisionMesh_queryAABB(shape->mesh, &query, characterPhysics_collectTriangle, aabb);

				for (int t = 0; t < query.count; t++) {
					Triangle triangle;
					collisionMesh_getTriangle(shape->mesh, query.triangle[t], &triangle);
					Vector3 closest = triangle_closestToPoint(&triangle.vertices[0], &triangle.vertices[1], &triangle.vertices[2], &local_center);
					floorProbe_consider(probe, &local_center, radius, &closest);
				}
				break;
			}
			case SHAPE_BOX: {
				Vector3 local_center = transform_mulVectorTransposed(&tx, center);

				Vector3 e = shape->box.e;
				AABB box_local = { { -e.x, -e.y, -e.z }, { e.x, e.y, e.z } };
				Vector3 closest_local = aabb_closestToPoint(&box_local, &local_center);
				Vector3 closest = transform_mulVector(&tx, &closest_local);
				floorProbe_consider(probe, center, radius, &closest);
				break;
			}
			case SHAPE_SPHERE: {
				const Vector3 *pos = &tx.position;
				Vector3 d = vector3_difference(center, pos);
				Vector3 dir = vector3_normalized(&d);
				Vector3 closest = *pos;
				vector3_addScaledVector(&closest, &dir, shape->sphere.radius);
				floorProbe_consider(probe, center, radius, &closest);
				break;
			}
			case SHAPE_CAPSULE: {
				Vector3 a, b;
				capsule_getSegment(&shape->capsule, &tx, &a, &b);
				Vector3 on_seg = segment_closestToPoint(&a, &b, center);
				Vector3 d = vector3_difference(center, &on_seg);
				Vector3 dir = vector3_normalized(&d);
				Vector3 closest = on_seg;
				vector3_addScaledVector(&closest, &dir, shape->capsule.radius);
				floorProbe_consider(probe, center, radius, &closest);
				break;
			}
		}
	}
	}
}

static void characterPhysics_findFloor(const Character *character, const PhysicsWorld *world, FloorProbe *probe)
{
	float radius = character->collider.shape.radius;

	/* Bottom-sphere center, swept down by the snap length. */
	Vector3 center = character->body.position;
	center.z += radius - CHARACTER_FLOOR_SNAP_LENGTH;

	characterPhysics_probeFloor(world, &center, radius, probe);
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

/* Water probe: fraction of the capsule under the surface, from the world's
   buoyancy volumes. The character is kinematic, so it never shows up in a
   sensor's contact list — it asks the volumes directly. Read by the movement
   code on the next frame, which is where the fake buoyancy lives. */
static void characterPhysics_probeWater(Character *character, const PhysicsWorld *world)
{
	CharacterMovementData *data = &character->movement.data;

	data->in_water = false;
	data->submerged_fraction = 0.0f;

	Vector3 feet = character->body.position;

	for (int32_t i = 0; i < world->buoyancy_count; i++) {
		const BuoyancyVolume *volume = world->buoyancy[i];

		for (const PhysicsShape *shape = volume->body->shapes; shape; shape = shape->next) {
			if (!shape->sensor) continue;
			if (!physicsShape_testPoint(shape, &volume->body->tx, &feet)) continue;

			float surface = volume->surface_height(volume->surface, feet.x, feet.y);
			float height  = 2.0f * (character->collider.shape.half_height + character->collider.shape.radius);

			float fraction = (surface - feet.z) / height;
			if (fraction <= 0.0f) continue;

			data->in_water = true;
			data->submerged_fraction = (fraction > 1.0f) ? 1.0f : fraction;
			return;
		}
	}
}

void characterPhysics_collide(Character *character, const PhysicsWorld *world)
{
	CharacterMovementData *data = &character->movement.data;

	bool was_on_floor       = data->is_grounded;
	bool velocity_facing_up = character->body.velocity.z > 0.0f;
	data->is_grounded = 0;

	characterCollider_setVertical(&character->collider, &character->body.position);

	characterPhysics_probeWater(character, world);

	CharacterCollisionState state = { .wall_depth = -1.0f };
	characterPhysics_recover(character, world, &state);
	characterCollision_respond(character, &state, was_on_floor);

	FloorProbe floor;
	characterPhysics_findFloor(character, world, &floor);

	characterPhysics_snapToFloor(character, &floor, was_on_floor, velocity_facing_up);

	/* Nothing walkable under the probe and no contact resolved as floor.
	   A contact already classified as floor outranks the probe: pressed against
	   a wall the probe can miss the floor it is standing on, and falling on that
	   alone puts the character back on the ground the next frame, one frame at a
	   time, forever. */
	if (!floor.found && !data->is_grounded && characterMovement_isLocomotion(character->movement.current))
		characterMovement_setMode(&character->movement, MOVEMENT_STATE_FALLING);
}
