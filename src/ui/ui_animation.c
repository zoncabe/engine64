/*
	Track animation over the elements of a 2D scene. A track names the
	element it writes and which of its fields, so the animation is data: it
	holds no addresses and survives the scene being loaded again.
*/
#include <stddef.h>

#include "ui/ui_animation.h"
#include "scene2d/scene2d.h"
#include "physics/math/math_common.h"
#include "menu/menu.h"


/* The three kinds a field can be written as; one of them is set. */
typedef struct {

	float   *as_float;
	uint8_t *as_u8;
	bool    *as_bool;

} FieldRef;

static FieldRef uiAnimation_field(Scene2D *scene2d, const UIAnimationTrack *track)
{
	DrawElement *e = scene2d_getElement(scene2d, track->layer, track->element);

	switch (track->field) {

		case UI_FIELD_POSITION_X:   return (FieldRef){ .as_float = &e->position.x };
		case UI_FIELD_POSITION_Y:   return (FieldRef){ .as_float = &e->position.y };
		case UI_FIELD_SCALE_X:      return (FieldRef){ .as_float = &e->scale.x };
		case UI_FIELD_SCALE_Y:      return (FieldRef){ .as_float = &e->scale.y };
		case UI_FIELD_ROTATION:     return (FieldRef){ .as_float = &e->rotation };

		case UI_FIELD_TRANSPARENCY: return (FieldRef){ .as_u8 = &e->transparency };
		case UI_FIELD_TEXT_STYLE:   return (FieldRef){ .as_u8 = &e->text.style };
		case UI_FIELD_SPRITE_FRAME: return (FieldRef){ .as_u8 = &e->sprite.frame };

		case UI_FIELD_COLOR_R:      return (FieldRef){ .as_u8 = &e->rectangle.color.r };
		case UI_FIELD_COLOR_G:      return (FieldRef){ .as_u8 = &e->rectangle.color.g };
		case UI_FIELD_COLOR_B:      return (FieldRef){ .as_u8 = &e->rectangle.color.b };
		case UI_FIELD_COLOR_A:      return (FieldRef){ .as_u8 = &e->rectangle.color.a };

		case UI_FIELD_GRADIENT_R:   return (FieldRef){ .as_u8 = &e->rectangle.gradient[track->corner].r };
		case UI_FIELD_GRADIENT_G:   return (FieldRef){ .as_u8 = &e->rectangle.gradient[track->corner].g };
		case UI_FIELD_GRADIENT_B:   return (FieldRef){ .as_u8 = &e->rectangle.gradient[track->corner].b };
		case UI_FIELD_GRADIENT_A:   return (FieldRef){ .as_u8 = &e->rectangle.gradient[track->corner].a };

		case UI_FIELD_HIDDEN:       return (FieldRef){ .as_bool = &e->is_hidden };
	}

	return (FieldRef){0};
}

static void uiAnimation_write(const FieldRef *ref, float value)
{
	if (ref->as_float) *ref->as_float = value;
	if (ref->as_u8)    *ref->as_u8    = (uint8_t)value;
}

static bool uiAnimation_sourceIndex(const UIAnimationTrack *track, int8_t *index)
{
	switch (track->source) {
		case UI_SOURCE_MENU_INDEX: *index = menuStack_getIndex(); return true;
	}
	return false;
}


typedef float (*EaseFunction)(float t);

static const EaseFunction ease_function[UI_EASING_COUNT] = {
	[UI_EASING_LINEAR]       = ease_linear,
	[UI_EASING_QUAD_IN]      = ease_quad_in,
	[UI_EASING_QUAD_OUT]     = ease_quad_out,
	[UI_EASING_QUAD_IN_OUT]  = ease_quad_in_out,
	[UI_EASING_CUBIC_IN]     = ease_cubic_in,
	[UI_EASING_CUBIC_OUT]    = ease_cubic_out,
	[UI_EASING_CUBIC_IN_OUT] = ease_cubic_in_out,
	[UI_EASING_EXPO_IN]      = ease_expo_in,
	[UI_EASING_EXPO_OUT]     = ease_expo_out,
	[UI_EASING_EXPO_IN_OUT]  = ease_expo_in_out,
};


static float uiAnimation_duration(const UIAnimation *animation)
{
	float max = 0.0f;
	for (int i = 0; i < animation->track_count; i++) {
		float end = animation->track[i].delay + animation->track[i].duration;
		if (end > max) max = end;
	}
	return max;
}

/* Backwards there is no stagger: every track leaves at once, from frame 0,
   so the way out is over as soon as the slowest one is. Waiting out the
   delays again would leave the screen half gone for twice as long, which is
   what a state switch hides behind. */
static float uiAnimation_reverseDuration(const UIAnimation *animation)
{
	float max = 0.0f;
	for (int i = 0; i < animation->track_count; i++) {
		float d = animation->track[i].duration;
		if (d > max) max = d;
	}
	return max;
}

/* How far along a track that already started is, 1 for a step. */
static float uiAnimation_time(const UIAnimationTrack *track, float local)
{
	if (track->duration <= 0.0f) return 1.0f;

	float t = local / track->duration;
	return (t > 1.0f) ? 1.0f : t;
}

static float uiAnimation_progress(const UIAnimationTrack *track, float local)
{
	return ease_function[track->easing](uiAnimation_time(track, local));
}

