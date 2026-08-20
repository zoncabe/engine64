#include <libdragon.h>

#include "time/time.h"
#include "graphics/font.h"
#include "graphics/sprites.h"
#include "graphics/shapes.h"
#include "render/render.h"
#include "screen/screen.h"
#include "screen/screen_animation.h"
#include "ui/credits_ui.h"


/* Scroll window: the content lives in section 1, clipped by the scissor.
   credits_ui_update integrates the offset from the velocity the control sets
   each frame. */
#define CREDITS_CLIP_Y       50.0f
#define CREDITS_CLIP_H      162.0f
#define CREDITS_BASE_Y       80.0f
#define CREDITS_ROLL_X       36.0f
#define CREDITS_ROLL_WIDTH    248
#define CREDITS_SCROLL_END  190.0f


static ScreenAnimationPlayer credits_player;

static float credits_offset;
static float credits_offset_max;
static float credits_scroll_velocity;


static const rdpq_textparms_t h14_parms   = { .char_spacing = 0.95f };
static const rdpq_textparms_t h40_parms   = { .char_spacing = 0.0f  };
static const rdpq_textparms_t roll_parms  = { .width = CREDITS_ROLL_WIDTH, .wrap = WRAP_WORD };


typedef enum {

	CREDITS_BG,
	CREDITS_TITLE,

} CreditsFixedElement;

typedef enum {

	CREDITS_HINT_SCROLL,
	CREDITS_HINT_BACK,
	CREDITS_D_UP,
	CREDITS_D_DOWN,
	CREDITS_BTN_B,

} CreditsHintElement;

typedef enum {

	CREDITS_ROLL,
	CREDITS_ROLL_COUNT,

} CreditsRollElement;

typedef enum {

	CREDITS_FADE_TOP,
	CREDITS_FADE_BOTTOM,

} CreditsOverlayElement;

typedef enum {

	CREDITS_COVER,

} CreditsCoverElement;


/* ^01 grey, ^02 yellow: the styles loaded in font_loadAsset. */
static const char credits_roll_text[] =
	"\n"
	"^02zoncabe\n"
	"^01yours truly, creator of the project\n"
	"\n"
	"the project was born after the 2022 N64brew jam, continuing the base that team achieved together\n"
	"\n"
	"\n"
	"^02SPECIAL THANKS:\n"
	"\n"
	"^02Buu342\n"
	"^01who taught me how to model in blender, welcomed me into the N64brew discord and made me feel right at home\n"
	"\n"
	"^02Jaltekruse\n"
	"^01creator of the entity system that survives to this day, who taught me the basics of gamedev on the calls we had for that jam\n"
	"\n"
	"^02libdragon and tiny3d teams\n"
	"^01for creating and maintaining the most refined libraries in the whole retro scene\n"
	"\n"
	"^02rasky, HailToDodongo, snacchus, anacierdem, meeq, networkfusion, SpookyIluha, gamemasterplc, thekovic and many others...\n"
	"\n"
	"you are the best!!";


Screen credits_screen = {
	.section = {
		[0] = {
			.element = {
				[CREDITS_BG]          = { .type = DRAW_RECTANGLE, .position = {   0.0f,   0.0f }, .scale = { 320.0f, 240.0f }, .rectangle = { SHAPE_FILL_GRADIENT, .gradient = { RGBA32(25, 121, 201, 255), RGBA32(117, 175, 223, 255), RGBA32(117, 175, 223, 255), RGBA32(25, 121, 201, 255) } } },
				[CREDITS_TITLE]       = { .type = DRAW_TEXT,      .position = {  43.0f,  45.0f },                              .text      = { XOLONIUM_40, MENU_STYLE_NORMAL, "Credits", &h40_parms } },
			},
			.element_count = 2,
		},
		[1] = {
			.has_scissor = true,
			.scissor_x = 0.0f,   .scissor_y = CREDITS_CLIP_Y,
			.scissor_w = 320.0f, .scissor_h = CREDITS_CLIP_H,
			.element = {
				[CREDITS_ROLL] = { .type = DRAW_TEXT, .position = { CREDITS_ROLL_X, 0.0f }, .text = { XOLONIUM_14, MENU_STYLE_NORMAL, credits_roll_text, &roll_parms } },
			},
			.element_count = CREDITS_ROLL_COUNT,
		},
		/* Fade at the window edges: two gradients in the background colour,
		   opaque toward the edge and transparent toward the centre, drawn over
		   the text. The background runs horizontally, so each overlay copies
		   the colours of its two ends. */
		[2] = {
			.element = {
				[CREDITS_FADE_TOP]    = { .type = DRAW_RECTANGLE, .position = { 0.0f,  50.0f }, .scale = { 320.0f, 14.0f }, .rectangle = { SHAPE_FILL_GRADIENT, .gradient = { RGBA32(25, 121, 201, 255), RGBA32(117, 175, 223, 255), RGBA32(117, 175, 223, 0), RGBA32(25, 121, 201, 0) } } },
				[CREDITS_FADE_BOTTOM] = { .type = DRAW_RECTANGLE, .position = { 0.0f, 198.0f }, .scale = { 320.0f, 14.0f }, .rectangle = { SHAPE_FILL_GRADIENT, .gradient = { RGBA32(25, 121, 201, 0), RGBA32(117, 175, 223, 0), RGBA32(117, 175, 223, 255), RGBA32(25, 121, 201, 255) } } },
			},
			.element_count = 2,
		},
		/* The hints draw last, over the bottom fade. */
		[3] = {
			.element = {
				[CREDITS_HINT_SCROLL] = { .type = DRAW_TEXT,   .position = { 273.0f, 201.0f },                            .text   = { XOLONIUM_14, 0, "Scroll", &h14_parms } },
				[CREDITS_HINT_BACK]   = { .type = DRAW_TEXT,   .position = { 281.0f, 216.0f },                            .text   = { XOLONIUM_14, 0, "Back",   &h14_parms } },
				[CREDITS_D_UP]        = { .type = DRAW_SPRITE, .position = { 248.0f, 193.0f }, .scale = { 0.48f, 0.48f }, .sprite = { SPRITE_D_UP   } },
				[CREDITS_D_DOWN]      = { .type = DRAW_SPRITE, .position = { 258.0f, 194.0f }, .scale = { 0.48f, 0.48f }, .sprite = { SPRITE_D_DOWN } },
				[CREDITS_BTN_B]       = { .type = DRAW_SPRITE, .position = { 266.0f, 207.0f }, .scale = { 0.60f, 0.60f }, .sprite = { SPRITE_BTN_B  } },
			},
			.element_count = 5,
		},
		[4] = {
			.element = {
				[CREDITS_COVER] = { .type = DRAW_RECTANGLE, .position = { 0.0f, 0.0f }, .scale = { 320.0f, 240.0f }, .rectangle = { SHAPE_FILL_GRADIENT, .gradient = { RGBA32(0, 0, 0, 255), RGBA32(0, 0, 0, 255), RGBA32(0, 0, 0, 255), RGBA32(0, 0, 0, 255) } }, .is_hidden = true },
			},
			.element_count = 1,
		},
	},
	.section_count = 5,
};


