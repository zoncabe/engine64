#include <assert.h>
#include <stdlib.h>

#include "sound/asset_sound.h"
#include "sound/sound.h"
#include "entity/entity.h"
#include "scene3d/scene3d.h"
#include "physics/collision/contact.h"
#include "physics/body/rigid_body.h"
#include "physics/world/physics_world.h"
#include "physics/math/math_common.h"

/* The measure of a hit is the impulse the solver spent stopping it, over the
   body's mass: the speed it killed, in m/s — one scale for every weight.
   Below the floor it is resting jitter and stays silent. */
#define ASSET_SOUND_HIT_SPEED_MIN  0.4f
#define ASSET_SOUND_HIT_SPEED_MAX  3.0f
#define ASSET_SOUND_HIT_VOLUME_MIN 0.1f
#define ASSET_SOUND_HIT_VOLUME_MAX 1.0

/* The plunge is the vertical speed on the frame the body meets the water:
   rolling in barely whispers, a fall from the deck slaps. No cutoff floor —
   entering slowly still wets. */
#define ASSET_SOUND_PLUNGE_SPEED_MIN  0.5f
#define ASSET_SOUND_PLUNGE_SPEED_MAX  6.0f
#define ASSET_SOUND_PLUNGE_VOLUME_MIN 0.1f
#define ASSET_SOUND_PLUNGE_VOLUME_MAX 0.4f

/* The loaded scene's placements: the one registry that knows both the
   asset (what sounds) and the entity it produced (where it is). */
static const Scene3DDef *scene_def;

/* Ambient emitters, partitioned once at start: the ones on a moving body
   first, so the per-frame follow walks only 0..mover_count and a static
   entity is never touched again. */
static struct {
	SoundEmitter emitter;
	const RigidBody *body;
} ambient[SCENE_MAX_ENTITIES];
static uint8_t ambient_count;
static uint8_t mover_count;


static void assetSound_ambient(const Entity *entity, const AssetSound *sound)
{
	assert(ambient_count < SCENE_MAX_ENTITIES);

	SoundEmitter emitter = sound_play(sound->sound[rand() % sound->count],
		&entity->transform.position, 1.0f, 0.0f);

	bool moves = entity->body && (entity->body->flags & BODY_FLAG_DYNAMIC);
	if (moves) {
		ambient[ambient_count] = ambient[mover_count];
		ambient[mover_count].emitter = emitter;
		ambient[mover_count].body    = entity->body;
		mover_count++;
	} else {
		ambient[ambient_count].emitter = emitter;
		ambient[ambient_count].body    = NULL;
	}
	ambient_count++;
}

void assetSound_start(const Scene3DDef *def)
{
	scene_def = def;

	for (uint8_t i = 0; i < def->asset_count; i++) {
		const Scene3DAsset *placed = &def->asset[i];

		for (uint8_t s = 0; s < placed->asset->sound_count; s++) {
			if (placed->asset->sound[s].trigger != ASSET_SOUND_AMBIENT) continue;
			assetSound_ambient(placed->entity, &placed->asset->sound[s]);
		}
	}
}

void assetSound_stop(void)
{
	for (uint8_t i = 0; i < ambient_count; i++)
		sound_stop(ambient[i].emitter);
	ambient_count = 0;
	mover_count   = 0;
	scene_def     = NULL;
}

/* Only a placement ties a body back to what its asset declared. */
static const Asset *assetSound_assetOf(const RigidBody *body)
{
	for (uint8_t i = 0; i < scene_def->asset_count; i++) {
		const Entity *entity = scene_def->asset[i].entity;
		if (entity && entity->body == body) return scene_def->asset[i].asset;
	}
	return NULL;
}

static bool assetSound_declares(const RigidBody *body, AssetSoundTrigger trigger)
{
	const Asset *asset = assetSound_assetOf(body);
	if (asset == NULL) return false;

	for (uint8_t i = 0; i < asset->sound_count; i++)
		if (asset->sound[i].trigger == trigger) return true;
	return false;
}

