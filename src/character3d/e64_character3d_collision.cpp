/*
	Capsule collision for the character's collider, against static geometry:
	detection, depenetration and the velocity response.

	Depenetration and floor detection ported from Godot: the recovery step of
	GodotSpace3D::test_body_motion (modules/godot_physics_3d/godot_space_3d.cpp)
	and the contact classification of Character3DBody3D::_set_collision_direction
	(scene/3d/physics/character3d_body_3d.cpp). No swept motion: the collider
	moves the full step first and is recovered out of penetration here.
*/
#include <math.h>
#include <fmath.h>
#include <stdint.h>

#include "character3d/e64_character3d.h"
#include "entity/e64_entity.h"
#include "math/e64_math.h"
#include "physics3d/e64_physics_world.h"
#include "physics3d/e64_rigid_body.h"
#include "physics3d/e64_buoyancy.h"
#include "physics3d/collision/e64_collision.h"
#include "physics3d/collision/e64_mesh_collider.h"

namespace e64 {

#define CHARACTER3D_MAX_CONTACTS 16 /* contacts kept per recovery pass */
#define CHARACTER3D_MAX_TRIANGLES 20 /* triangle candidates per query */
#define CHARACTER3D_RECOVERY_ATTEMPTS 4 /* Godot: recover_attempts */
#define CHARACTER3D_RECOVERY_MARGIN 0.001f /* Godot: default safe margin */
#define CHARACTER3D_MIN_CONTACT_DEPTH (CHARACTER3D_RECOVERY_MARGIN * 0.05f) /* Godot: TEST_MOTION_MIN_CONTACT_DEPTH_FACTOR */
#define CHARACTER3D_RECOVERY_FACTOR 0.4f /* Godot: fraction of the depth recovered per pass */
#define CHARACTER3D_FLOOR_SNAP_LENGTH 0.15f /* downward probe, metres */
#define CHARACTER3D_FALL_PROBE_LENGTH 8.0f /* how far down the landing is looked for */
/* Walkable limit of 50 degrees plus Godot's FLOOR_ANGLE_THRESHOLD of 0.01
   rad, as a cosine: floor is decided on the cosine, never the angle, because
   acosf is newlib with no fast counterpart. The argument is constant, so gcc
   folds the cosf at compile time. */
#define CHARACTER3D_FLOOR_MAX_SLOPE_COS cosf(PI / 180 * 50.0f + 0.01f)


namespace character3d {

namespace collider {

void init(Collider *collider, float radius, float half_height, const Vector3 *center, float yaw)
{
	collider->shape.radius = radius;
	collider->shape.half_height = half_height;
	collider->transform.position = *center;
	setYaw(collider, yaw);
}

/* The matrix is the one t3d_mat4_from_srt_euler builds for a rotation about
   z: ex = (cos, -sin, 0), ey = (sin, cos, 0). */
float getYaw(const Collider *collider)
{
	const Matrix3 *r = &collider->transform.rotation;
	return rad_to_deg(fm_atan2f(r->ey.x, r->ex.x));
}

void setYaw(Collider *collider, float yaw)
{
	float s, c;
	fm_sincosf(deg_to_rad(yaw), &s, &c);

	collider->transform.rotation = (Matrix3){
		.ex = { c, -s, 0.0f },
		.ey = { s, c, 0.0f },
		.ez = { 0.0f, 0.0f, 1.0f },
	};
}

}


namespace collision {

/* Registers the kinematic body in the world, as the entity's body. Infinite
   mass and no gravity: the solver reads it to push rigid bodies and never
   writes back. It must not sleep, or the boxes resting on it would miss the
   moment it starts moving. */
void createBody(Character3D *character, e64::physics::World *world)
{
	RigidBody::Def def;
	rigidBody::def::init(&def);

	def.body_type = RigidBody::BODY_KINEMATIC;
	def.position = character->collider.transform.position;
	def.axis = (Vector3){ 0.0f, 0.0f, 1.0f };
	def.angle = 0.0f;
	def.allow_sleep = 0;

	RigidBody *rigid = e64::physics::world::createBody(world, &def);
	if (rigid == NULL) return;

	rigid->owner = character->entity;

	Capsule::Def shape = {
		.radius = character->collider.shape.radius,
		.half_height = character->collider.shape.half_height,
		.friction = 0.5f,
		.restitution = 0.0f,
		.density = 0.0f,
	};
	transform::init(&shape.tx);

	rigidBody::addCapsule(rigid, &shape);
	character->entity->body = rigid;
}


/* Hands the frame's outcome to the world. Velocity matters as much as the
   position: it is what tells a contact how hard the character is pushing. */
void syncBody(Character3D *character, const Quaternion *rotation)
{
	RigidBody *rigid = character->entity->body;
	if (rigid == NULL) return;

	const Transform *transform = &character->collider.transform;

	rigidBody::setTransformPositionRotation(rigid, transform->position, &transform->rotation, rotation);
	rigidBody::setLinearVelocity(rigid, character->movement.velocity);
	rigidBody::setToAwake(rigid);
}


/* Contact gathering: every touching manifold of the frame, not just one. */

typedef struct BodyContact {
	Vector3 normal; /* unit, from the surface toward the character */
	float depth; /* positive penetration */
} BodyContact;

typedef struct CollisionState {
	bool floor;
	bool wall;
	bool ceiling;
	Vector3 wall_normal; /* deepest wall contact, toward the character */
	float wall_depth;
} CollisionState;

/* Only mesh and count are set up before a query: count gates the triangle
   array, whose entries are written before they are read. Zeroing the whole
   struct would cost a memset of the array on every call. */
typedef struct TriangleQuery {
	const MeshCollider *mesh;
	int32_t triangle[CHARACTER3D_MAX_TRIANGLES];
	int count;
} TriangleQuery;

static int collectTriangle(void *cb, int32_t id)
{
	TriangleQuery *query = (TriangleQuery *)cb;
	if (query->count >= CHARACTER3D_MAX_TRIANGLES) return 0;
	query->triangle[query->count++] = (int32_t)(intptr_t)collision::dynamicAABBTree::getUserData(&query->mesh->tree, id);
	return 1;
}

/* The manifold normal points from the capsule toward the surface and its
   penetration is dist - radius, negative when touching. The contact keeps the
   opposite convention: normal toward the character, positive depth. Every
   collision function hands over a unit normal, with a fallback for the
   degenerate case, so it is only flipped here, never renormalized. */
static int appendContact(BodyContact *contacts, int count, const Contact::Manifold *m)
{
	if (count >= CHARACTER3D_MAX_CONTACTS) return count;

	float depth = -m->contacts[0].penetration;
	if (depth <= 0.0f) return count;

	contacts[count].normal = vector3::inverted(&m->normal);
	contacts[count].depth = depth;
	return count + 1;
}

/* All touching triangles of one mesh. The tree lives in mesh-local space:
   the capsule and the velocity hint are brought into it through the inverse
   of the mesh's world transform, and each contact normal is rotated back out
   to world space before it is kept. */
static int collectMeshContacts(const Collider *collider,
                               const MeshCollider *mesh, const Transform *tx,
                               const Vector3 *velocity,
                               BodyContact *contacts, int count)
{
	Transform local = transform::productTransposed(tx, &collider->transform);
	Vector3 local_velocity = matrix3::transformVectorTransposed(&tx->rotation, velocity);

	AABB aabb;
	capsule::computeAABB(&collider->shape, &local, &aabb);

	/* libdragon builds with -ftrivial-auto-var-init=pattern, which fills every
	   local without an initializer on entry. The large ones of this hot path
	   opt out: each is written before it is read, and the fill alone was a
	   memset of several KB per frame. */
	TriangleQuery query __attribute__((uninitialized));
	query.mesh = mesh;
	query.count = 0;
	meshCollider::queryAABB(mesh, &query, collectTriangle, aabb);

	for (int i = 0; i < query.count; i++) {
		Triangle triangle __attribute__((uninitialized));
		meshCollider::getTriangle(mesh, query.triangle[i], &triangle);

		/* contact_count gates the manifold: normal and contacts[0] are written
		   by the collision function whenever it sets a contact. */
		Contact::Manifold m __attribute__((uninitialized));
		m.contact_count = 0;
		collision::capsuleToTriangle(&m, &collider->shape, &local, &triangle);
		if (!m.contact_count) continue;

		/* Contact point on the triangle: the manifold stores the point on the
		   capsule surface, the triangle sits penetration further along the
		   normal. Everything here is mesh-local, velocity included. */
		Vector3 tri_point = m.contacts[0].position;
		vector3::addScaledVector(&tri_point, &m.normal, m.contacts[0].penetration);

		m.normal = collision::fixTriangleNormal(&triangle, &tri_point, &m.normal, &local_velocity);
		m.normal = matrix3::transformVector(&tx->rotation, &m.normal);

		count = appendContact(contacts, count, &m);
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
static int collectContacts(const Collider *collider,
                           const e64::physics::World *world,
                           const RigidBody *self,
                           const Vector3 *velocity,
                           BodyContact *contacts)
{
	int count = 0;

	for (const RigidBody *body = world->body_list; body; body = body->next) {
		if (body == self) continue;
		if (!(body->flags & (RigidBody::BODY_FLAG_STATIC | RigidBody::BODY_FLAG_KINEMATIC))) continue;

		for (const e64::physics::Shape *shape = body->shapes; shape; shape = shape->next) {
			if (shape->sensor) continue; /* volumes (water), not obstacles */

			const Transform *tx = &shape->world;
			Contact::Manifold m __attribute__((uninitialized));
			m.contact_count = 0;

			switch (shape->type) {
				case e64::physics::Shape::SHAPE_MESH:
					count = collectMeshContacts(collider, shape->mesh, tx, velocity, contacts, count);
					continue;
				case e64::physics::Shape::SHAPE_BOX:
					collision::capsuleToStaticBox(&m, &collider->shape, &collider->transform, &shape->box, tx);
					break;
				case e64::physics::Shape::SHAPE_SPHERE:
					collision::capsuleToStaticSphere(&m, &collider->shape, &collider->transform, &shape->sphere, tx);
					break;
				case e64::physics::Shape::SHAPE_CAPSULE:
					collision::capsuleToStaticCapsule(&m, &collider->shape, &collider->transform, &shape->capsule, tx);
					break;
			}
			if (!m.contact_count) continue;

			count = appendContact(contacts, count, &m);
		}
	}

	return count;
}


/* Port of Character3DBody3D::_set_collision_direction: every contact of the
   pass is classified by its angle against the up axis — floor, ceiling or
   wall — and the frame state accumulates them. Floor detection can never be
   masked by a wall contact. */
static void classifyContacts(const BodyContact *contacts, int count, CollisionState *state)
{
	bool was_wall = state->wall;
	bool pass_floor = false;
	bool pass_wall = false;

	int wall_collision_count = 0;
	Vector3 combined_wall_normal = { 0.0f, 0.0f, 0.0f };
	Vector3 tmp_wall_col = { 0.0f, 0.0f, 0.0f };

	for (int i = count - 1; i >= 0; i--) {
		const BodyContact *c = &contacts[i];

		/* dot(normal, up) == normal.z; angle <= limit is cosine >= its cosine */
		if (c->normal.z >= CHARACTER3D_FLOOR_MAX_SLOPE_COS) {
			pass_floor = true;
			state->floor = true;
			continue;
		}

		if (-c->normal.z >= CHARACTER3D_FLOOR_MAX_SLOPE_COS) {
			state->ceiling = true;
			continue;
		}

		/* Collision is wall by default. */
		pass_wall = true;
		state->wall = true;

		if (c->depth > state->wall_depth) {
			state->wall_depth = c->depth;
			state->wall_normal = c->normal;
		}

		/* Collect normal for calculating average. */
		Vector3 d = vector3::difference(&c->normal, &tmp_wall_col);
		if (vector3::dot(&d, &d) > 1.0e-6f) {
			tmp_wall_col = c->normal;
			vector3::add(&combined_wall_normal, &c->normal);
			wall_collision_count++;
		}
	}

	/* Two steep walls can add up to walkable support (a wedge): their combined
	   normal points up within the floor limit even though neither does. */
	if (pass_wall && wall_collision_count > 1 && !pass_floor) {
		float magnitude = vector3::magnitude(&combined_wall_normal);
		if (magnitude > 1.0e-6f && combined_wall_normal.z >= CHARACTER3D_FLOOR_MAX_SLOPE_COS * magnitude) {
			state->floor = true;
			state->wall = was_wall;
		}
	}
}

/* Port of test_body_motion STEP 1 (free body if stuck): one recovery vector
   accumulated over every contact and applied whole, up to four attempts. Each
   contact's depth is re-measured against the motion accumulated so far in the
   pass, so stacked contacts on the same plane do not over-correct. */
static void recover(Character3D *character, const e64::physics::World *world, CollisionState *state)
{
	int recover_attempts = CHARACTER3D_RECOVERY_ATTEMPTS;

	do {
		BodyContact contacts[CHARACTER3D_MAX_CONTACTS] __attribute__((uninitialized));
		int count = collectContacts(&character->collider, world, character->entity->body,
		                            &character->movement.velocity, contacts);
		if (!count) break;

		classifyContacts(contacts, count, state);

		Vector3 recover_motion = { 0.0f, 0.0f, 0.0f };
		for (int i = 0; i < count; i++) {
			float depth = contacts[i].depth - vector3::dot(&contacts[i].normal, &recover_motion);
			if (depth > CHARACTER3D_MIN_CONTACT_DEPTH + 1.0e-5f)
				vector3::addScaledVector(&recover_motion, &contacts[i].normal, (depth - CHARACTER3D_MIN_CONTACT_DEPTH) * CHARACTER3D_RECOVERY_FACTOR);
		}

		if (vector3::dot(&recover_motion, &recover_motion) == 0.0f) break;

		vector3::add(&character->collider.transform.position, &recover_motion);
	} while (--recover_attempts);
}


/* Velocity responses, once per frame from the classified state. */

static void setGroundResponse(Character3D *character)
{
	Movement *movement = &character->movement;
	MovementData *data = &movement->data;

	/* Moving up (jump takeoff) — touching the ground must not cancel it. */
	if (movement->velocity.z > 0.0f) return;

	data->is_grounded = 1;
	movement->acceleration.z = 0.0f;
	movement->velocity.z = 0.0f;

	/* The floor is back: the next ledge gets its own coyote window. */
	data->coyote_timer = 0.0f;
	if (movement->mode == MOVEMENT_STATE_FALL)
		movement->setMode(MOVEMENT_STATE_WALK);
}

static void setCeilingResponse(Character3D *character)
{
	Vector3 *velocity = &character->movement.velocity;
	if (velocity->z > 0.0f) velocity->z = 0.0f;
}

/* Slide: remove the velocity component pushing into the wall, keep the rest.
   On the floor only the horizontal part of the wall normal is used — edge
   normals of the mesh carry a vertical component, and letting it through
   injects upward velocity that kills the floor detection. */
static void setWallResponse(Character3D *character, const CollisionState *state, bool was_on_floor)
{
	Vector3 *velocity = &character->movement.velocity;

	if (was_on_floor || character->movement.data.is_grounded) {
		Vector3 n = state->wall_normal;
		n.z = 0.0f;

		float magnitude = vector3::magnitude(&n);
		if (magnitude < 1.0e-6f) return;
		vector3::scale(&n, 1.0f / magnitude);

		float t = velocity->x * n.x + velocity->y * n.y;
		if (t < 0.0f) {
			velocity->x -= t * n.x;
			velocity->y -= t * n.y;
		}
	}
	else {
		float t = vector3::dot(velocity, &state->wall_normal);
		if (t < 0.0f) vector3::addScaledVector(velocity, &state->wall_normal, -t);
	}
}

static void respond(Character3D *character, const CollisionState *state, bool was_on_floor)
{
	if (state->floor) setGroundResponse(character);
	if (state->ceiling) setCeilingResponse(character);
	if (state->wall) setWallResponse(character, state, was_on_floor);
}


/* Floor query: the capsule's bottom sphere swept down by the snap
   length. Answers whether there is walkable floor under the character,
   how deep the probe sinks into it and with which normal. Contacts whose
   normal is steeper than the walkable limit (walls) are ignored. */

typedef struct FloorProbe {
	int found;
	float penetration; /* deepest floor contact, along its normal */
	Vector3 normal; /* from the floor toward the character */
} FloorProbe;

/* closest: nearest point of the surface to the sphere center, world space. */
static void floorProbe_consider(FloorProbe *probe, const Vector3 *center, float radius, const Vector3 *closest)
{
	Vector3 d = vector3::difference(center, closest);
	float dist2 = vector3::dot(&d, &d);
	if (dist2 > radius * radius) return;

	float dist = sqrtf(dist2);
	Vector3 normal = (dist > 1.0e-6f)
		? vector3::scaled(&d, 1.0f / dist)
		: vector3::create(0.0f, 0.0f, 1.0f);

	/* Walkable floor only. */
	if (normal.z < CHARACTER3D_FLOOR_MAX_SLOPE_COS) return;

	float penetration = radius - dist;
	if (!probe->found || penetration > probe->penetration) {
		probe->found = 1;
		probe->penetration = penetration;
		probe->normal = normal;
	}
}

static void probeFloor(const e64::physics::World *world, const Vector3 *center, float radius, FloorProbe *probe)
{
	/* found gates every other field: they are written together on a hit. */
	probe->found = 0;

	for (const RigidBody *body = world->body_list; body; body = body->next) {
	if (!(body->flags & RigidBody::BODY_FLAG_STATIC)) continue;

	for (const e64::physics::Shape *shape = body->shapes; shape; shape = shape->next) {
		if (shape->sensor) continue; /* volumes (water), not floor */

		const Transform *tx = &shape->world;

		switch (shape->type) {
			case e64::physics::Shape::SHAPE_MESH: {
				/* The tree is mesh-local: the sphere goes in through the
				   inverse transform, the closest point comes back out. */
				Vector3 local_center = transform::mulVectorTransposed(tx, center);

				AABB aabb = {
					{ local_center.x - radius, local_center.y - radius, local_center.z - radius },
					{ local_center.x + radius, local_center.y + radius, local_center.z + radius },
				};

				TriangleQuery query __attribute__((uninitialized));
				query.mesh = shape->mesh;
				query.count = 0;
				meshCollider::queryAABB(shape->mesh, &query, collectTriangle, aabb);

				for (int t = 0; t < query.count; t++) {
					Triangle triangle __attribute__((uninitialized));
					meshCollider::getTriangle(shape->mesh, query.triangle[t], &triangle);
					Vector3 closest_local = triangle::closestToPoint(&triangle, &local_center);
					Vector3 closest = transform::mulVector(tx, &closest_local);
					floorProbe_consider(probe, center, radius, &closest);
				}
				break;
			}
			case e64::physics::Shape::SHAPE_BOX: {
				Vector3 local_center = transform::mulVectorTransposed(tx, center);

				Vector3 e = shape->box.e;
				AABB box_local = { { -e.x, -e.y, -e.z }, { e.x, e.y, e.z } };
				Vector3 closest_local = aabb::closestToPoint(&box_local, &local_center);
				Vector3 closest = transform::mulVector(tx, &closest_local);
				floorProbe_consider(probe, center, radius, &closest);
				break;
			}
			case e64::physics::Shape::SHAPE_SPHERE: {
				const Vector3 *pos = &tx->position;
				Vector3 d = vector3::difference(center, pos);
				Vector3 dir = vector3::normalized(&d);
				Vector3 closest = *pos;
				vector3::addScaledVector(&closest, &dir, shape->sphere.radius);
				floorProbe_consider(probe, center, radius, &closest);
				break;
			}
			case e64::physics::Shape::SHAPE_CAPSULE: {
				Vector3 a, b;
				capsule::getSegment(&shape->capsule, tx, &a, &b);
				Vector3 on_seg = segment_closestToPoint(&a, &b, center);
				Vector3 d = vector3::difference(center, &on_seg);
				Vector3 dir = vector3::normalized(&d);
				Vector3 closest = on_seg;
				vector3::addScaledVector(&closest, &dir, shape->capsule.radius);
				floorProbe_consider(probe, center, radius, &closest);
				break;
			}
		}
	}
	}
}

/* How far the floor is straight below the capsule's bottom, so the animation
   can start the landing exactly one clip-to-contact away from it. Negative with
   nothing within reach. Sensors are skipped: the water is not a floor to land
   on. */
static float floorDistance(const Character3D *character, const e64::physics::World *world)
{
	const Collider *collider = &character->collider;
	Vector3 down = { 0.0f, 0.0f, -1.0f };

	/* The ray takes the center and is lowered to the capsule's bottom. */
	RaycastData ray;
	raycast::set(&ray, &collider->transform.position, &down, CHARACTER3D_FALL_PROBE_LENGTH);
	ray.start.z -= collider->shape.radius + collider->shape.half_height;

	float distance = -1.0f;

	for (const RigidBody *body = world->body_list; body; body = body->next) {
		if (!(body->flags & RigidBody::BODY_FLAG_STATIC)) continue;

		for (const e64::physics::Shape *shape = body->shapes; shape; shape = shape->next) {
			if (shape->sensor) continue;
			if (!e64::physics::shape::raycast(shape, &ray)) continue;

			/* Closest wins: the ray is shortened so the rest is behind it. */
			distance = ray.toi;
			ray.t = ray.toi;
		}
	}

	return distance;
}

static void findFloor(const Character3D *character, const e64::physics::World *world, FloorProbe *probe)
{
	float radius = character->collider.shape.radius;

	/* Bottom-sphere center, swept down by the snap length. */
	Vector3 center = character->collider.transform.position;
	center.z -= character->collider.shape.half_height + CHARACTER3D_FLOOR_SNAP_LENGTH;

	probeFloor(world, &center, radius, probe);
}

/* Godot's _snap_on_floor conditions: only when the character was on the
   floor, is not on it now, and is not moving up. Vertical-only correction,
   fed by the bottom-sphere floor probe. */
static void snapToFloor(Character3D *character, const FloorProbe *floor,
                        bool was_on_floor, bool velocity_facing_up)
{
	if (character->movement.data.is_grounded || !was_on_floor || velocity_facing_up) return;
	if (!floor->found) return;

	float drop = CHARACTER3D_FLOOR_SNAP_LENGTH - floor->penetration / floor->normal.z;
	if (drop > 0.0f) character->collider.transform.position.z -= drop;

	setGroundResponse(character);
}

/* Water probe: fraction of the capsule under the surface, from the world's
   buoyancy volumes. The character is kinematic, so it never shows up in a
   sensor's contact list — it asks the volumes directly. Read by the movement
   code on the next frame, which is where the fake buoyancy lives. */
static void probeWater(Character3D *character, const e64::physics::World *world)
{
	MovementData *data = &character->movement.data;
	const Collider *collider = &character->collider;

	data->in_water = false;
	data->submerged_fraction = 0.0f;

	float height = 2.0f * (collider->shape.half_height + collider->shape.radius);

	/* The volume is tested at the capsule's bottom, so the water counts from
	   the first contact. */
	Vector3 bottom = collider->transform.position;
	bottom.z -= 0.5f * height;

	for (int32_t i = 0; i < world->buoyancy_count; i++) {
		const buoyancy::Volume *volume = world->buoyancy[i];

		for (const e64::physics::Shape *shape = volume->body->shapes; shape; shape = shape->next) {
			if (!shape->sensor) continue;
			if (!e64::physics::shape::testPoint(shape, &bottom)) continue;

			float surface = volume->surface_height(volume->surface, bottom.x, bottom.y);

			float fraction = (surface - bottom.z) / height;
			if (fraction <= 0.0f) continue;

			data->in_water = true;
			data->submerged_fraction = (fraction > 1.0f) ? 1.0f : fraction;
			return;
		}
	}
}

void collide(Character3D *character, const e64::physics::World *world)
{
	Movement *movement = &character->movement;
	MovementData *data = &movement->data;

	bool was_on_floor = data->is_grounded;
	bool velocity_facing_up = movement->velocity.z > 0.0f;
	data->is_grounded = 0;

	probeWater(character, world);

	CollisionState state = { .wall_depth = -1.0f };
	recover(character, world, &state);
	respond(character, &state, was_on_floor);

	FloorProbe floor;
	findFloor(character, world, &floor);

	snapToFloor(character, &floor, was_on_floor, velocity_facing_up);

	/* Nothing walkable under the probe and no contact resolved as floor.
	   A contact already classified as floor outranks the probe: pressed against
	   a wall the probe can miss the floor it is standing on, and falling on that
	   alone puts the character back on the ground the next frame, one frame at a
	   time, forever.

	   Only the ground modes fall: swimming and climbing have their own exits. */
	if (!floor.found && !data->is_grounded
	 && (movement->mode == MOVEMENT_STATE_IDLE || movement->mode == MOVEMENT_STATE_WALK))
		movement->setMode(MOVEMENT_STATE_FALL);

	/* Only worth measuring off the ground, and only on the way down: it feeds
	   the landing, and a rising body has nothing to time yet. */
	data->floor_distance = (!data->is_grounded && movement->velocity.z < 0.0f)
		? floorDistance(character, world)
		: -1.0f;
}

}

}

}
