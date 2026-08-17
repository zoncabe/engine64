#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "screen/screen_animation.h"
#include "physics/math/math_common.h"


typedef float (*EaseFunction)(float t);

static const EaseFunction ease_function[SCREEN_ANIMATION_EASING_COUNT] = {
	[SCREEN_ANIMATION_EASING_LINEAR]       = ease_linear,
	[SCREEN_ANIMATION_EASING_QUAD_IN]      = ease_quad_in,
	[SCREEN_ANIMATION_EASING_QUAD_OUT]     = ease_quad_out,
	[SCREEN_ANIMATION_EASING_QUAD_IN_OUT]  = ease_quad_in_out,
	[SCREEN_ANIMATION_EASING_CUBIC_IN]     = ease_cubic_in,
	[SCREEN_ANIMATION_EASING_CUBIC_OUT]    = ease_cubic_out,
	[SCREEN_ANIMATION_EASING_CUBIC_IN_OUT] = ease_cubic_in_out,
	[SCREEN_ANIMATION_EASING_EXPO_IN]      = ease_expo_in,
	[SCREEN_ANIMATION_EASING_EXPO_OUT]     = ease_expo_out,
	[SCREEN_ANIMATION_EASING_EXPO_IN_OUT]  = ease_expo_in_out,
};


static float screenAnimationTrack_end(const ScreenAnimationTrack *track)
{
	return track->delay + track->duration;
}

static float screenAnimation_duration(const ScreenAnimation *animation)
{
	float max = 0.0f;
	for (int i = 0; i < animation->track_count; i++) {
		float end = screenAnimationTrack_end(&animation->track[i]);
		if (end > max) max = end;
	}
	return max;
}

/* In reverse the last track to arrive is the first to leave: a track's
   reverse delay is (max forward delay - its own), so everything moves from
   frame 0 and the stagger flips. */
static float screenAnimation_maxDelay(const ScreenAnimation *animation)
{
	float max = 0.0f;
	for (int i = 0; i < animation->track_count; i++) {
		float d = animation->track[i].delay;
		if (d > max) max = d;
	}
	return max;
}

static float screenAnimation_reverseDuration(const ScreenAnimation *animation)
{
	float max_delay = screenAnimation_maxDelay(animation);
	float max = 0.0f;
	for (int i = 0; i < animation->track_count; i++) {
		float end = (max_delay - animation->track[i].delay) + animation->track[i].duration;
		if (end > max) max = end;
	}
	return max;
}

static void screenAnimationTrack_writeText(const ScreenAnimationTrack *track)
{
	if (!track->target_text || track->target_size == 0) return;

	if (track->source_bool && track->source_label) {
		const char *label = track->source_label[*track->source_bool ? 1 : 0];
		strncpy(track->target_text, label, track->target_size - 1);
		track->target_text[track->target_size - 1] = '\0';
		return;
	}
	if (track->source_u8 && track->source_label) {
		const char *label = track->source_label[*track->source_u8];
		strncpy(track->target_text, label, track->target_size - 1);
		track->target_text[track->target_size - 1] = '\0';
		return;
	}
	if (track->source_u8 && track->format) {
		snprintf(track->target_text, track->target_size, track->format, *track->source_u8);
		return;
	}
}

static void screenAnimationPlayer_applyTrack(const ScreenAnimationTrack *track, float time)
{
	if (track->target_text) {
		screenAnimationTrack_writeText(track);
		return;
	}

	if (track->source_int && track->values_by_index) {
		float value = track->values_by_index[*track->source_int];
		if (track->target)    *track->target    = value;
		if (track->target_u8) *track->target_u8 = (uint8_t)value;
		return;
	}

	if (track->target_bool) {
		float end = track->delay + track->duration;
		bool  in_window = (time >= track->delay) && (track->duration <= 0.0f || time < end);
		*track->target_bool = in_window ? track->to_bool : track->from_bool;
		return;
	}

	if (!track->target && !track->target_u8) return;

	/* A pending track writes nothing: start primes every target, so chained
	   fades over one target hold whatever the last expired track left. */
	float local = time - track->delay;
	if (local < 0.0f) return;

	float t;
	if (track->duration > 0.0f) {
		t = local / track->duration;
		if (t > 1.0f) t = 1.0f;
	} else {
		t = 1.0f;
	}

	float eased = ease_function[track->easing](t);
	float value = lerpf(track->from, track->to, eased);

	if (track->target)    *track->target    = value;
	if (track->target_u8) *track->target_u8 = (uint8_t)value;
}