/* The whole entry is the black cover, dither noise included, dropping its
   alpha: background, text and sprites come up together, with no motion.
   Reversed, the same cover fades to black on the way out. Once the window is
   over the rect sends no command to the RDP. */
static const ScreenAnimationTrack credits_transition_track[] = {

	{ .target_u8 = &credits_screen.section[4].element[CREDITS_COVER].rectangle.gradient[0].a, .from = 255.0f, .to = 0.0f, .delay = 0.00f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_CUBIC_IN },
	{ .target_u8 = &credits_screen.section[4].element[CREDITS_COVER].rectangle.gradient[1].a, .from = 255.0f, .to = 0.0f, .delay = 0.00f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_CUBIC_IN },
	{ .target_u8 = &credits_screen.section[4].element[CREDITS_COVER].rectangle.gradient[2].a, .from = 255.0f, .to = 0.0f, .delay = 0.00f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_CUBIC_IN },
	{ .target_u8 = &credits_screen.section[4].element[CREDITS_COVER].rectangle.gradient[3].a, .from = 255.0f, .to = 0.0f, .delay = 0.00f, .duration = 0.25f, .easing = SCREEN_ANIMATION_EASING_CUBIC_IN },

	{ .target_bool = &credits_screen.section[4].element[CREDITS_COVER].is_hidden, .from_bool = true, .to_bool = false, .duration = 0.25f },
};

static const ScreenAnimation credits_transition_animation = {
	.track       = credits_transition_track,
	.track_count = sizeof(credits_transition_track) / sizeof(credits_transition_track[0]),
};


void credits_ui_setScrollVelocity(float velocity)
{
	credits_scroll_velocity = velocity;
}

void credits_ui_update(void)
{
	if (credits_player.is_active)
		screenAnimationPlayer_update(&credits_player, time_get()->delta);

	credits_offset += credits_scroll_velocity * time_get()->delta;
	if (credits_offset < 0.0f)               credits_offset = 0.0f;
	if (credits_offset > credits_offset_max) credits_offset = credits_offset_max;
	credits_scroll_velocity = 0.0f;

	/* Whole pixels: half a position splits the glyphs when rasterized. */
	credits_screen.section[1].element[CREDITS_ROLL].position.y = floorf(CREDITS_BASE_Y - credits_offset);
}


void credits_ui_startEnter(void)
{
	credits_offset          = 0.0f;
	credits_scroll_velocity = 0.0f;

	int nbytes = sizeof(credits_roll_text) - 1;
	rdpq_paragraph_t *layout = rdpq_paragraph_build(&roll_parms, XOLONIUM_14, credits_roll_text, &nbytes);
	credits_offset_max = CREDITS_BASE_Y + layout->bbox.y1 - CREDITS_SCROLL_END;
	rdpq_paragraph_free(layout);
	if (credits_offset_max < 0.0f) credits_offset_max = 0.0f;
	screenAnimationPlayer_start(&credits_player, &credits_transition_animation, SCREEN_ANIMATION_PLAY_ONCE, false);
	credits_player.on_finish     = NULL;
	credits_player.on_finish_ctx = NULL;
}


void credits_ui_startExit(void (*on_finish)(void *ctx), void *ctx)
{
	screenAnimationPlayer_start(&credits_player, &credits_transition_animation, SCREEN_ANIMATION_PLAY_ONCE, true);
	credits_player.on_finish     = on_finish;
	credits_player.on_finish_ctx = ctx;
}


bool credits_ui_isTransitioning(void)
{
	return credits_player.is_active;
}
