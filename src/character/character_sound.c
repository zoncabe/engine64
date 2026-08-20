#include <stdlib.h>

#include "character/character.h"
#include "character/character_sound.h"
#include "entity/entity.h"
#include "sound/sound.h"
#include "time/time.h"


/* Submersion above this keeps the dry footsteps quiet: wading out of the
   ramp is still water, and tile steps from inside it sound wrong. */
#define CHARACTER_SOUND_WET_FRACTION 0.25f


static SoundID characterSound_pick(const SoundID *id, uint8_t count)
{
	return id[rand() % count];
}


/* True when the normalized cycle walked past mark since the previous frame,
   including the frame where the clip wraps around. */
static bool characterSound_crossed(float previous, float current, float mark)
{
	if (previous < 0.0f) return false;

	if (current >= previous) return previous < mark && current >= mark;

	return previous < mark || current >= mark;
}


/* A step at walking pace lands softer than one at a sprint: linear ramp
   from volume_min to volume_max over [0, footstep_speed_max]. A zero
   speed_max opts out of the scaling and every step hits at max. */
static float characterSound_footstepVolume(const CharacterSoundDef *def, float speed)
{
	if (def->footstep_speed_max <= 0.0f) return def->footstep_volume_max;

	float t = speed / def->footstep_speed_max;
	if (t > 1.0f) t = 1.0f;
	if (t < 0.0f) t = 0.0f;

	return def->footstep_volume_min + t * (def->footstep_volume_max - def->footstep_volume_min);
}


/* One step each time the cycle walks past a footing mark. The cycle is the
   phase of the grid's dominant clip, left behind by the graph, so the marks
   land on the feet no matter which gait carries the blend. Grounded and in
   locomotion only: airborne or rolling feet touch nothing, and the swim
   reads the same cycle through its own marks. */
static void characterSound_updateFootsteps(Character *character, const CharacterSoundDef *def)
{
	const CharacterMovement *movement = &character->movement;

	if (!def->footstep_count || !def->footing_count) return;
	if (!movement->data.is_grounded) return;
	if (!characterMovement_isLocomotion(movement->current)) return;

	/* Wading: no dry steps until the body is mostly out of the water. */
	if (movement->data.in_water && movement->data.submerged_fraction > CHARACTER_SOUND_WET_FRACTION) return;

	float cycle = character->animation.locomotion_cycle;
	float speed = movement->data.horizontal_speed;

	for (int i = 0; i < def->footing_count; i++) {
		if (!characterSound_crossed(character->sound.previous_cycle, cycle, def->footing[i]))
			continue;

		sound_play(characterSound_pick(def->footstep, def->footstep_count),
			&character->entity->transform.position,
			characterSound_footstepVolume(def, speed), 0.0f);

		/* Clock reading for whoever must keep distance from a step: the
		   roll's launch foot skips itself when one just landed. */
		character->sound.last_footstep = time_get()->counter;
	}
}


/* The launch is still in the air: the body only starts scraping the floor once
   the roll timer reaches roll_ground_time, and that is where the noise is. */
static void characterSound_updateRoll(Character *character, const CharacterSoundDef *def)
{
	if (character->movement.current != MOVEMENT_STATE_ROLLING) return;

	const CharacterMovementSettings *settings = character->movement.settings;

	float previous = character->sound.previous_roll_timer;
	float timer    = character->movement.data.roll_timer;

	float start = settings->roll_ground_time + def->roll_delay;

	/* The timer is reset on the way out, so a zero behind a running one is
	   the frame the roll started: the foot pushing off. Dropped when a
	   footstep just played, or the two land too close together. */
	if (def->footstep_count && previous == 0.0f && timer > 0.0f
	    && time_get()->counter - character->sound.last_footstep >= def->roll_launch_gap)
		sound_play(characterSound_pick(def->footstep, def->footstep_count),
			&character->entity->transform.position,
			def->roll_launch_volume, 0.0f);

	/* The body scrapes the floor until grip: the sample is asked to cover
	   what is left of that, and slows down as much as that takes. */
	if (def->roll_count && previous < start && timer >= start)
		sound_play(characterSound_pick(def->roll, def->roll_count),
			&character->entity->transform.position,
			def->roll_volume, settings->roll_grip_time - start);

	/* Grip is the foot planting to come out of the roll. */
	if (def->footstep_count && previous < settings->roll_grip_time && timer >= settings->roll_grip_time)
		sound_play(characterSound_pick(def->footstep, def->footstep_count),
			&character->entity->transform.position,
			def->roll_stand_volume, 0.0f);
}


/* Runs after the movement and animation updates, on the pose the frame is
   about to render: both edges it looks for are set by then. */
/* The charge is the crouch; the launch is the body leaving the floor. The
   noise belongs to the second, so it fires when the timer crosses the end of
   the first. */
