#include <libdragon.h>

#include "time/time.h"
#include "graphics/font.h"
#include "graphics/sprites.h"
#include "graphics/shapes.h"
#include "render/render.h"
#include "screen/screen.h"
#include "screen/screen_animation.h"
#include "menu/menu.h"
#include "ui/main_menu_ui.h"


static ScreenAnimationPlayer main_menu_player;


static const rdpq_textparms_t h14_parms = { .char_spacing = 0.95f };
static const rdpq_textparms_t h20_parms = { .char_spacing = 0.7f  };
static const rdpq_textparms_t h40_parms = { .char_spacing = 0.0f  };


typedef enum {

	MAIN_MENU_BG,
	MAIN_MENU_TITLE_1,
	MAIN_MENU_TITLE_2,
	MAIN_MENU_PLAY,
	MAIN_MENU_OPTIONS,
	MAIN_MENU_CREDITS,
	MAIN_MENU_HINT_MOVE,
	MAIN_MENU_HINT_SELECT,
	MAIN_MENU_BTN_A,
	MAIN_MENU_D_UP,
	MAIN_MENU_D_DOWN,
	MAIN_MENU_GORILLA,

} MainMenuElement;


static ScreenAnimationTrack main_menu_track[];

static const ScreenAnimation main_menu_animation = {
	.track       = main_menu_track,
	.track_count = 3,
};

Screen main_menu_screen = {
	.section = {
		[0] = {
			.element = {
				[MAIN_MENU_BG]          = { .type = DRAW_RECTANGLE, .position = {   0.0f,   0.0f }, .scale = { 320.0f, 240.0f }, .rectangle = { SHAPE_FILL_GRADIENT, .gradient = { RGBA32(201, 121, 25, 255), RGBA32(223, 175, 117, 255), RGBA32(223, 175, 117, 255), RGBA32(201, 121, 25, 255) } } },
				[MAIN_MENU_TITLE_1]     = { .type = DRAW_TEXT,      .position = {  43.0f,  55.0f },                              .text      = { HEADLINER_40, MENU_STYLE_NORMAL, "not a",   &h40_parms } },
				[MAIN_MENU_TITLE_2]     = { .type = DRAW_TEXT,      .position = {  43.0f,  98.0f },                              .text      = { HEADLINER_40, MENU_STYLE_NORMAL, "GAME",    &h40_parms } },
				[MAIN_MENU_PLAY]        = { .type = DRAW_TEXT,      .position = {  45.0f, 137.0f },                              .text      = { HEADLINER_20, MENU_STYLE_NORMAL, "Play",    &h20_parms } },
				[MAIN_MENU_OPTIONS]     = { .type = DRAW_TEXT,      .position = {  45.0f, 162.0f },                              .text      = { HEADLINER_20, MENU_STYLE_NORMAL, "Options", &h20_parms } },
				[MAIN_MENU_CREDITS]     = { .type = DRAW_TEXT,      .position = {  45.0f, 187.0f },                              .text      = { HEADLINER_20, MENU_STYLE_NORMAL, "Credits", &h20_parms } },
				[MAIN_MENU_HINT_MOVE]   = { .type = DRAW_TEXT,      .position = {  65.0f, 225.0f },                              .text      = { HEADLINER_14, 0,                 "Move",    &h14_parms } },
				[MAIN_MENU_HINT_SELECT] = { .type = DRAW_TEXT,      .position = { 115.0f, 225.0f },                              .text      = { HEADLINER_14, 0,                 "Select",  &h14_parms } },
				[MAIN_MENU_BTN_A]       = { .type = DRAW_SPRITE,    .position = { 102.0f, 216.0f }, .scale = { 0.60f, 0.60f },   .sprite    = { SPRITE_BTN_A   } },
				[MAIN_MENU_D_UP]        = { .type = DRAW_SPRITE,    .position = {  43.0f, 217.0f }, .scale = { 0.48f, 0.48f },   .sprite    = { SPRITE_D_UP    } },
				[MAIN_MENU_D_DOWN]      = { .type = DRAW_SPRITE,    .position = {  53.0f, 217.0f }, .scale = { 0.48f, 0.48f },   .sprite    = { SPRITE_D_DOWN  } },
				[MAIN_MENU_GORILLA]     = { .type = DRAW_SPRITE,    .position = { 170.0f,   0.0f }, .scale = { 1.00f, 1.00f },   .sprite    = { SPRITE_GORILLA } },
			},
			.element_count = 12,
		},
	},
	.section_count = 1,
	.animation     = &main_menu_animation,
};

static const float main_menu_style_play[]    = { MENU_STYLE_SELECTED, MENU_STYLE_NORMAL,   MENU_STYLE_NORMAL   };
static const float main_menu_style_options[] = { MENU_STYLE_NORMAL,   MENU_STYLE_SELECTED, MENU_STYLE_NORMAL   };
static const float main_menu_style_credits[] = { MENU_STYLE_NORMAL,   MENU_STYLE_NORMAL,   MENU_STYLE_SELECTED };

static ScreenAnimationTrack main_menu_track[] = {

	{ .target_u8 = &main_menu_screen.section[0].element[MAIN_MENU_PLAY].text.style,    .source_int = NULL, .values_by_index = main_menu_style_play    },
	{ .target_u8 = &main_menu_screen.section[0].element[MAIN_MENU_OPTIONS].text.style, .source_int = NULL, .values_by_index = main_menu_style_options },
	{ .target_u8 = &main_menu_screen.section[0].element[MAIN_MENU_CREDITS].text.style, .source_int = NULL, .values_by_index = main_menu_style_credits },
};