/* Backwards is the same motion seen in reverse, so the curve mirrors too:
   what eases in on the way in eases out on the way out. Re-easing forward
   instead would hold the element still and then snap it. */
static float uiAnimation_progressReversed(const UIAnimationTrack *track, float local)
{
	float t = uiAnimation_time(track, local);
	return 1.0f - ease_function[track->easing](1.0f - t);
}

static void uiAnimation_applyTrack(Scene2D *scene2d, const UIAnimationTrack *track, float time)
{
	FieldRef ref = uiAnimation_field(scene2d, track);

	int8_t source_index;
	if (track->values_by_index && uiAnimation_sourceIndex(track, &source_index)) {
		uiAnimation_write(&ref, track->values_by_index[source_index]);
		return;
	}

	if (ref.as_bool) {
		float end = track->delay + track->duration;
		bool  in_window = (time >= track->delay) && (track->duration <= 0.0f || time < end);
		*ref.as_bool = in_window ? track->to_bool : track->from_bool;
		return;
	}

	/* A pending track writes nothing: the prime left every target on its
	   start value, so chained fades over one target hold what the last
	   expired track left. */
	float local = time - track->delay;
	if (local < 0.0f) return;

	uiAnimation_write(&ref, lerpf(track->from, track->to, uiAnimation_progress(track, local)));
}

static void uiAnimation_applyTrackReversed(Scene2D *scene2d, const UIAnimationTrack *track, float time)
{
	FieldRef ref = uiAnimation_field(scene2d, track);

	int8_t source_index;
	if (track->values_by_index && uiAnimation_sourceIndex(track, &source_index)) {
		uiAnimation_write(&ref, track->values_by_index[source_index]);
		return;
	}

	if (ref.as_bool) {
		/* The element stays the way the animation left it for as long as it
		   takes to leave; a step holds it to the end. */
		bool in_window = (track->duration <= 0.0f) || (time < track->duration);
		*ref.as_bool = in_window ? track->to_bool : track->from_bool;
		return;
	}

	uiAnimation_write(&ref, lerpf(track->to, track->from, uiAnimation_progressReversed(track, time)));
}

static void uiAnimationPlayer_applyFrame(UIAnimationPlayer *player, Scene2D *scene2d, float time)
{
	const UIAnimation *animation = player->animation;

	if (player->is_reversed) {
		for (int i = 0; i < animation->track_count; i++)
			uiAnimation_applyTrackReversed(scene2d, &animation->track[i], time);
		return;
	}

	for (int i = 0; i < animation->track_count; i++)
		uiAnimation_applyTrack(scene2d, &animation->track[i], time);
}

/* Leaves every lerp target on its start value so pending tracks can stay
   silent. Forward it walks the array backwards, so the chronologically first
   track over a target wins; reversed it walks forwards, since the reverse
   starts from the end state. Live lookups and flags write every frame and
   need no priming. */
static void uiAnimationPlayer_prime(UIAnimationPlayer *player, Scene2D *scene2d)
{
	const UIAnimation *animation = player->animation;

	for (int i = 0; i < animation->track_count; i++) {
		const UIAnimationTrack *track = player->is_reversed
			? &animation->track[i]
			: &animation->track[animation->track_count - 1 - i];

		if (track->values_by_index) continue;

		FieldRef ref = uiAnimation_field(scene2d, track);
		if (ref.as_bool) continue;

		uiAnimation_write(&ref, player->is_reversed ? track->to : track->from);
	}
}


void uiAnimation_apply(Scene2D *scene2d, const UIAnimation *animation, float time)
{
	for (int i = 0; i < animation->track_count; i++)
		uiAnimation_applyTrack(scene2d, &animation->track[i], time);
}


void uiAnimationPlayer_start(UIAnimationPlayer *player, Scene2D *scene2d, const UIAnimation *animation, UIAnimationPlayMode mode, bool is_reversed)
{
	player->animation   = animation;
	player->mode        = mode;
	player->time        = 0.0f;
	player->is_active   = true;
	player->is_reversed = is_reversed;

	uiAnimationPlayer_prime(player, scene2d);
	uiAnimationPlayer_applyFrame(player, scene2d, 0.0f);
}

void uiAnimationPlayer_stop(UIAnimationPlayer *player)
{
	player->is_active = false;
}

void uiAnimationPlayer_update(UIAnimationPlayer *player, Scene2D *scene2d, float dt)
{
	if (!player->is_active || !player->animation) return;

	player->time += dt;
	float total = player->is_reversed
		? uiAnimation_reverseDuration(player->animation)
		: uiAnimation_duration(player->animation);

	if (player->time < total) {
		uiAnimationPlayer_applyFrame(player, scene2d, player->time);
		return;
	}

	switch (player->mode) {

		case UI_ANIMATION_PLAY_ONCE:
			uiAnimationPlayer_applyFrame(player, scene2d, total);
			player->is_active = false;
			break;

		case UI_ANIMATION_PLAY_LOOP:
			while (player->time >= total) player->time -= total;
			uiAnimationPlayer_applyFrame(player, scene2d, player->time);
			break;

		case UI_ANIMATION_PLAY_PING_PONG:
			while (player->time >= total) player->time -= total;
			player->is_reversed = !player->is_reversed;
			uiAnimationPlayer_applyFrame(player, scene2d, player->time);
			break;
	}
}
