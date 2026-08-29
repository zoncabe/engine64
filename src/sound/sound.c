#include <math.h>
#include <malloc.h>
#include <assert.h>

#include "sound/sound.h"
#include "time/time.h"
#include "player/player.h"
#include "viewport/viewport.h"

/* Output rate of the AI. The mixer resamples every voice to it, so its cost is
   paid per output sample: this is the number that sets the RSP budget, not the
   rate of the samples themselves. */
#define SOUND_OUTPUT_RATE 32000

/* Volume changes walk to their new value over one frame instead of jumping,
   which is what keeps a moving emitter from clicking. */
#define SOUND_RAMP_MIN_SAMPLES 16

/* The mixer sums its channels into one accumulator, so voices playing at once
   add up. This is the headroom that keeps a handful of them from running past
   the end of the range. */
#define SOUND_MASTER_VOLUME 0.5f

#define SOUND_EPSILON 1e-4f


typedef struct {

	Vector3 position;
	Vector3 right;

} SoundListener;

typedef struct {

	SoundID id;
	Vector3 position;
	float volume_scale;
	float duration;

	/* Mixer channel the emitter is playing on, or -1 while it is out of
	   range. A looping emitter keeps its slot either way. */
	int channel;

	bool active;

} SoundEmitterState;


/* The game's bank, handed over at sound_init. */
static const SoundDef *sound_bank;
static uint8_t         sound_count;

static wav64_t **sound_wave;
static SoundEmitterState sound_emitter[SOUND_MAX_EMITTERS];
static SoundListener sound_listener;

/* Emitter each channel belongs to. A one-shot frees its channel the moment the
   sample ends, one frame before its emitter notices, so without this the next
   sound to start can take that channel while the old emitter still writes
   volume to it. */
static SoundEmitter sound_channel_owner[SOUND_MIXER_CHANNELS];


void sound_setListener(const Vector3 *position, const Vector3 *right)
{
	sound_listener.position = *position;
	sound_listener.right    = *right;
}


/* Inverse rolloff between the two radii, rescaled so it reaches zero exactly
   at max_distance instead of trailing off forever. */
static float sound_attenuation(const SoundDef *def, float distance)
{
	if (def->max_distance <= 0.0f) return 1.0f;
	if (distance <= def->min_distance) return 1.0f;
	if (distance >= def->max_distance) return 0.0f;

	float near_gain = def->min_distance / distance;
	float far_gain  = def->min_distance / def->max_distance;

	return (near_gain - far_gain) / (1.0f - far_gain);
}


/* A sound on top of the listener has no side to come from: its direction is
   whatever a frame of movement left over, which would slam the panning to one
   end. Inside min_distance it is walked back to the centre. */