static ScreenAnimationTrack main_menu_transition_track[] = {

	{ .target = &main_menu_screen.section[0].element[MAIN_MENU_TITLE_1].position.x, .from = -260.0f, .to =  43.0f, .delay = 0.00f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_CUBIC_OUT },
	{ .target = &main_menu_screen.section[0].element[MAIN_MENU_TITLE_2].position.x, .from = -260.0f, .to =  43.0f, .delay = 0.05f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_CUBIC_OUT },

	{ .target = &main_menu_screen.section[0].element[MAIN_MENU_PLAY].position.x,    .from = 400.0f, .to = 45.0f, .delay = 0.15f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_CUBIC_OUT },
	{ .target = &main_menu_screen.section[0].element[MAIN_MENU_OPTIONS].position.x, .from = 400.0f, .to = 45.0f, .delay = 0.21f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_CUBIC_OUT },
	{ .target = &main_menu_screen.section[0].element[MAIN_MENU_CREDITS].position.x, .from = 400.0f, .to = 45.0f, .delay = 0.27f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_CUBIC_OUT },

	{ .target = &main_menu_screen.section[0].element[MAIN_MENU_GORILLA].position.x, .from = 400.0f, .to = 170.0f, .delay = 0.00f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_CUBIC_OUT },

	{ .target_u8 = &main_menu_screen.section[0].element[MAIN_MENU_BG].rectangle.gradient[0].r, .from =  12.0f, .to = 201.0f, .delay = 0.00f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_CUBIC_OUT },
	{ .target_u8 = &main_menu_screen.section[0].element[MAIN_MENU_BG].rectangle.gradient[0].g, .from =  14.0f, .to = 121.0f, .delay = 0.00f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_CUBIC_OUT },
	{ .target_u8 = &main_menu_screen.section[0].element[MAIN_MENU_BG].rectangle.gradient[0].b, .from =  28.0f, .to =  25.0f, .delay = 0.00f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_CUBIC_OUT },

	{ .target_u8 = &main_menu_screen.section[0].element[MAIN_MENU_BG].rectangle.gradient[1].r, .from =  40.0f, .to = 223.0f, .delay = 0.00f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_CUBIC_OUT },
	{ .target_u8 = &main_menu_screen.section[0].element[MAIN_MENU_BG].rectangle.gradient[1].g, .from =  30.0f, .to = 175.0f, .delay = 0.00f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_CUBIC_OUT },
	{ .target_u8 = &main_menu_screen.section[0].element[MAIN_MENU_BG].rectangle.gradient[1].b, .from =  70.0f, .to = 117.0f, .delay = 0.00f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_CUBIC_OUT },

	{ .target_u8 = &main_menu_screen.section[0].element[MAIN_MENU_BG].rectangle.gradient[2].r, .from =  40.0f, .to = 223.0f, .delay = 0.00f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_CUBIC_OUT },
	{ .target_u8 = &main_menu_screen.section[0].element[MAIN_MENU_BG].rectangle.gradient[2].g, .from =  30.0f, .to = 175.0f, .delay = 0.00f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_CUBIC_OUT },
	{ .target_u8 = &main_menu_screen.section[0].element[MAIN_MENU_BG].rectangle.gradient[2].b, .from =  70.0f, .to = 117.0f, .delay = 0.00f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_CUBIC_OUT },

	{ .target_u8 = &main_menu_screen.section[0].element[MAIN_MENU_BG].rectangle.gradient[3].r, .from =  12.0f, .to = 201.0f, .delay = 0.00f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_CUBIC_OUT },
	{ .target_u8 = &main_menu_screen.section[0].element[MAIN_MENU_BG].rectangle.gradient[3].g, .from =  14.0f, .to = 121.0f, .delay = 0.00f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_CUBIC_OUT },
	{ .target_u8 = &main_menu_screen.section[0].element[MAIN_MENU_BG].rectangle.gradient[3].b, .from =  28.0f, .to =  25.0f, .delay = 0.00f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_CUBIC_OUT },
};

static const ScreenAnimation main_menu_transition_animation = {
	.track       = main_menu_transition_track,
	.track_count = 18,
};


void main_menu_ui_init(void)
{
	const int8_t *index = menuStack_getIndexPtr();
	main_menu_track[0].source_int = index;
	main_menu_track[1].source_int = index;
	main_menu_track[2].source_int = index;
}


void main_menu_update(void)
{
	if (main_menu_player.is_active) {
		screenAnimationPlayer_update(&main_menu_player, time_get()->delta);
	}
	screenAnimation_apply(&main_menu_animation, 0.0f);
}


void main_menu_startEnter(void)
{
	screenAnimationPlayer_start(&main_menu_player, &main_menu_transition_animation, SCREEN_ANIMATION_PLAY_ONCE, false);
	main_menu_player.on_finish     = NULL;
	main_menu_player.on_finish_ctx = NULL;
}


void main_menu_startExit(void (*on_finish)(void *ctx), void *ctx)
{
	screenAnimationPlayer_start(&main_menu_player, &main_menu_transition_animation, SCREEN_ANIMATION_PLAY_ONCE, true);
	main_menu_player.on_finish     = on_finish;
	main_menu_player.on_finish_ctx = ctx;
}


bool main_menu_isTransitioning(void)
{
	return main_menu_player.is_active;
}
