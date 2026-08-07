#include <libdragon.h>

#include "time/time.h"
#include "graphics/font.h"
#include "graphics/sprites.h"
#include "graphics/shapes.h"
#include "render/render.h"
#include "screen/screen.h"
#include "screen/screen_animation.h"
#include "menu/menu.h"
#include "ui/pause_ui.h"


static const rdpq_textparms_t h14_parms = { .char_spacing = 0.95f };
static const rdpq_textparms_t h20_parms = { .char_spacing = 0.7f  };


typedef enum {

	PAUSE_BG,
	PAUSE_CONTINUE,
	PAUSE_QUIT,
	PAUSE_HINT_MOVE,
	PAUSE_HINT_SELECT,
	PAUSE_HINT_BACK,
	PAUSE_D_UP,
	PAUSE_D_DOWN,
	PAUSE_BTN_A,
	PAUSE_BTN_B,

} PauseElement;


static ScreenAnimationTrack pause_transition_track[];
static ScreenAnimationTrack pause_cursor_track[];

static const ScreenAnimation pause_transition_animation = {
	.track       = pause_transition_track,
	.track_count = 13,
};

static const ScreenAnimation pause_cursor_animation = {
	.track       = pause_cursor_track,
	.track_count = 2,
};

Screen pause_screen = {
	.section = {
		[0] = {
			.element = {
				[PAUSE_BG]          = { .type = DRAW_RECTANGLE, .position = {   0.0f,   0.0f }, .scale = { 320.0f, 240.0f }, .rectangle = { SHAPE_FILL_GRADIENT } },
				[PAUSE_CONTINUE]    = { .type = DRAW_TEXT,      .position = { 320.0f,  50.0f },                              .text      = { HEADLINER_20, MENU_STYLE_NORMAL, "Continue", &h20_parms } },
				[PAUSE_QUIT]        = { .type = DRAW_TEXT,      .position = { 320.0f,  80.0f },                              .text      = { HEADLINER_20, MENU_STYLE_NORMAL, "Quit",     &h20_parms } },
				[PAUSE_HINT_MOVE]   = { .type = DRAW_TEXT,      .position = { 347.0f, 196.0f },                              .text      = { HEADLINER_14, 0,                 "Move",     &h14_parms } },
				[PAUSE_HINT_SELECT] = { .type = DRAW_TEXT,      .position = { 338.0f, 211.0f },                              .text      = { HEADLINER_14, 0,                 "Select",   &h14_parms } },
				[PAUSE_HINT_BACK]   = { .type = DRAW_TEXT,      .position = { 338.0f, 226.0f },                              .text      = { HEADLINER_14, 0,                 "Back",     &h14_parms } },
				[PAUSE_D_UP]        = { .type = DRAW_SPRITE,    .position = { 320.0f, 186.0f }, .scale = { 0.48f, 0.48f },   .sprite    = { SPRITE_D_UP   } },
				[PAUSE_D_DOWN]      = { .type = DRAW_SPRITE,    .position = { 330.0f, 187.0f }, .scale = { 0.48f, 0.48f },   .sprite    = { SPRITE_D_DOWN } },
				[PAUSE_BTN_A]       = { .type = DRAW_SPRITE,    .position = { 320.0f, 201.0f }, .scale = { 0.60f, 0.60f },   .sprite    = { SPRITE_BTN_A  } },
				[PAUSE_BTN_B]       = { .type = DRAW_SPRITE,    .position = { 320.0f, 216.0f }, .scale = { 0.60f, 0.60f },   .sprite    = { SPRITE_BTN_B  } },
			},
			.element_count = 10,
		},
	},
	.section_count = 1,
	.animation     = &pause_transition_animation,
};

static const float pause_style_continue[] = { MENU_STYLE_SELECTED, MENU_STYLE_NORMAL   };
static const float pause_style_quit[]     = { MENU_STYLE_NORMAL,   MENU_STYLE_SELECTED };

static ScreenAnimationTrack pause_transition_track[] = {

	{ .target_u8 = &pause_screen.section[0].element[PAUSE_BG].rectangle.gradient[0].a, .from = 0.0f, .to =  76.5f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_LINEAR },
	{ .target_u8 = &pause_screen.section[0].element[PAUSE_BG].rectangle.gradient[1].a, .from = 0.0f, .to = 255.0f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_LINEAR },
	{ .target_u8 = &pause_screen.section[0].element[PAUSE_BG].rectangle.gradient[2].a, .from = 0.0f, .to = 255.0f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_LINEAR },
	{ .target_u8 = &pause_screen.section[0].element[PAUSE_BG].rectangle.gradient[3].a, .from = 0.0f, .to =  76.5f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_LINEAR },

	{ .target = &pause_screen.section[0].element[PAUSE_CONTINUE].position.x,    .from = 320.0f, .to = 238.0f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_LINEAR },
	{ .target = &pause_screen.section[0].element[PAUSE_QUIT].position.x,        .from = 320.0f, .to = 263.0f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_LINEAR },

	{ .target = &pause_screen.section[0].element[PAUSE_HINT_MOVE].position.x,   .from = 347.0f, .to = 262.0f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_LINEAR },
	{ .target = &pause_screen.section[0].element[PAUSE_HINT_SELECT].position.x, .from = 338.0f, .to = 262.0f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_LINEAR },
	{ .target = &pause_screen.section[0].element[PAUSE_HINT_BACK].position.x,   .from = 338.0f, .to = 262.0f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_LINEAR },

	{ .target = &pause_screen.section[0].element[PAUSE_D_UP].position.x,        .from = 320.0f, .to = 235.0f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_LINEAR },
	{ .target = &pause_screen.section[0].element[PAUSE_D_DOWN].position.x,      .from = 330.0f, .to = 245.0f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_LINEAR },
	{ .target = &pause_screen.section[0].element[PAUSE_BTN_A].position.x,       .from = 320.0f, .to = 244.0f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_LINEAR },
	{ .target = &pause_screen.section[0].element[PAUSE_BTN_B].position.x,       .from = 320.0f, .to = 244.0f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_LINEAR },
};

static ScreenAnimationTrack pause_cursor_track[] = {

	{ .target_u8 = &pause_screen.section[0].element[PAUSE_CONTINUE].text.style, .source_int = NULL, .values_by_index = pause_style_continue },
	{ .target_u8 = &pause_screen.section[0].element[PAUSE_QUIT].text.style,     .source_int = NULL, .values_by_index = pause_style_quit     },
};


static ScreenAnimationPlayer pause_animation_player;

void pause_ui_init(void)
{
	const int8_t *index = menuStack_getIndexPtr();
	pause_cursor_track[0].source_int = index;
	pause_cursor_track[1].source_int = index;
}

void pause_update(void)
{
	if (pause_animation_player.is_active)
		screenAnimationPlayer_update(&pause_animation_player, time_get()->delta);
	screenAnimation_apply(&pause_cursor_animation, 0.0f);
}

void pause_startEnter(void)
{
	screenAnimationPlayer_start(&pause_animation_player, &pause_transition_animation, SCREEN_ANIMATION_PLAY_ONCE, false);
	pause_animation_player.on_finish     = NULL;
	pause_animation_player.on_finish_ctx = NULL;
}

void pause_startExit(void (*on_finish)(void *ctx), void *ctx)
{
	screenAnimationPlayer_start(&pause_animation_player, &pause_transition_animation, SCREEN_ANIMATION_PLAY_ONCE, true);
	pause_animation_player.on_finish     = on_finish;
	pause_animation_player.on_finish_ctx = ctx;
}

bool pause_isTransitioning(void)
{
	return pause_animation_player.is_active;
}