static float sound_panning(const SoundDef *def, const Vector3 *to_emitter, float distance)
{
	if (distance < SOUND_EPSILON) return 0.5f;

	Vector3 direction = vector3_scaled(to_emitter, 1.0f / distance);
	float side = vector3_dot(&direction, &sound_listener.right);

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
static void sound_applyMix(int channel, float gain, float pan, int ramp_samples)
{
	if (gain > 1.0f) gain = 1.0f;
	if (gain < 0.0f) gain = 0.0f;

	float left  = sqrtf(1.0f - pan);
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
static void sound_applyStretch(int channel, const waveform_t *wave, float duration)
{
	if (duration <= 0.0f) return;

	float length = (float)wave->len / wave->frequency;
	if (length < SOUND_EPSILON) return;

	float stretch = duration / length;
	if (stretch <= 1.0f) return;

	mixer_ch_set_freq(channel, wave->frequency / stretch);
}


/* True while the channel is still the emitter's to write to. */
static bool sound_ownsChannel(const SoundEmitterState *emitter)
{
	if (emitter->channel < 0) return false;

	return sound_channel_owner[emitter->channel] == (SoundEmitter)(emitter - sound_emitter);
}


static void sound_dropChannel(SoundEmitterState *emitter)
{
	if (sound_ownsChannel(emitter)) {
		mixer_ch_stop(emitter->channel);
		sound_channel_owner[emitter->channel] = SOUND_NO_EMITTER;
	}

	emitter->channel = -1;
}


static bool sound_startEmitter(SoundEmitterState *emitter, float gain, float pan)
{
	const SoundDef *def = &sound_bank[emitter->id];
	wav64_t *wave = sound_wave[emitter->id];

	if (!wave) return false;

	bool stereo = wave->wave.channels == 2;
	int channel;

	if (!mixer_ch_alloc(0, SOUND_MIXER_CHANNELS, 1, stereo,
			def->priority, &wave->wave, &channel))
		return false;

	/* Whatever held it is losing it. Only the owner changes hands: the previous
	   emitter keeps its channel number so sound_update still finds it by the
	   >= 0 test, sees it no longer owns the channel and gives its slot back.
	   Clearing its channel here would hide it from that test forever. */
	if (mixer_ch_playing(channel))
		mixer_ch_stop(channel);

	wav64_play(wave, channel);
	mixer_ch_set_priority(channel, def->priority);
	sound_applyStretch(channel, &wave->wave, emitter->duration);
	sound_applyMix(channel, gain, pan, 0);

	emitter->channel = channel;
	sound_channel_owner[channel] = (SoundEmitter)(emitter - sound_emitter);
	return true;
}


static void sound_releaseEmitter(SoundEmitterState *emitter)
{
	sound_dropChannel(emitter);
	emitter->active = false;
}


void sound_init(const SoundDef *bank, uint8_t count)
{
	sound_bank  = bank;
	sound_count = count;

	sound_wave = calloc(count, sizeof(wav64_t *));
	assert(sound_wave);

	audio_init(SOUND_OUTPUT_RATE, AUDIO_DEFAULT_LATENCY);
	mixer_init(SOUND_MIXER_CHANNELS);
	mixer_set_vol(SOUND_MASTER_VOLUME);

	sound_listener.right = vector3_create(0.0f, -1.0f, 0.0f);

	for (int i = 0; i < sound_count; i++) {
		const SoundDef *def = &sound_bank[i];

		wav64_loadparms_t parms = {
			.streaming_mode = def->preload ? WAV64_STREAMING_NONE
			                               : WAV64_STREAMING_FULL,
		};

		sound_wave[i] = wav64_load(def->path, &parms);

		if (sound_wave[i] && def->loop)
			wav64_set_loop(sound_wave[i], true);
	}

	for (int i = 0; i < SOUND_MAX_EMITTERS; i++)
		sound_emitter[i].channel = -1;

	for (int i = 0; i < SOUND_MIXER_CHANNELS; i++)
		sound_channel_owner[i] = SOUND_NO_EMITTER;
}


void sound_close(void)
{
	sound_stopAll();

	for (int i = 0; i < sound_count; i++) {
		if (!sound_wave[i]) continue;

		wav64_close(sound_wave[i]);
		sound_wave[i] = NULL;
	}

	mixer_close();
	audio_close();
}


SoundEmitter sound_play(SoundID id, const Vector3 *position, float volume_scale, float duration)
{
	if (id >= sound_count || !sound_wave[id]) return SOUND_NO_EMITTER;

	for (int i = 0; i < SOUND_MAX_EMITTERS; i++) {
		SoundEmitterState *emitter = &sound_emitter[i];

		if (emitter->active) continue;

		emitter->id           = id;
		emitter->position     = *position;
		emitter->volume_scale = volume_scale;
		emitter->duration     = duration;
		emitter->channel      = -1;
		emitter->active       = true;

		/* Placed straight away so the first frame already opens at the right
		   volume instead of ramping up from wherever the channel was. */
		const SoundDef *def = &sound_bank[id];

		Vector3 to_emitter = vector3_difference(position, &sound_listener.position);
		float distance = vector3_magnitude(&to_emitter);
		float gain = def->volume * volume_scale * sound_attenuation(def, distance);

		if (gain > 0.0f && !sound_startEmitter(emitter, gain, sound_panning(def, &to_emitter, distance))) {
			emitter->active = false;
			return SOUND_NO_EMITTER;
		}

		/* A one-shot that starts out of range has nothing to wait for. */
		if (gain <= 0.0f && !def->loop) {
			emitter->active = false;
			return SOUND_NO_EMITTER;
		}

		return i;
	}

	return SOUND_NO_EMITTER;
}


void sound_stop(SoundEmitter emitter)
{
	if (emitter < 0 || emitter >= SOUND_MAX_EMITTERS) return;
	if (!sound_emitter[emitter].active) return;

	sound_releaseEmitter(&sound_emitter[emitter]);
}


void sound_stopAll(void)
{
	for (int i = 0; i < SOUND_MAX_EMITTERS; i++) {
		if (sound_emitter[i].active) sound_releaseEmitter(&sound_emitter[i]);
	}
}


void sound_setEmitterPosition(SoundEmitter emitter, const Vector3 *position)
{
	if (emitter < 0 || emitter >= SOUND_MAX_EMITTERS) return;
	if (!sound_emitter[emitter].active) return;

	sound_emitter[emitter].position = *position;
}


void sound_poll(void)
{
	mixer_try_play();
}


/* The ear rides the driven body — the camera floats away on the spring arm,
   and hearing from there detaches the sound from the character. With nobody
   possessed (menus, cutscenes) it falls back to the camera. The right vector
   is always the camera's: panning follows what the screen shows. */
static void sound_updateListener(void)
{
	const Player   *player   = player_get();
	const Viewport *viewport = viewport_get();

	Vector3 ear = player[0].entity ? player[0].entity->transform.position
	                               : viewport->camera.position;
	Vector3 right = camera_getRight(&viewport->camera);

	sound_setListener(&ear, &right);
}


void sound_update(void)
{
	sound_updateListener();

	int ramp_samples = (int)(time_get()->delta * SOUND_OUTPUT_RATE);
	if (ramp_samples < SOUND_RAMP_MIN_SAMPLES) ramp_samples = SOUND_RAMP_MIN_SAMPLES;

	for (int i = 0; i < SOUND_MAX_EMITTERS; i++) {
		SoundEmitterState *emitter = &sound_emitter[i];

		if (!emitter->active) continue;

		const SoundDef *def = &sound_bank[emitter->id];

		/* A one-shot that ran out, or lost its channel to a later sound,
		   gives its slot back. A looping one keeps the emitter and asks for
		   a new channel below. */
		if (emitter->channel >= 0 && (!sound_ownsChannel(emitter) || !mixer_ch_playing(emitter->channel))) {
			if (!def->loop) {
				sound_releaseEmitter(emitter);
				continue;
			}

			sound_dropChannel(emitter);
		}

		Vector3 to_emitter = vector3_difference(&emitter->position, &sound_listener.position);
		float distance = vector3_magnitude(&to_emitter);
		float gain = def->volume * emitter->volume_scale * sound_attenuation(def, distance);

		if (gain <= 0.0f) {
			/* Out of range: the channel is worth more to something audible.
			   The emitter itself stays, waiting for the listener to come
			   back. */
			sound_dropChannel(emitter);

			if (!def->loop) emitter->active = false;
			continue;
		}

		float pan = sound_panning(def, &to_emitter, distance);

		if (emitter->channel < 0) {
			/* A looping emitter keeps asking until the mixer has room. A
			   one-shot that cannot get a channel has missed its moment, and
			   holding the slot open would starve everything after it. */
			if (!sound_startEmitter(emitter, gain, pan) && !def->loop)
				emitter->active = false;

			continue;
		}

		sound_applyMix(emitter->channel, gain, pan, ramp_samples);
	}

	sound_poll();
}