static void assetSound_collision(const RigidBody *body, float impulse)
{
	const Asset *asset = assetSound_assetOf(body);
	if (asset == NULL || asset->sound == NULL) return;

	float speed = impulse * body->inv_mass;
	if (speed < ASSET_SOUND_HIT_SPEED_MIN) return;

	float t = (speed - ASSET_SOUND_HIT_SPEED_MIN)
	        / (ASSET_SOUND_HIT_SPEED_MAX - ASSET_SOUND_HIT_SPEED_MIN);
	if (t > 1.0f) t = 1.0f;

	float volume = ASSET_SOUND_HIT_VOLUME_MIN
	             + t * (ASSET_SOUND_HIT_VOLUME_MAX - ASSET_SOUND_HIT_VOLUME_MIN);

	/* The body lives in metres; the emitters in render units. */
	Vector3 position = vector3_scaled(&body->tx.position, RENDER_SCALE);

	for (uint8_t i = 0; i < asset->sound_count; i++) {
		const AssetSound *sound = &asset->sound[i];
		if (sound->trigger != ASSET_SOUND_COLLISION) continue;

		sound_play(sound->sound[rand() % sound->count], &position, volume, 0.0f);
	}
}

static void assetSound_waterEntry(const RigidBody *body, float plunge_speed)
{
	const Asset *asset = assetSound_assetOf(body);
	if (asset == NULL || asset->sound == NULL) return;

	float t = (plunge_speed - ASSET_SOUND_PLUNGE_SPEED_MIN)
	        / (ASSET_SOUND_PLUNGE_SPEED_MAX - ASSET_SOUND_PLUNGE_SPEED_MIN);
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;

	float volume = ASSET_SOUND_PLUNGE_VOLUME_MIN
	             + t * (ASSET_SOUND_PLUNGE_VOLUME_MAX - ASSET_SOUND_PLUNGE_VOLUME_MIN);

	/* The body lives in metres; the emitters in render units. */
	Vector3 position = vector3_scaled(&body->tx.position, RENDER_SCALE);

	for (uint8_t i = 0; i < asset->sound_count; i++) {
		const AssetSound *sound = &asset->sound[i];
		if (sound->trigger != ASSET_SOUND_WATER_ENTRY) continue;

		sound_play(sound->sound[rand() % sound->count], &position, volume, 0.0f);
	}
}

void assetSound_update(struct PhysicsWorld *world)
{
	/* The ambient sound follows its body: a carried or pushed prop keeps
	   emitting from where it is. */
	for (uint8_t i = 0; i < mover_count; i++) {
		Vector3 position = vector3_scaled(&ambient[i].body->tx.position, RENDER_SCALE);
		sound_setEmitterPosition(ambient[i].emitter, &position);
	}

	/* The constraint's first colliding frame, tracked by the engine itself,
	   so no state lives here. A solid contact is a hit; a sensor one is a
	   body meeting a water volume. */
	for (const ContactConstraint *c = world->contact_manager.contact_list; c; c = c->next) {
		if (!(c->flags & CONSTRAINT_COLLIDING))    continue;
		if (  c->flags & CONSTRAINT_WAS_COLLIDING) continue;

		if (c->manifold.sensor) {
			/* The wet side is whichever body belongs to a water asset;
			   the other one just fell in, at its own vertical speed. */
			const Asset *asset_a = assetSound_assetOf(c->body_a);
			const Asset *asset_b = assetSound_assetOf(c->body_b);

			if (asset_a && asset_a->type == ASSET_WATER)
				assetSound_waterEntry(c->body_b, -c->body_b->linear_velocity.z);
			else if (asset_b && asset_b->type == ASSET_WATER)
				assetSound_waterEntry(c->body_a, -c->body_a->linear_velocity.z);
			continue;
		}

		float impulse = 0.0f;
		for (int32_t i = 0; i < c->manifold.contact_count; i++)
			impulse += c->manifold.contacts[i].normal_impulse;

		bool a_hits = (c->body_a->flags & BODY_FLAG_DYNAMIC)
		           && assetSound_declares(c->body_a, ASSET_SOUND_COLLISION);
		bool b_hits = (c->body_b->flags & BODY_FLAG_DYNAMIC)
		           && assetSound_declares(c->body_b, ASSET_SOUND_COLLISION);

		/* When both sides would sound, the heavier body owns the hit: one
		   thud per contact, not two stacked. */
		if (a_hits && b_hits) {
			if (c->body_a->mass >= c->body_b->mass) b_hits = false;
			else                                    a_hits = false;
		}

		if (a_hits) assetSound_collision(c->body_a, impulse);
		if (b_hits) assetSound_collision(c->body_b, impulse);
	}
}
