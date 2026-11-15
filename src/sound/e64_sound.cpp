#include <math.h>
#include <assert.h>

#include "sound/e64_sound.h"
#include "resource/e64_resource.h"
#include "time/e64_time.h"
#include "scene3d/e64_scene3d.h"

namespace e64 {

namespace sound {

/* Output rate of the AI. The mixer resamples every voice to it, so its cost is
   paid per output sample: this is the number that sets the RSP budget, not the
   rate of the samples themselves. */
static constexpr int OUTPUT_RATE = 32000;

/* Volume changes walk to their new value over one frame instead of jumping,
   which is what keeps a moving emitter from clicking. */
static constexpr int RAMP_MIN_SAMPLES = 16;

/* The mixer sums its channels into one accumulator, so voices playing at once
   add up. This is the headroom that keeps a handful of them from running past
   the end of the range. */
static constexpr float MASTER_VOLUME = 0.5f;

static constexpr float EPSILON = 1e-4f;


struct Listener {

	Vector3 position;
	Vector3 right;

};

struct EmitterState {

	const Sound *sound;
	Vector3 position;
	float volume_scale;
	float duration;

	/* Mixer channel the emitter is playing on, or -1 while it is out of
	   range. A looping emitter keeps its slot either way. */
	int channel;

	bool active;

};


static EmitterState emitter_state[Sound::MAX_EMITTERS];
static Listener listener;

/* Emitter each channel belongs to. A one-shot frees its channel the moment the
   sample ends, one frame before its emitter notices, so without this the next
   sound to start can take that channel while the old emitter still writes
   volume to it. */
static Sound::Emitter channel_owner[Sound::MIXER_CHANNELS];


void setListener(const Vector3 *position, const Vector3 *right)
{
	listener.position = *position;
	listener.right = *right;
}


/* Inverse rolloff between the two radii, rescaled so it reaches zero exactly
   at max_distance instead of trailing off forever. */
static float attenuation(const Sound::Def *def, float distance)
{
	if (def->max_distance <= 0.0f) return 1.0f;
	if (distance <= def->min_distance) return 1.0f;
	if (distance >= def->max_distance) return 0.0f;

	float near_gain = def->min_distance / distance;
	float far_gain = def->min_distance / def->max_distance;

	return (near_gain - far_gain) / (1.0f - far_gain);
}


/* A sound on top of the listener has no side to come from: its direction is
   whatever a frame of movement left over, which would slam the panning to one
   end. Inside min_distance it is walked back to the centre. */
static float panning(const Sound::Def *def, const Vector3 *to_emitter, float distance)
{
	if (distance < EPSILON) return 0.5f;

	Vector3 direction = vector3::scaled(to_emitter, 1.0f / distance);
	float side = vector3::dot(&direction, &listener.right);

	if (def->min_distance > 0.0f && distance < def->min_distance)
		side *= distance / def->min_distance;

	float pan = 0.5f + 0.5f * side;

	/* sqrt.s traps on the N64, and rounding alone is enough to push this a
	   hair past either end. */
	if (pan < 0.0f) pan = 0.0f;
	if (pan > 1.0f) pan = 1.0f;

	return pan;
}


/* Attenuation and own volume go to the channel gain; panning goes to the
   left/right volumes. The mixer keeps the two independent, so neither erases
   the other. Constant power keeps a sound crossing the screen from dipping as
   it passes the centre. */
static void applyMix(int channel, float gain, float pan, int ramp_samples)
{
	if (gain > 1.0f) gain = 1.0f;
	if (gain < 0.0f) gain = 0.0f;

	float left = sqrtf(1.0f - pan);
	float right = sqrtf(pan);

	if (ramp_samples > 0) {
		mixer_ch_set_gain_ramp(channel, gain, ramp_samples, mixer_ramp_linear, 0);
		mixer_ch_set_vol_ramp(channel, left, right, ramp_samples);
		return;
	}

	mixer_ch_set_gain(channel, gain);
	mixer_ch_set_vol(channel, left, right);
}


/* Playing a sample slower makes it last longer and drop in pitch by the same
   factor. The caller names the length it needs and gets it. */
static void applyStretch(int channel, const waveform_t *wave, float duration)
{
	if (duration <= 0.0f) return;

	float length = (float)wave->len / wave->frequency;
	if (length < EPSILON) return;

	float stretch = duration / length;
	if (stretch <= 1.0f) return;

	mixer_ch_set_freq(channel, wave->frequency / stretch);
}


/* True while the channel is still the emitter's to write to. */
static bool ownsChannel(const EmitterState *emitter)
{
	if (emitter->channel < 0) return false;

	return channel_owner[emitter->channel] == (Sound::Emitter)(emitter - emitter_state);
}


static void dropChannel(EmitterState *emitter)
{
	if (ownsChannel(emitter)) {
		mixer_ch_stop(emitter->channel);
		channel_owner[emitter->channel] = Sound::NO_EMITTER;
	}

	emitter->channel = -1;
}


static bool startEmitter(EmitterState *emitter, float gain, float pan)
{
	const Sound::Def *def = emitter->sound->def;
	wav64_t *wave = emitter->sound->wave;

	bool stereo = wave->wave.channels == 2;
	int channel;

	if (!mixer_ch_alloc(0, Sound::MIXER_CHANNELS, 1, stereo,
			def->priority, &wave->wave, &channel))
		return false;

	/* Whatever held it is losing it. Only the owner changes hands: the previous
	   emitter keeps its channel number so update still finds it by the >= 0
	   test, sees it no longer owns the channel and gives its slot back.
	   Clearing its channel here would hide it from that test forever. */
	if (mixer_ch_playing(channel))
		mixer_ch_stop(channel);

	wav64_play(wave, channel);
	mixer_ch_set_priority(channel, def->priority);
	applyStretch(channel, &wave->wave, emitter->duration);
	applyMix(channel, gain, pan, 0);

	emitter->channel = channel;
	channel_owner[channel] = (Sound::Emitter)(emitter - emitter_state);
	return true;
}


static void releaseEmitter(EmitterState *emitter)
{
	dropChannel(emitter);
	emitter->active = false;
}


void init(void)
{
	audio_init(OUTPUT_RATE, AUDIO_DEFAULT_LATENCY);
	mixer_init(Sound::MIXER_CHANNELS);
	mixer_set_vol(MASTER_VOLUME);

	listener.right = vector3::create(0.0f, -1.0f, 0.0f);

	for (int i = 0; i < Sound::MAX_EMITTERS; i++)
		emitter_state[i].channel = -1;

	for (int i = 0; i < Sound::MIXER_CHANNELS; i++)
		channel_owner[i] = Sound::NO_EMITTER;
}


void close(void)
{
	stopAll();
	mixer_close();
	audio_close();
}


Sound load(const Sound::Def *def)
{
	assert(def && def->path);

	wav64_loadparms_t parms = {
		.streaming_mode = def->preload ? WAV64_STREAMING_NONE
		                               : WAV64_STREAMING_FULL,
	};

	wav64_t *wave = (wav64_t *)resource::load(def->path, Resource::WAVE, &parms);
	assert(wave);

	if (def->loop)
		wav64_set_loop(wave, true);

	return (Sound){ .def = def, .wave = wave };
}

void unload(Sound *sound)
{
	if (!sound->wave) return;

	/* Nothing may keep playing a file about to close. */
	for (int i = 0; i < Sound::MAX_EMITTERS; i++) {
		if (emitter_state[i].active && emitter_state[i].sound == sound)
			releaseEmitter(&emitter_state[i]);
	}

	resource::unload(sound->wave);
	sound->wave = NULL;
}


Sound::Emitter play(const Sound *sound, const Vector3 *position, float volume_scale, float duration)
{
	if (!sound || !sound->wave) return Sound::NO_EMITTER;

	const Sound::Def *def = sound->def;

	for (int i = 0; i < Sound::MAX_EMITTERS; i++) {
		EmitterState *emitter = &emitter_state[i];

		if (emitter->active) continue;

		emitter->sound = sound;
		emitter->position = *position;
		emitter->volume_scale = volume_scale;
		emitter->duration = duration;
		emitter->channel = -1;
		emitter->active = true;

		/* Placed straight away so the first frame already opens at the right
		   volume instead of ramping up from wherever the channel was. */
		Vector3 to_emitter = vector3::difference(position, &listener.position);
		float distance = vector3::magnitude(&to_emitter);
		float gain = def->volume * volume_scale * attenuation(def, distance);

		if (gain > 0.0f && !startEmitter(emitter, gain, panning(def, &to_emitter, distance))) {
			emitter->active = false;
			return Sound::NO_EMITTER;
		}

		/* A one-shot that starts out of range has nothing to wait for. */
		if (gain <= 0.0f && !def->loop) {
			emitter->active = false;
			return Sound::NO_EMITTER;
		}

		return i;
	}

	return Sound::NO_EMITTER;
}


void stop(Sound::Emitter emitter)
{
	if (emitter < 0 || emitter >= Sound::MAX_EMITTERS) return;
	if (!emitter_state[emitter].active) return;

	releaseEmitter(&emitter_state[emitter]);
}


void stopAll(void)
{
	for (int i = 0; i < Sound::MAX_EMITTERS; i++) {
		if (emitter_state[i].active) releaseEmitter(&emitter_state[i]);
	}
}


void setEmitterPosition(Sound::Emitter emitter, const Vector3 *position)
{
	if (emitter < 0 || emitter >= Sound::MAX_EMITTERS) return;
	if (!emitter_state[emitter].active) return;

	emitter_state[emitter].position = *position;
}


void poll(void)
{
	mixer_try_play();
}


/* NULL: the ear is on the camera. */
static const Vector3 *listener_override = NULL;

void setListenerOverride(const Vector3 *position)
{
	listener_override = position;
}

void clearListenerOverride(void)
{
	listener_override = NULL;
}

/* The ear on the override or on the scene's camera. The right vector is
   always the camera's: panning follows what the screen shows. */
static void updateListener(void)
{
	const Camera3D *camera = &scene3d::get()->camera;

	Vector3 ear = listener_override ? *listener_override
	                                : camera->position;
	Vector3 right = camera->getRight();

	setListener(&ear, &right);
}


void update(void)
{
	updateListener();

	int ramp_samples = (int)(time::get()->delta * OUTPUT_RATE);
	if (ramp_samples < RAMP_MIN_SAMPLES) ramp_samples = RAMP_MIN_SAMPLES;

	for (int i = 0; i < Sound::MAX_EMITTERS; i++) {
		EmitterState *emitter = &emitter_state[i];

		if (!emitter->active) continue;

		const Sound::Def *def = emitter->sound->def;

		/* A one-shot that ran out, or lost its channel to a later sound,
		   gives its slot back. A looping one keeps the emitter and asks for
		   a new channel below. */
		if (emitter->channel >= 0 && (!ownsChannel(emitter) || !mixer_ch_playing(emitter->channel))) {
			if (!def->loop) {
				releaseEmitter(emitter);
				continue;
			}

			dropChannel(emitter);
		}

		Vector3 to_emitter = vector3::difference(&emitter->position, &listener.position);
		float distance = vector3::magnitude(&to_emitter);
		float gain = def->volume * emitter->volume_scale * attenuation(def, distance);

		if (gain <= 0.0f) {
			/* Out of range: the channel is worth more to something audible.
			   The emitter itself stays, waiting for the listener to come
			   back. */
			dropChannel(emitter);

			if (!def->loop) emitter->active = false;
			continue;
		}

		float pan = panning(def, &to_emitter, distance);

		if (emitter->channel < 0) {
			/* A looping emitter keeps asking until the mixer has room. A
			   one-shot that cannot get a channel has missed its moment, and
			   holding the slot open would starve everything after it. */
			if (!startEmitter(emitter, gain, pan) && !def->loop)
				emitter->active = false;

			continue;
		}

		applyMix(emitter->channel, gain, pan, ramp_samples);
	}

	poll();
}

}

}
