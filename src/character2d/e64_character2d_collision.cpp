/*
	Port of character3d::physics to the plane, which is itself Godot's
	test_body_motion recovery and CharacterBody3D::_set_collision_direction.
	Detection is the stage's GridCollider2D; what is done with the contacts
	stays here, in Vector3 with z at zero.
*/
#include <math.h>
#include <stdint.h>

#include "character2d/e64_character2d.h"
#include "stage2d/e64_stage2d.h"
#include "math/e64_math.h"
#include "math/e64_vector3.h"
#include "physics2d/collision/e64_grid_collider2d.h"
#include "physics2d/collision/e64_collision2d.h"

namespace e64 {

#define CHARACTER2D_MAX_CONTACTS 16 /* contacts kept per recovery pass */
#define CHARACTER2D_RECOVERY_ATTEMPTS 4 /* Godot: recover_attempts */
#define CHARACTER2D_RECOVERY_MARGIN 0.05f /* Godot's safe margin, in pixels */
#define CHARACTER2D_MIN_CONTACT_DEPTH (CHARACTER2D_RECOVERY_MARGIN * 0.05f) /* Godot: TEST_MOTION_MIN_CONTACT_DEPTH_FACTOR */
#define CHARACTER2D_RECOVERY_FACTOR 0.4f /* Godot: fraction of the depth recovered per pass */
#define CHARACTER2D_FLOOR_SNAP_LENGTH 2.0f /* downward probe, pixels */
#define CHARACTER2D_FALL_PROBE_CELLS 6 /* how far down the landing is looked for */
/* Walkable limit of 50 degrees plus Godot's FLOOR_ANGLE_THRESHOLD of 0.01
   rad, as a cosine: floor is decided on the cosine, never the angle. The
   argument is constant, so gcc folds the cosf at compile time. */
#define CHARACTER2D_FLOOR_MAX_SLOPE_COS cosf(PI / 180 * 50.0f + 0.01f)


namespace character2d {
namespace physics {

typedef struct Contact {
	Vector3 normal; /* from the cell toward the body */
	float depth;
} Contact;

typedef struct CollisionState {
	bool floor;
	bool wall;
	bool ceiling;
	Vector3 wall_normal;
	float wall_depth;
} CollisionState;


/* --- the capsule ------------------------------------------------------------ */

/* The capsule as a physics2d shape, feet at position, its radius grown by
   margin so the contacts reach that far out of the surface. */
static void getCapsule(const Character2D *character, float margin, physics2d::Shape2D *shape)
{
	const ColliderSettings *collider = character->def->collider_settings;
	float half = 0.5f * collider->height;

	shape->type = physics2d::Shape2D::SHAPE_CAPSULE;
	shape->local = (Transform2D){ { 0.0f, -half }, 0.0f };
	shape->world = (Transform2D){ { character->position.x, character->position.y - half }, 0.0f };
	shape->next = NULL;
	shape->owner = (void *)character;
	shape->sensor = 0;
	shape->capsule = (Capsule2D){ collider->radius + margin, half - collider->radius };
}

/* The bottom sphere of the capsule, swept down by the snap length. */
static void getFloorProbe(const Character2D *character, physics2d::Shape2D *shape)
{
	float radius = character->def->collider_settings->radius;
	float offset = -radius + CHARACTER2D_FLOOR_SNAP_LENGTH;

	shape->type = physics2d::Shape2D::SHAPE_CIRCLE;
	shape->local = (Transform2D){ { 0.0f, offset }, 0.0f };
	shape->world = (Transform2D){ { character->position.x, character->position.y + offset }, 0.0f };
	shape->next = NULL;
	shape->owner = (void *)character;
	shape->sensor = 0;
	shape->circle = (Circle2D){ radius };
}


/* --- contacts ---------------------------------------------------------------- */

typedef struct ContactQuery {
	const physics2d::Shape2D *body;
	float margin; /* grown into the body's radius, taken back from the depth */
	Contact *contacts;
	int count;
} ContactQuery;

/* One cell of the grid query: the body against the cell's shape, and the
   manifold turned around into a contact, seen from the cell. */
static int addContact(void *cb, const physics2d::Shape2D *cell, int32_t, int32_t)
{
	ContactQuery *query = (ContactQuery *)cb;

	Contact2D::Manifold m;
	collision2d::collide(&m, query->body, cell);
	if (!m.contact_count) return 1;

	query->contacts[query->count++] = (Contact){
		.normal = vector3::create(-m.normal.x, -m.normal.y, 0.0f),
		.depth = -m.contacts[0].penetration - query->margin,
	};
	return query->count < CHARACTER2D_MAX_CONTACTS;
}

/* Every solid cell the body reaches, as a contact. */
static int queryContacts(const Character2D *character, const physics2d::Shape2D *body, float margin, Contact *contacts)
{
	const Stage2D *stage = character->stage;
	Transform2D world = stage2d::getColliderTransform(stage);

	AABB2D box;
	physics2d::shape2d::computeAABB(body, &box);

	ContactQuery query = { body, margin, contacts, 0 };
	gridCollider2d::queryAABB(stage->collider, &world, &box, &query, addContact);
	return query.count;
}

/* Every solid cell the capsule overlaps, or is within the recovery margin
   of, as a contact. */
static int collectContacts(const Character2D *character, Contact *contacts)
{
	physics2d::Shape2D capsule;
	getCapsule(character, CHARACTER2D_RECOVERY_MARGIN, &capsule);

	return queryContacts(character, &capsule, CHARACTER2D_RECOVERY_MARGIN, contacts);
}

/* Port of CharacterBody3D::_set_collision_direction: every contact of the
   pass is classified by its angle against the up axis, floor, ceiling or
   wall, and the frame state accumulates them. Up is -y here. */
static void classifyContacts(const Contact *contacts, int count, CollisionState *state)
{
	bool was_wall = state->wall;
	bool pass_floor = false;
	bool pass_wall = false;

	int wall_collision_count = 0;
	Vector3 combined_wall_normal = { 0.0f, 0.0f, 0.0f };
	Vector3 tmp_wall_col = { 0.0f, 0.0f, 0.0f };

	for (int i = count - 1; i >= 0; i--) {
		const Contact *c = &contacts[i];

		/* dot(normal, up) == -normal.y; angle <= limit is cosine >= its cosine */
		if (-c->normal.y >= CHARACTER2D_FLOOR_MAX_SLOPE_COS) {
			pass_floor = true;
			state->floor = true;
			continue;
		}

		if (c->normal.y >= CHARACTER2D_FLOOR_MAX_SLOPE_COS) {
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

		Vector3 d = vector3::difference(&c->normal, &tmp_wall_col);
		if (vector3::dot(&d, &d) > 1.0e-6f) {
			tmp_wall_col = c->normal;
			vector3::add(&combined_wall_normal, &c->normal);
			wall_collision_count++;
		}
	}

	/* Two steep walls can add up to walkable support (a wedge). */
	if (pass_wall && wall_collision_count > 1 && !pass_floor) {
		float magnitude = vector3::magnitude(&combined_wall_normal);
		if (magnitude > 1.0e-6f && -combined_wall_normal.y >= CHARACTER2D_FLOOR_MAX_SLOPE_COS * magnitude) {
			state->floor = true;
			state->wall = was_wall;
		}
	}
}

/* Port of test_body_motion STEP 1 (free body if stuck): one recovery vector
   accumulated over every contact and applied whole, up to four attempts. */
static void recover(Character2D *character, CollisionState *state)
{
	int recover_attempts = CHARACTER2D_RECOVERY_ATTEMPTS;

	do {
		Contact contacts[CHARACTER2D_MAX_CONTACTS] __attribute__((uninitialized));
		int count = collectContacts(character, contacts);
		if (!count) break;

		classifyContacts(contacts, count, state);

		Vector3 recover_motion = { 0.0f, 0.0f, 0.0f };
		for (int i = 0; i < count; i++) {
			float depth = contacts[i].depth - vector3::dot(&contacts[i].normal, &recover_motion);
			if (depth > CHARACTER2D_MIN_CONTACT_DEPTH + 1.0e-5f)
				vector3::addScaledVector(&recover_motion, &contacts[i].normal, (depth - CHARACTER2D_MIN_CONTACT_DEPTH) * CHARACTER2D_RECOVERY_FACTOR);
		}

		if (vector3::dot(&recover_motion, &recover_motion) == 0.0f) break;

		character->position.x += recover_motion.x;
		character->position.y += recover_motion.y;
	} while (--recover_attempts);
}


/* --- responses --------------------------------------------------------------- */

static void setGroundResponse(Character2D *character)
{
	MovementData *data = &character->movement.data;

	/* Moving up (jump takeoff): touching the ground must not cancel it. */
	if (data->velocity.y < 0.0f) return;

	data->is_grounded = true;
	data->velocity.y = 0.0f;

	/* The floor is back: the next ledge gets its own coyote window. */
	data->coyote_timer = 0.0f;
	if (character->movement.current == MOVEMENT2D_STATE_FALLING)
		movement::setMode(&character->movement, character->movement.locomotion);
}

static void setCeilingResponse(Character2D *character)
{
	MovementData *data = &character->movement.data;
	if (data->velocity.y < 0.0f) data->velocity.y = 0.0f;
}

/* Slide: remove the velocity component pushing into the wall, keep the
   rest. On the floor only the horizontal part of the wall normal is used,
   so a corner does not inject upward velocity that kills the floor. */
static void setWallResponse(Character2D *character, const CollisionState *state, bool was_on_floor)
{
	MovementData *data = &character->movement.data;

	if (was_on_floor || data->is_grounded) {
		float nx = state->wall_normal.x;
		if (fabsf(nx) < 1.0e-6f) return;
		nx = nx < 0.0f ? -1.0f : 1.0f;

		float t = data->velocity.x * nx;
		if (t < 0.0f) data->velocity.x -= t * nx;
	}
	else {
		float t = data->velocity.x * state->wall_normal.x + data->velocity.y * state->wall_normal.y;
		if (t < 0.0f) {
			data->velocity.x -= t * state->wall_normal.x;
			data->velocity.y -= t * state->wall_normal.y;
		}
	}
}

static void respond(Character2D *character, const CollisionState *state, bool was_on_floor)
{
	if (state->floor) setGroundResponse(character);
	if (state->ceiling) setCeilingResponse(character);
	if (state->wall) setWallResponse(character, state, was_on_floor);
}


/* --- floor ------------------------------------------------------------------- */

/* Floor query: the capsule's bottom sphere swept down by the snap length.
   Answers whether there is walkable floor under the body, how deep the probe
   sinks into it and with which normal. */
typedef struct FloorProbe {
	bool found;
	float penetration;
	Vector3 normal;
} FloorProbe;

static void floorProbe_consider(FloorProbe *probe, const Contact *contact)
{
	/* Walkable floor only. */
	if (-contact->normal.y < CHARACTER2D_FLOOR_MAX_SLOPE_COS) return;

	if (!probe->found || contact->depth > probe->penetration) {
		probe->found = true;
		probe->penetration = contact->depth;
		probe->normal = contact->normal;
	}
}

static void findFloor(const Character2D *character, FloorProbe *probe)
{
	/* Every field written up front: found gates the others, and the
	   compiler cannot see that across the inlining. */
	probe->found = false;
	probe->penetration = 0.0f;
	probe->normal = vector3::create(0.0f, -1.0f, 0.0f);

	physics2d::Shape2D circle;
	getFloorProbe(character, &circle);

	Contact contacts[CHARACTER2D_MAX_CONTACTS] __attribute__((uninitialized));
	int count = queryContacts(character, &circle, 0.0f, contacts);

	for (int i = 0; i < count; i++) floorProbe_consider(probe, &contacts[i]);
}

/* How far the floor is straight below the feet, so the animation can start
   the landing exactly one clip-to-contact away from it. Negative with
   nothing within reach. */
static float floorDistance(const Character2D *character)
{
	const Stage2D *stage = character->stage;
	Transform2D world = stage2d::getColliderTransform(stage);

	return gridCollider2d::distanceDown(stage->collider, &world, &character->position, CHARACTER2D_FALL_PROBE_CELLS);
}

/* Godot's _snap_on_floor conditions: only when the body was on the floor,
   is not on it now, and is not moving up. Vertical-only correction. */
static void snapToFloor(Character2D *character, const FloorProbe *floor, bool was_on_floor, bool velocity_facing_up)
{
	if (character->movement.data.is_grounded || !was_on_floor || velocity_facing_up) return;
	if (!floor->found) return;

	float drop = CHARACTER2D_FLOOR_SNAP_LENGTH - floor->penetration / -floor->normal.y;
	if (drop > 0.0f) character->position.y += drop;

	setGroundResponse(character);
}


void collide(Character2D *character)
{
	MovementData *data = &character->movement.data;

	/* No stage, nothing to stand on or walk into. */
	if (!character->stage) return;

	bool was_on_floor = data->is_grounded;
	bool velocity_facing_up = data->velocity.y < 0.0f;
	data->is_grounded = false;

	CollisionState state = { .wall_depth = -1.0f };
	recover(character, &state);
	respond(character, &state, was_on_floor);

	FloorProbe floor __attribute__((uninitialized));
	findFloor(character, &floor);

	snapToFloor(character, &floor, was_on_floor, velocity_facing_up);

	/* Nothing walkable under the probe and no contact resolved as floor: a
	   fall. Only locomotion falls: a roll runs to the end of its own clip,
	   and a crouch under way holds the body until it launches. */
	if (!floor.found && !data->is_grounded
	 && movement::isLocomotion(character->movement.current)
	 && !movement::isChargingJump(character))
		movement::setMode(&character->movement, MOVEMENT2D_STATE_FALLING);

	if (data->is_grounded) data->grounding_height = character->position.y;

	/* Only worth measuring off the ground, and only on the way down: it
	   feeds the landing, and a rising body has nothing to time yet. */
	data->floor_distance = (!data->is_grounded && data->velocity.y > 0.0f)
		? floorDistance(character)
		: -1.0f;
}

}
}

}
