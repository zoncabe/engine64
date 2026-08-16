#ifndef CHARACTER_SOUND_H
#define CHARACTER_SOUND_H

#include <stdbool.h>
#include <stdint.h>

#include "sound/sound_bank.h"

typedef struct Character Character;

#define CHARACTER_SOUND_MAX_VARIATIONS 8
#define CHARACTER_SOUND_MAX_FOOTINGS   4


/* What a character sounds like. Sits in its def next to the animation and
   movement settings: two bodies can walk the same graph and step on different
   samples. */
typedef struct CharacterSoundDef {

	/* One is picked at random per step, so the same noise does not repeat. */
	const SoundID *footstep;
	uint8_t footstep_count;

	/* Points of the locomotion clip where a foot lands, as normalized time.
	   They belong to the clip, not to the sound: a gait that lands its feet
	   elsewhere moves them. */
	const float *footing;
	uint8_t footing_count;

	/* A step at walking pace should not hit as hard as one at a sprint. */
	float footstep_volume_min;
	float footstep_volume_max;
	float footstep_speed_max;

	const SoundID *roll;
	uint8_t roll_count;
	float roll_volume;

	/* Seconds to wait past roll_ground_time. The movement settings say when
	   the body reaches the floor; this says how far into that the sample
	   wants to come in. */
	float roll_delay;

	/* Fires when the crouch ends and the body leaves the floor. */
	const SoundID *jump;
	uint8_t jump_count;
	float jump_volume;

	/* Fires on touchdown, scaled by how fast the body was falling. */
	const SoundID *land;
	uint8_t land_count;
	float land_volume_min;
	float land_volume_max;
	float land_speed_max;

} CharacterSoundDef;


/* Edge detection state. Nothing here is worth saving. */
typedef struct CharacterSound {

	const CharacterSoundDef *def;

	/* Negative until the first update: with no frame behind it, every mark
	   of the cycle would read as just crossed. */
	float previous_cycle;

	float previous_roll_timer;
	float previous_jump_timer;

	/* The collision zeroes the fall before this system runs, so the speed of
	   the impact is the one the previous frame carried. */
	float previous_fall_speed;
	bool previous_grounded;

} CharacterSound;


void characterSound_update(Character *character);

#endif
