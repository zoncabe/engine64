#ifndef SCREEN_ANIMATION_H
#define SCREEN_ANIMATION_H

#include <stdbool.h>
#include <stdint.h>


typedef enum {

	SCREEN_ANIMATION_EASING_LINEAR,

	SCREEN_ANIMATION_EASING_QUAD_IN,
	SCREEN_ANIMATION_EASING_QUAD_OUT,
	SCREEN_ANIMATION_EASING_QUAD_IN_OUT,

	SCREEN_ANIMATION_EASING_CUBIC_IN,
	SCREEN_ANIMATION_EASING_CUBIC_OUT,
	SCREEN_ANIMATION_EASING_CUBIC_IN_OUT,

	SCREEN_ANIMATION_EASING_EXPO_IN,
	SCREEN_ANIMATION_EASING_EXPO_OUT,
	SCREEN_ANIMATION_EASING_EXPO_IN_OUT,

	SCREEN_ANIMATION_EASING_COUNT,

} ScreenAnimationEasing;


typedef enum {

	SCREEN_ANIMATION_PLAY_ONCE,
	SCREEN_ANIMATION_PLAY_LOOP,
	SCREEN_ANIMATION_PLAY_PING_PONG,

} ScreenAnimationPlayMode;


typedef struct {

	float                *target;
	uint8_t              *target_u8;
	bool                 *target_bool;
	char                 *target_text;

	float                 from;
	float                 to;
	bool                  from_bool;
	bool                  to_bool;
	float                 delay;
	float                 duration;
	ScreenAnimationEasing easing;

	const int8_t         *source_int;
	const float          *values_by_index;

	uint16_t              target_size;
	const char           *format;
	const uint8_t        *source_u8;
	const bool           *source_bool;
	const char *const    *source_label;

} ScreenAnimationTrack;


typedef struct {

	const ScreenAnimationTrack *track;
	uint8_t                     track_count;

} ScreenAnimation;


typedef struct {

	const ScreenAnimation  *animation;
	ScreenAnimationPlayMode mode;
	float                   time;
	bool                    is_active;
	bool                    is_reversed;

	void (*on_finish)(void *ctx);
	void  *on_finish_ctx;

} ScreenAnimationPlayer;


void screenAnimation_apply(const ScreenAnimation *animation, float time);

void screenAnimationPlayer_start(ScreenAnimationPlayer *animation_player, const ScreenAnimation *animation, ScreenAnimationPlayMode mode, bool is_reversed);
void screenAnimationPlayer_stop(ScreenAnimationPlayer *animation_player);
void screenAnimationPlayer_reverse(ScreenAnimationPlayer *animation_player);
void screenAnimationPlayer_update(ScreenAnimationPlayer *animation_player, float dt);

#endif
