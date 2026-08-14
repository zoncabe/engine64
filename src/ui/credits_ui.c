#include <libdragon.h>

#include "time/time.h"
#include "graphics/font.h"
#include "graphics/sprites.h"
#include "graphics/shapes.h"
#include "render/render.h"
#include "screen/screen.h"
#include "screen/screen_animation.h"
#include "ui/credits_ui.h"


/* Ventana de scroll: el contenido vive en la seccion 1, recortada por
   scissor. El offset lo integra credits_update con la velocidad que el
   control setea cada frame; las lineas fuera de la ventana se ocultan
   para no mandar ni un comando. */
#define CREDITS_CLIP_Y       50.0f
#define CREDITS_CLIP_H      162.0f
#define CREDITS_BASE_Y       80.0f
#define CREDITS_CONTENT_END 392.0f
#define CREDITS_OFFSET_MAX  (CREDITS_BASE_Y + CREDITS_CONTENT_END - 190.0f)
#define CREDITS_CULL_TOP     38.0f
#define CREDITS_CULL_BOTTOM 228.0f


static ScreenAnimationPlayer credits_player;

static float credits_offset;
static float credits_scroll_velocity;


static const rdpq_textparms_t h14_parms = { .char_spacing = 0.95f };
static const rdpq_textparms_t h40_parms = { .char_spacing = 0.0f  };


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

	CREDITS_ZONCABE,
	CREDITS_ZONCABE_ROLE,
	CREDITS_BORN_1,
	CREDITS_BORN_2,
	CREDITS_BORN_3,
	CREDITS_BUU,
	CREDITS_BUU_1,
	CREDITS_BUU_2,
	CREDITS_BUU_3,
	CREDITS_BUU_4,
	CREDITS_JASON,
	CREDITS_JASON_1,
	CREDITS_JASON_2,
	CREDITS_JASON_3,
	CREDITS_JASON_4,
	CREDITS_LIBS,
	CREDITS_LIBS_1,
	CREDITS_LIBS_2,
	CREDITS_LIBS_3,
	CREDITS_LIBS_NAMES_1,
	CREDITS_LIBS_NAMES_2,
	CREDITS_LIBS_NAMES_3,
	CREDITS_LIBS_NAMES_4,
	CREDITS_THANKS,
	CREDITS_ROLL_COUNT,

} CreditsRollElement;

typedef enum {

	CREDITS_FADE_TOP,
	CREDITS_FADE_BOTTOM,

} CreditsOverlayElement;

typedef enum {

	CREDITS_COVER,

} CreditsCoverElement;


/* Posicion vertical de cada linea dentro del contenido (offset 0 = primera
   linea en la parte alta de la ventana, debajo del titulo fijo). */
static const float credits_rel[CREDITS_ROLL_COUNT] = {

	[CREDITS_ZONCABE]      =   0.0f,
	[CREDITS_ZONCABE_ROLE] =  14.0f,
	[CREDITS_BORN_1]       =  42.0f,
	[CREDITS_BORN_2]       =  56.0f,
	[CREDITS_BORN_3]       =  70.0f,
	[CREDITS_BUU]          =  98.0f,
	[CREDITS_BUU_1]        = 112.0f,
	[CREDITS_BUU_2]        = 126.0f,
	[CREDITS_BUU_3]        = 140.0f,
	[CREDITS_BUU_4]        = 154.0f,
	[CREDITS_JASON]        = 182.0f,
	[CREDITS_JASON_1]      = 196.0f,
	[CREDITS_JASON_2]      = 210.0f,
	[CREDITS_JASON_3]      = 224.0f,
	[CREDITS_JASON_4]      = 238.0f,
	[CREDITS_LIBS]         = 266.0f,
	[CREDITS_LIBS_1]       = 280.0f,
	[CREDITS_LIBS_2]       = 294.0f,
	[CREDITS_LIBS_3]       = 308.0f,
	[CREDITS_LIBS_NAMES_1] = 322.0f,
	[CREDITS_LIBS_NAMES_2] = 336.0f,
	[CREDITS_LIBS_NAMES_3] = 350.0f,
	[CREDITS_LIBS_NAMES_4] = 364.0f,
	[CREDITS_THANKS]       = CREDITS_CONTENT_END,
};


