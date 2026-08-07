#include <libdragon.h>

#include "time/time.h"
#include "graphics/shapes.h"
#include "render/render.h"
#include "screen/screen.h"
#include "screen/screen_animation.h"
#include "ui/gameplay_ui.h"


typedef enum {

	GAMEPLAY_FADE,

} GameplayElement;


static ScreenAnimationTrack gameplay_track[];

static const ScreenAnimation gameplay_animation = {
	.track       = gameplay_track,
	.track_count = 2,
};

Screen gameplay_screen = {
	.section = {
		[0] = {
			.element = {
				[GAMEPLAY_FADE] = { .type = DRAW_RECTANGLE, .position = { 0.0f, 0.0f }, .scale = { 320.0f, 240.0f }, .rectangle = { SHAPE_FILL_SOLID, .color = RGBA32(0, 0, 0, 255) } },
			},
			.element_count = 1,
		},
	},
	.section_count = 1,
	.animation     = &gameplay_animation,
};

static ScreenAnimationTrack gameplay_track[] = {

	{ .target_u8 = &gameplay_screen.section[0].element[GAMEPLAY_FADE].rectangle.color.a, .from = 255.0f, .to = 0.0f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_LINEAR },

	/* Visible solo mientras dura el fade: terminado, el rect no manda ningun
	   comando al RDP. En reverse la ventana se espeja y lo destapa sola. */
	{ .target_bool = &gameplay_screen.section[0].element[GAMEPLAY_FADE].is_hidden, .from_bool = true, .to_bool = false, .duration = 0.25f },
};


static ScreenAnimationPlayer gameplay_player;

void gameplay_ui_init(void) {}

void gameplay_update(void)
{
	if (gameplay_player.is_active)
		screenAnimationPlayer_update(&gameplay_player, time_get()->delta);
}

void gameplay_startEnter(void)
{
	screenAnimationPlayer_start(&gameplay_player, &gameplay_animation, SCREEN_ANIMATION_PLAY_ONCE, false);
	gameplay_player.on_finish     = NULL;
	gameplay_player.on_finish_ctx = NULL;
}

void gameplay_startExit(void (*on_finish)(void *ctx), void *ctx)
{
	screenAnimationPlayer_start(&gameplay_player, &gameplay_animation, SCREEN_ANIMATION_PLAY_ONCE, true);
	gameplay_player.on_finish     = on_finish;
	gameplay_player.on_finish_ctx = ctx;
}

bool gameplay_isTransitioning(void)
{
	return gameplay_player.is_active;
}
