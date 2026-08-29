#ifndef SOUND_H
#define SOUND_H

#include <libdragon.h>
#include "physics/math/vector3.h"

/* Index into the bank the game hands to sound_init. */
typedef uint8_t SoundID;

#define SOUND_MAX_EMITTERS 12

/* Mixer channels the emitters draw from. A stereo sample takes two of them. */
#define SOUND_MIXER_CHANNELS 16

#define SOUND_PRIORITY_ONESHOT  64
#define SOUND_PRIORITY_AMBIENCE 128

/* Returned by sound_play; SOUND_NO_EMITTER when the bank or the mixer had
   nothing left to give. */
typedef int SoundEmitter;
#define SOUND_NO_EMITTER (-1)


/* Every sound is positional: the two radii below are what decide its volume,
   and its own volume only scales the result. A sound meant to play flat sets
   max_distance to 0. */
typedef struct SoundDef {

	const char *path;

	/* Own volume of the sample, in [0..1]. Multiplies the distance gain. */
	float volume;

	/* Inside min_distance the sound plays at its own volume; past
	   max_distance it is silent and gives its mixer channels back. */
	float min_distance;
	float max_distance;

	bool loop;

	/* Voice stealing weight: ambience outlives one-shots. */
	uint8_t priority;

	/* Short samples are decoded into RAM once, so firing one costs no DMA.
	   Long ones stream from ROM, which is the only thing that fits. */
	bool preload;


} SoundDef;

/* Hands the engine the game's sound bank and brings the mixer up. Runs
   once, before any scene loads. */
void sound_init(const SoundDef *bank, uint8_t count);
void sound_close(void);

/* Where the world is heard from. Position decides attenuation, right decides
   panning: the caller is free to take them from different places. */
void sound_setListener(const Vector3 *position, const Vector3 *right);

/* Runs from the game loop, in every state: the mixer has to be polled whether
   or not a scene is loaded. */
void sound_update(void);

/* Feeds the mixer without touching the emitters. A single call per frame runs
   dry whenever a frame stretches, so this goes around the expensive parts of
   the loop as well. */
void sound_poll(void);

/* Starts the sound at a point in the world. Looping sounds hold their emitter
   until sound_stop; one-shots release it when the sample ends.

   volume_scale scales the sound's own volume, for a noise that is the same
   sample at different strengths. duration asks the sample to last that many
   seconds, slowing it down as much as that takes; 0 plays it at its
   own speed. */
SoundEmitter sound_play(SoundID id, const Vector3 *position, float volume_scale, float duration);
void sound_stop(SoundEmitter emitter);
void sound_stopAll(void);

/* For emitters that follow something that moves. */
void sound_setEmitterPosition(SoundEmitter emitter, const Vector3 *position);

#endif
