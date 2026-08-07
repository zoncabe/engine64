#include <libdragon.h>

#include "graphics/sprites.h"
#include "graphics/shapes.h"
#include "render/render.h"
#include "screen/screen.h"
#include "screen/screen_animation.h"
#include "cutscene/intro.h"


typedef enum {

	INTRO_N64LOGO,
	INTRO_LIBDRAGON,
	INTRO_TINY3D,
	INTRO_ENGINE64,
	INTRO_RECT,

} IntroElement;


static const rdpq_textparms_t intro_title_parms = {
	.width  = 320,
	.height = 240,
	.align  = ALIGN_CENTER,
	.valign = VALIGN_CENTER,
};

static const ScreenAnimationTrack intro_track[];

static const ScreenAnimation intro_animation = {
	.track       = intro_track,
	.track_count = 12,
};

Screen intro_screen = {
	.section = {
		[0] = {
			.element = {
				[INTRO_N64LOGO]   = { .type = DRAW_SPRITE,    .position = { 0.0f, 0.0f }, .scale = { 1.0f, 1.0f },     .sprite    = { SPRITE_N64LOGO },   .is_hidden = true },
				[INTRO_LIBDRAGON] = { .type = DRAW_SPRITE,    .position = { 0.0f, 0.0f }, .scale = { 1.0f, 1.0f },     .sprite    = { SPRITE_LIBDRAGON }, .is_hidden = true },
				[INTRO_TINY3D]    = { .type = DRAW_SPRITE,    .position = { 0.0f, 0.0f }, .scale = { 1.0f, 1.0f },     .sprite    = { SPRITE_TINY3D },    .is_hidden = true },
				[INTRO_ENGINE64]  = { .type = DRAW_TEXT,      .position = { 0.0f, 0.0f },                              .text      = { HEADLINER_60, TEXT_STYLE_RED, "engine 64", &intro_title_parms }, .is_hidden = true },
				[INTRO_RECT]      = { .type = DRAW_RECTANGLE, .position = { 0.0f, 0.0f }, .scale = { 320.0f, 240.0f }, .rectangle = { .fill = SHAPE_FILL_SOLID, .color = RGBA32(0, 0, 0, 255) } },
			},
			.element_count = 5,
		},
	},
	.section_count = 1,
	.animation     = &intro_animation,
};

ScreenAnimationPlayer intro_animation_player;


static const ScreenAnimationTrack intro_track[] = {

	{ .target_bool = &intro_screen.section[0].element[INTRO_N64LOGO].is_hidden,   .from_bool = true, .to_bool = false, .delay = 0.75f, .duration = 2.25f },
	{ .target_bool = &intro_screen.section[0].element[INTRO_LIBDRAGON].is_hidden, .from_bool = true, .to_bool = false, .delay = 3.00f, .duration = 2.00f },
	{ .target_bool = &intro_screen.section[0].element[INTRO_TINY3D].is_hidden,    .from_bool = true, .to_bool = false, .delay = 5.00f, .duration = 2.00f },
	{ .target_bool = &intro_screen.section[0].element[INTRO_ENGINE64].is_hidden,  .from_bool = true, .to_bool = false, .delay = 7.00f, .duration = 3.00f },

	{ .target_u8 = &intro_screen.section[0].element[INTRO_RECT].rectangle.color.a, .from = 255.0f, .to = 0.0f,   .delay = 0.75f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_LINEAR },
	{ .target_u8 = &intro_screen.section[0].element[INTRO_RECT].rectangle.color.a, .from = 0.0f,   .to = 255.0f, .delay = 2.75f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_LINEAR },

	{ .target_u8 = &intro_screen.section[0].element[INTRO_RECT].rectangle.color.a, .from = 255.0f, .to = 0.0f,   .delay = 3.00f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_LINEAR },
	{ .target_u8 = &intro_screen.section[0].element[INTRO_RECT].rectangle.color.a, .from = 0.0f,   .to = 255.0f, .delay = 4.75f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_LINEAR },

	{ .target_u8 = &intro_screen.section[0].element[INTRO_RECT].rectangle.color.a, .from = 255.0f, .to = 0.0f,   .delay = 5.00f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_LINEAR },
	{ .target_u8 = &intro_screen.section[0].element[INTRO_RECT].rectangle.color.a, .from = 0.0f,   .to = 255.0f, .delay = 6.75f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_LINEAR },

	{ .target_u8 = &intro_screen.section[0].element[INTRO_RECT].rectangle.color.a, .from = 255.0f, .to = 0.0f,   .delay = 7.00f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_LINEAR },
	{ .target_u8 = &intro_screen.section[0].element[INTRO_RECT].rectangle.color.a, .from = 0.0f,   .to = 255.0f, .delay = 9.50f, .duration = 0.50f, .easing = SCREEN_ANIMATION_EASING_LINEAR },
};


void intro_init(void)
{
	screenAnimationPlayer_start(&intro_animation_player, &intro_animation, SCREEN_ANIMATION_PLAY_ONCE, false);
}