static void screenAnimationPlayer_applyTrackReversed(const ScreenAnimationTrack *track, float time, float reverse_delay)
{
	if (track->target_text) {
		screenAnimationTrack_writeText(track);
		return;
	}
	if (track->source_int && track->values_by_index) {
		float value = track->values_by_index[*track->source_int];
		if (track->target)    *track->target    = value;
		if (track->target_u8) *track->target_u8 = (uint8_t)value;
		return;
	}
	if (track->target_bool) {
		/* Mirror of the forward window: a step (duration 0) holds to_bool
		   until reverse_delay, a window during
		   [reverse_delay, reverse_delay + duration). */
		bool in_window;
		if (track->duration <= 0.0f)
			in_window = (time < reverse_delay);
		else
			in_window = (time >= reverse_delay) && (time < reverse_delay + track->duration);
		*track->target_bool = in_window ? track->to_bool : track->from_bool;
		return;
	}
	if (!track->target && !track->target_u8) return;

	/* Before reverse_delay the track is pending and writes nothing, the prime
	   left the end state; after it, lerp toward `from` over `duration`.
	   reverse_delay is 0 for the track with the largest forward delay. */
	float local = time - reverse_delay;
	if (local < 0.0f) return;

	float t;
	if (track->duration > 0.0f) {
		t = local / track->duration;
		if (t > 1.0f) t = 1.0f;
	} else {
		t = 1.0f;
	}

	float eased = ease_function[track->easing](t);
	float value = lerpf(track->to, track->from, eased);

	if (track->target)    *track->target    = value;
	if (track->target_u8) *track->target_u8 = (uint8_t)value;
}

static void screenAnimationPlayer_applyFrame(ScreenAnimationPlayer *animation_player, float time)
{
	const ScreenAnimation *animation = animation_player->animation;
	if (animation_player->is_reversed) {
		float max_delay = screenAnimation_maxDelay(animation);
		for (int i = 0; i < animation->track_count; i++) {
			const ScreenAnimationTrack *track = &animation->track[i];
			float reverse_delay = max_delay - track->delay;
			screenAnimationPlayer_applyTrackReversed(track, time, reverse_delay);
		}
	} else {
		for (int i = 0; i < animation->track_count; i++)
			screenAnimationPlayer_applyTrack(&animation->track[i], time);
	}
}

/* Leaves every lerp target at its start value so pending tracks can stay
   silent. Forward it walks the array backwards, so the chronologically first
   track over a target wins; reversed it walks forwards, since the reverse
   starts from the end state. Reactive tracks (text, lookup) and bools write
   every frame and need no priming. */
static void screenAnimationPlayer_prime(ScreenAnimationPlayer *animation_player)
{
	const ScreenAnimation *animation = animation_player->animation;

	for (int i = 0; i < animation->track_count; i++) {
		const ScreenAnimationTrack *track = animation_player->is_reversed
			? &animation->track[i]
			: &animation->track[animation->track_count - 1 - i];

		if (track->target_text || track->source_int || track->target_bool) continue;
		if (!track->target && !track->target_u8) continue;

		float value = animation_player->is_reversed ? track->to : track->from;
		if (track->target)    *track->target    = value;
		if (track->target_u8) *track->target_u8 = (uint8_t)value;
	}
}

static void screenAnimationPlayer_finish(ScreenAnimationPlayer *animation_player)
{
	animation_player->is_active = false;
	if (animation_player->on_finish) animation_player->on_finish(animation_player->on_finish_ctx);
}


void screenAnimation_apply(const ScreenAnimation *animation, float time)
{
	for (int i = 0; i < animation->track_count; i++)
		screenAnimationPlayer_applyTrack(&animation->track[i], time);
}


void screenAnimationPlayer_start(ScreenAnimationPlayer *animation_player, const ScreenAnimation *animation, ScreenAnimationPlayMode mode, bool is_reversed)
{
	animation_player->animation   = animation;
	animation_player->mode        = mode;
	animation_player->time        = 0.0f;
	animation_player->is_active   = true;
	animation_player->is_reversed = is_reversed;
	screenAnimationPlayer_prime(animation_player);
	screenAnimationPlayer_applyFrame(animation_player, 0.0f);
}

void screenAnimationPlayer_stop(ScreenAnimationPlayer *animation_player)
{
	animation_player->is_active = false;
}

void screenAnimationPlayer_reverse(ScreenAnimationPlayer *animation_player)
{
	if (!animation_player->animation) return;
	float total = screenAnimation_duration(animation_player->animation);
	animation_player->time        = total - animation_player->time;
	animation_player->is_reversed = !animation_player->is_reversed;
}

void screenAnimationPlayer_update(ScreenAnimationPlayer *animation_player, float dt)
{
	if (!animation_player->is_active || !animation_player->animation) return;

	animation_player->time += dt;
	float total = animation_player->is_reversed
		? screenAnimation_reverseDuration(animation_player->animation)
		: screenAnimation_duration(animation_player->animation);

	if (animation_player->time < total) {
		screenAnimationPlayer_applyFrame(animation_player, animation_player->time);
		return;
	}

	switch (animation_player->mode) {

		case SCREEN_ANIMATION_PLAY_ONCE:
			screenAnimationPlayer_applyFrame(animation_player, total);
			screenAnimationPlayer_finish(animation_player);
			break;

		case SCREEN_ANIMATION_PLAY_LOOP:
			while (animation_player->time >= total) animation_player->time -= total;
			screenAnimationPlayer_applyFrame(animation_player, animation_player->time);
			break;

		case SCREEN_ANIMATION_PLAY_PING_PONG:
			while (animation_player->time >= total) animation_player->time -= total;
			animation_player->is_reversed = !animation_player->is_reversed;
			screenAnimationPlayer_applyFrame(animation_player, animation_player->time);
			break;
	}
}
