#include <stdlib.h>

#include "character/character.h"
#include "character/character_sound.h"
#include "entity/entity.h"
#include "sound/sound.h"


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


static float characterSound_footstepVolume(const CharacterSoundDef *def, float speed)
{
	if (def->footstep_speed_max <= 0.0f) return def->footstep_volume_max;

	float t = speed / def->footstep_speed_max;
	if (t > 1.0f) t = 1.0f;
	if (t < 0.0f) t = 0.0f;

	return def->footstep_volume_min + t * (def->footstep_volume_max - def->footstep_volume_min);
}


static void characterSound_updateFootsteps(Character *character, const CharacterSoundDef *def)
{
	const CharacterMovement *movement = &character->movement;

	if (!def->footstep_count || !def->footing_count) return;
	if (!movement->data.is_grounded) return;
	if (!characterMovement_isLocomotion(movement->current)) return;

	float cycle = character->animation.locomotion_cycle;
	float speed = movement->data.horizontal_speed;

	for (int i = 0; i < def->footing_count; i++) {
		if (!characterSound_crossed(character->sound.previous_cycle, cycle, def->footing[i]))
			continue;

		sound_play(characterSound_pick(def->footstep, def->footstep_count),
			&character->entity->transform.position,
			characterSound_footstepVolume(def, speed), 0.0f);
	}
}


/* The launch is still in the air: the body only starts scraping the floor once
   the roll timer reaches roll_ground_time, and that is where the noise is. */
static void characterSound_updateRoll(Character *character, const CharacterSoundDef *def)
{
	if (!def->roll_count) return;
	if (character->movement.current != MOVEMENT_STATE_ROLLING) return;

	const CharacterMovementSettings *settings = character->movement.settings;

	float start = settings->roll_ground_time + def->roll_delay;
	float timer = character->movement.data.roll_timer;

	if (character->sound.previous_roll_timer >= start || timer < start) return;

	/* The body scrapes the floor until grip: the sample is asked to cover
	   what is left of that, and slows down as far as its def allows. */
	sound_play(characterSound_pick(def->roll, def->roll_count),
		&character->entity->transform.position,
		def->roll_volume, settings->roll_grip_time - start);
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

	float charge = character->movement.settings->jump_timer_max;
	float timer  = character->movement.data.jump_timer;

	if (character->sound.previous_jump_timer >= charge || timer < charge) return;

	sound_play(characterSound_pick(def->jump, def->jump_count),
		&character->entity->transform.position, def->jump_volume, 0.0f);
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


void characterSound_update(Character *character)
{
	const CharacterSoundDef *def = character->sound.def;

	if (!def) return;

	characterSound_updateFootsteps(character, def);
	characterSound_updateRoll(character, def);
	characterSound_updateJump(character, def);
	characterSound_updateLanding(character, def);

	character->sound.previous_cycle      = character->animation.locomotion_cycle;
	character->sound.previous_roll_timer = character->movement.data.roll_timer;
	character->sound.previous_jump_timer = character->movement.data.jump_timer;
	character->sound.previous_grounded   = character->movement.data.is_grounded;

	if (!character->movement.data.is_grounded)
		character->sound.previous_fall_speed = character->body.velocity.z;
}