static void characterSound_updateJump(Character *character, const CharacterSoundDef *def)
{
	if (!def->jump_count) return;
	if (character->movement.current != MOVEMENT_STATE_JUMPING) return;

	float timer = character->movement.data.jump_timer;

	/* The timer is reset on entry, so a zero behind a running one is the frame
	   the crouch started. */
	if (character->sound.previous_jump_timer != 0.0f || timer <= 0.0f) return;

	/* The jump set is the footstep samples: taking off mid stride right
	   after a step would flam two of them, same case as the roll launch. */
	if (time_get()->counter - character->sound.last_footstep < def->jump_launch_gap) return;

	/* jump_anim_air is where the clip has the body leaving the floor: the
	   sample is stretched to cover exactly that. */
	sound_play(characterSound_pick(def->jump, def->jump_count),
		&character->entity->transform.position, def->jump_volume,
		character->animation.def->settings->jump_anim_air);
}


static void characterSound_updateLanding(Character *character, const CharacterSoundDef *def)
{
	if (!def->land_count) return;

	bool grounded = character->movement.data.is_grounded;

	if (grounded == character->sound.previous_grounded || !grounded) return;

	/* Falling is negative, and a landing this system reaches after the
	   collision already cleared it reads zero: the previous frame holds it. */
	float speed = -character->sound.previous_fall_speed;
	float volume = def->land_volume_max;

	if (def->land_speed_max > 0.0f) {
		float t = speed / def->land_speed_max;
		if (t > 1.0f) t = 1.0f;
		if (t < 0.0f) t = 0.0f;

		volume = def->land_volume_min + t * (def->land_volume_max - def->land_volume_min);
	}

	sound_play(characterSound_pick(def->land, def->land_count),
		&character->entity->transform.position, volume, 0.0f);
}


static void characterSound_updateSwim(Character *character, const CharacterSoundDef *def)
{
	const CharacterMovement *movement = &character->movement;

	/* Splash on the frame the body enters the water, scaled by the plunge:
	   the minimum sits near zero, so wading in from the ramp is close to
	   silent and a jump from the deck slaps. */
	if (def->splash_count && movement->data.in_water && !character->sound.previous_in_water) {
		float speed  = -character->sound.previous_plunge_speed;
		float volume = def->splash_volume_max;

		if (def->splash_speed_max > 0.0f) {
			float t = speed / def->splash_speed_max;
			if (t > 1.0f) t = 1.0f;
			if (t < 0.0f) t = 0.0f;

			volume = def->splash_volume_min + t * (def->splash_volume_max - def->splash_volume_min);
		}

		sound_play(characterSound_pick(def->splash, def->splash_count),
			&character->entity->transform.position, volume, 0.0f);
	}

	/* Strokes on their phases of the swim cycle, which owns the dominant
	   clip while the state holds. Treading water pulls no arm, so nothing
	   fires until the body actually swims. */
	if (movement->current != MOVEMENT_STATE_SWIMMING) return;
	if (!def->stroke_count) return;

	const CharacterMovementSettings *settings = movement->settings;
	float speed = movement->data.horizontal_speed;
	if (speed < settings->swim_slow_speed * 0.5f) return;

	bool heavy = speed > (settings->swim_slow_speed + settings->swim_fast_speed) * 0.5f;
	const SoundID *stroke = heavy ? def->swim_stroke_heavy       : def->swim_stroke_light;
	uint8_t count         = heavy ? def->swim_stroke_heavy_count : def->swim_stroke_light_count;
	if (!count) return;

	float cycle = character->animation.locomotion_cycle;

	for (int i = 0; i < def->stroke_count; i++) {
		if (!characterSound_crossed(character->sound.previous_cycle, cycle, def->stroke[i]))
			continue;

		sound_play(characterSound_pick(stroke, count),
			&character->entity->transform.position, def->stroke_volume, 0.0f);
	}
}


void characterSound_update(Character *character)
{
	const CharacterSoundDef *def = character->sound.def;

	if (!def) return;

	characterSound_updateFootsteps(character, def);
	characterSound_updateRoll(character, def);
	characterSound_updateJump(character, def);
	characterSound_updateLanding(character, def);
	characterSound_updateSwim(character, def);

	character->sound.previous_cycle      = character->animation.locomotion_cycle;
	character->sound.previous_roll_timer = character->movement.data.roll_timer;
	character->sound.previous_jump_timer = character->movement.data.jump_timer;
	character->sound.previous_grounded   = character->movement.data.is_grounded;
	character->sound.previous_in_water   = character->movement.data.in_water;

	if (!character->movement.data.is_grounded)
		character->sound.previous_fall_speed = character->body.velocity.z;

	if (!character->movement.data.in_water)
		character->sound.previous_plunge_speed = character->body.velocity.z;
}
