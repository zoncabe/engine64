#ifndef ENGINE64_SOUND_H
#define ENGINE64_SOUND_H

#include <libdragon.h>
#include "math/e64_vector3.h"

namespace e64 {

/* A def with its file open: what whoever plays it keeps, from load to
   unload, and the only thing the emitters play. The file itself lives in
   the resource table, shared with whoever else loads the same path. */
class Sound {

public:

	static constexpr uint8_t MAX_EMITTERS = 12;

	/* Mixer channels the emitters draw from. A stereo sample takes two of
	   them. */
	static constexpr uint8_t MIXER_CHANNELS = 16;

	static constexpr uint8_t PRIORITY_ONESHOT = 64;
	static constexpr uint8_t PRIORITY_AMBIENCE = 128;

	/* Returned by sound::play; NO_EMITTER when the bank or the mixer had
	   nothing left to give. */
	typedef int Emitter;
	static constexpr Emitter NO_EMITTER = -1;

	/* What a sound is: its file and how it plays. Const data, declared by
	   the game next to whatever uses it. Nothing here changes at runtime.

	   Every sound is positional: the two radii below are what decide its
	   volume, and its own volume only scales the result. A sound meant to
	   play flat sets max_distance to 0. */
	struct Def {

		const char *path;

		/* Own volume of the sample, in [0..1]. Multiplies the distance
		   gain. */
		float volume;

		/* Inside min_distance the sound plays at its own volume; past
		   max_distance it is silent and gives its mixer channels back. */
		float min_distance;
		float max_distance;

		bool loop;

		/* What fires this sound. A number the game defines; the engine only
		   matches it. Several sounds of one prefab may share a trigger: one
		   of them is picked at random when it fires. Ignored for loops. */
		uint8_t trigger;

		/* Voice stealing weight: ambience outlives one-shots. */
		uint8_t priority;

		/* Short samples are decoded into RAM once, so firing one costs no
		   DMA. Long ones stream from ROM, which is the only thing that
		   fits. */
		bool preload;

	};

	const Def *def;
	wav64_t *wave;

};


namespace sound {

/* Brings the mixer up. Runs once, before any scene loads; nothing is
   opened here. */
void init(void);
void close(void);

/* Opens a def's file through the resource table. Whoever loads a sound
   unloads it; unloading cuts whatever emitter was still playing it. */
Sound load(const Sound::Def *def);
void unload(Sound *sound);

/* Where the world is heard from. Position decides attenuation, right decides
   panning: the caller is free to take them from different places. */
void setListener(const Vector3 *position, const Vector3 *right);

/* sound::update puts the ear on the camera each frame, unless an override
   points it at another position, as Unreal's SetAudioListenerOverride does:
   the sound sticks to a body while the camera floats on its arm. Panning
   always follows the camera. The position is read every frame, so it has to
   outlive the override: whoever sets it clears it before that memory goes
   away. */
void setListenerOverride(const Vector3 *position);
void clearListenerOverride(void);

/* Runs from the game loop, in every state: the mixer has to be polled whether
   or not a scene is loaded. */
void update(void);

/* Feeds the mixer without touching the emitters. A single call per frame runs
   dry whenever a frame stretches, so this goes around the expensive parts of
   the loop as well. */
void poll(void);

/* Starts the sound at a point in the world. Looping sounds hold their emitter
   until stop; one-shots release it when the sample ends. The sound has to
   outlive its emitters: unloading it cuts them.

   volume_scale scales the sound's own volume, for a noise that is the same
   sample at different strengths. duration asks the sample to last that many
   seconds, slowing it down as much as that takes; 0 plays it at its
   own speed. */
Sound::Emitter play(const Sound *sound, const Vector3 *position, float volume_scale, float duration);
void stop(Sound::Emitter emitter);
void stopAll(void);

/* For emitters that follow something that moves. */
void setEmitterPosition(Sound::Emitter emitter, const Vector3 *position);

}

}

#endif
