#ifndef UI_ANIMATION_H
#define UI_ANIMATION_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Scene2D Scene2D;


typedef enum {

	UI_EASING_LINEAR,

	UI_EASING_QUAD_IN,
	UI_EASING_QUAD_OUT,
	UI_EASING_QUAD_IN_OUT,

	UI_EASING_CUBIC_IN,
	UI_EASING_CUBIC_OUT,
	UI_EASING_CUBIC_IN_OUT,

	UI_EASING_EXPO_IN,
	UI_EASING_EXPO_OUT,
	UI_EASING_EXPO_IN_OUT,

	UI_EASING_COUNT,

} UIEasing;


typedef enum {

	UI_ANIMATION_PLAY_ONCE,
	UI_ANIMATION_PLAY_LOOP,
	UI_ANIMATION_PLAY_PING_PONG,

} UIAnimationPlayMode;


/* What of an element a track writes. The name carries the type: the first
   block is written as a float, the next as a byte, hidden as a flag. */
typedef enum {

	UI_FIELD_POSITION_X,
	UI_FIELD_POSITION_Y,
	UI_FIELD_SCALE_X,
	UI_FIELD_SCALE_Y,
	UI_FIELD_ROTATION,

	UI_FIELD_TRANSPARENCY,
	UI_FIELD_TEXT_STYLE,
	UI_FIELD_SPRITE_FRAME,

	UI_FIELD_COLOR_R,
	UI_FIELD_COLOR_G,
	UI_FIELD_COLOR_B,
	UI_FIELD_COLOR_A,

	/* Which corner comes from the track's own corner field. */
	UI_FIELD_GRADIENT_R,
	UI_FIELD_GRADIENT_G,
	UI_FIELD_GRADIENT_B,
	UI_FIELD_GRADIENT_A,

	UI_FIELD_HIDDEN,

} UIField;


/* A live source drives the value instead of time: the track names it and
   the engine reads it fresh on every apply. */
typedef enum {

	UI_SOURCE_NONE,
	UI_SOURCE_MENU_INDEX,   /* the menu stack cursor */

} UISource;


typedef struct {

	/* Which element of the live scene, and what of it. */
	uint8_t  layer;
	uint8_t  element;
	uint8_t  field;      /* UIField */
	uint8_t  corner;     /* gradient corner, 0..3 */

	float    from;
	float    to;
	bool     from_bool;
	bool     to_bool;
	float    delay;
	float    duration;
	UIEasing easing;

	uint8_t      source;   /* UISource */
	const float *values_by_index;

} UIAnimationTrack;


typedef struct UIAnimation {

	const UIAnimationTrack *track;
	uint8_t                 track_count;

} UIAnimation;


typedef struct {

	const UIAnimation  *animation;
	UIAnimationPlayMode mode;
	float               time;
	bool                is_active;
	bool                is_reversed;

} UIAnimationPlayer;


void uiAnimation_apply(Scene2D *scene2d, const UIAnimation *animation, float time);

void uiAnimationPlayer_start(UIAnimationPlayer *player, Scene2D *scene2d, const UIAnimation *animation, UIAnimationPlayMode mode, bool is_reversed);
void uiAnimationPlayer_stop(UIAnimationPlayer *player);
void uiAnimationPlayer_update(UIAnimationPlayer *player, Scene2D *scene2d, float dt);

#endif