Screen credits_screen = {
	.section = {
		[0] = {
			.element = {
				[CREDITS_BG]          = { .type = DRAW_RECTANGLE, .position = {   0.0f,   0.0f }, .scale = { 320.0f, 240.0f }, .rectangle = { SHAPE_FILL_GRADIENT, .gradient = { RGBA32(25, 121, 201, 255), RGBA32(117, 175, 223, 255), RGBA32(117, 175, 223, 255), RGBA32(25, 121, 201, 255) } } },
				[CREDITS_TITLE]       = { .type = DRAW_TEXT,      .position = {  43.0f,  45.0f },                              .text      = { HEADLINER_40, MENU_STYLE_NORMAL, "Credits", &h40_parms } },
			},
			.element_count = 2,
		},
		[1] = {
			.has_scissor = true,
			.scissor_x = 0.0f,   .scissor_y = CREDITS_CLIP_Y,
			.scissor_w = 320.0f, .scissor_h = CREDITS_CLIP_H,
			.element = {
				[CREDITS_ZONCABE]      = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_SELECTED, "zoncabe",                             &h14_parms } },
				[CREDITS_ZONCABE_ROLE] = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_NORMAL,   "yours truly, creator of the project", &h14_parms } },
				[CREDITS_BORN_1]       = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_NORMAL,   "the project was born after the",      &h14_parms } },
				[CREDITS_BORN_2]       = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_NORMAL,   "2022 N64brew jam, continuing the",    &h14_parms } },
				[CREDITS_BORN_3]       = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_NORMAL,   "base that team achieved together",    &h14_parms } },
				[CREDITS_BUU]          = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_SELECTED, "Buu342",                              &h14_parms } },
				[CREDITS_BUU_1]        = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_NORMAL,   "who taught me how to model in",       &h14_parms } },
				[CREDITS_BUU_2]        = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_NORMAL,   "blender, welcomed me into the",       &h14_parms } },
				[CREDITS_BUU_3]        = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_NORMAL,   "N64brew discord and made me",         &h14_parms } },
				[CREDITS_BUU_4]        = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_NORMAL,   "feel right at home",                  &h14_parms } },
				[CREDITS_JASON]        = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_SELECTED, "Jaltekruse",                          &h14_parms } },
				[CREDITS_JASON_1]      = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_NORMAL,   "creator of the entity system",        &h14_parms } },
				[CREDITS_JASON_2]      = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_NORMAL,   "that survives to this day, who",      &h14_parms } },
				[CREDITS_JASON_3]      = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_NORMAL,   "taught me the basics of gamedev",     &h14_parms } },
				[CREDITS_JASON_4]      = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_NORMAL,   "on the calls we had for that jam",    &h14_parms } },
				[CREDITS_LIBS]         = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_SELECTED, "libdragon and tiny3d teams",          &h14_parms } },
				[CREDITS_LIBS_1]       = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_NORMAL,   "for creating and maintaining",        &h14_parms } },
				[CREDITS_LIBS_2]       = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_NORMAL,   "the most refined libraries",    &h14_parms } },
				[CREDITS_LIBS_3]       = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_NORMAL,   "in the whole retro scene",                  &h14_parms } },
				[CREDITS_LIBS_NAMES_1] = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_SELECTED, "rasky, HailToDodongo, snacchus,",     &h14_parms } },
				[CREDITS_LIBS_NAMES_2] = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_SELECTED, "anacierdem, meeq, networkfusion,",    &h14_parms } },
				[CREDITS_LIBS_NAMES_3] = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_SELECTED, "SpookyIluha, gamemasterplc,",          &h14_parms } },
				[CREDITS_LIBS_NAMES_4] = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_SELECTED, "thekovic and many others...",         &h14_parms } },
				[CREDITS_THANKS]       = { .type = DRAW_TEXT, .position = { 36.0f, 0.0f }, .text = { HEADLINER_14, MENU_STYLE_SELECTED, "thank you all!!",                     &h14_parms } },
			},
			.element_count = CREDITS_ROLL_COUNT,
		},
		/* Fundido en los bordes de la ventana: dos gradientes del color del
		   fondo (opaco hacia el borde, transparente hacia el centro) dibujados
		   encima del texto. El fondo degrada en horizontal, asi que cada
		   overlay copia los colores de sus dos extremos. */
		[2] = {
			.element = {
				[CREDITS_FADE_TOP]    = { .type = DRAW_RECTANGLE, .position = { 0.0f,  50.0f }, .scale = { 320.0f, 14.0f }, .rectangle = { SHAPE_FILL_GRADIENT, .gradient = { RGBA32(25, 121, 201, 255), RGBA32(117, 175, 223, 255), RGBA32(117, 175, 223, 0), RGBA32(25, 121, 201, 0) } } },
				[CREDITS_FADE_BOTTOM] = { .type = DRAW_RECTANGLE, .position = { 0.0f, 198.0f }, .scale = { 320.0f, 14.0f }, .rectangle = { SHAPE_FILL_GRADIENT, .gradient = { RGBA32(25, 121, 201, 0), RGBA32(117, 175, 223, 0), RGBA32(117, 175, 223, 255), RGBA32(25, 121, 201, 255) } } },
			},
			.element_count = 2,
		},
		/* Los hints dibujan al final, por encima del fade inferior. */
		[3] = {
			.element = {
				[CREDITS_HINT_SCROLL] = { .type = DRAW_TEXT,   .position = { 273.0f, 201.0f },                            .text   = { HEADLINER_14, 0, "Scroll", &h14_parms } },
				[CREDITS_HINT_BACK]   = { .type = DRAW_TEXT,   .position = { 281.0f, 216.0f },                            .text   = { HEADLINER_14, 0, "Back",   &h14_parms } },
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


/* La entrada entera es el cover negro (con su ruido de dither) bajando el
   alfa: BG, textos y sprites emergen juntos, sin movimiento. En reversa el
   mismo cover funde a negro para salir. Terminada la ventana, el rect no
   manda ningun comando al RDP. */
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


void credits_setScrollVelocity(float velocity)
{
	credits_scroll_velocity = velocity;
}

void credits_update(void)
{
	if (credits_player.is_active)
		screenAnimationPlayer_update(&credits_player, time_get()->delta);

	credits_offset += credits_scroll_velocity * time_get()->delta;
	if (credits_offset < 0.0f)               credits_offset = 0.0f;
	if (credits_offset > CREDITS_OFFSET_MAX) credits_offset = CREDITS_OFFSET_MAX;
	credits_scroll_velocity = 0.0f;

	ScreenSection *section = &credits_screen.section[1];
	for (int i = 0; i < section->element_count; i++) {
		float y = CREDITS_BASE_Y + credits_rel[i] - credits_offset;
		section->element[i].position.y = y;
		section->element[i].is_hidden  = (y < CREDITS_CULL_TOP) || (y > CREDITS_CULL_BOTTOM);
	}
}


void credits_startEnter(void)
{
	credits_offset          = 0.0f;
	credits_scroll_velocity = 0.0f;
	screenAnimationPlayer_start(&credits_player, &credits_transition_animation, SCREEN_ANIMATION_PLAY_ONCE, false);
	credits_player.on_finish     = NULL;
	credits_player.on_finish_ctx = NULL;
}


void credits_startExit(void (*on_finish)(void *ctx), void *ctx)
{
	screenAnimationPlayer_start(&credits_player, &credits_transition_animation, SCREEN_ANIMATION_PLAY_ONCE, true);
	credits_player.on_finish     = on_finish;
	credits_player.on_finish_ctx = ctx;
}


bool credits_isTransitioning(void)
{
	return credits_player.is_active;
}
