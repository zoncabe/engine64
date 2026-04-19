#include <libdragon.h>
#include <stddef.h>

#include "time/time.h"
#include "graphics/font.h"
#include "graphics/sprites.h"
#include "graphics/shapes.h"
#include "render/render.h"
#include "screen/screen.h"
#include "screen/screen_animation.h"
#include "menu/menu.h"
#include "menu/settings.h"
#include "ui/settings_ui.h"


#define TAB_COUNT        4
#define ROW_MAX          7
#define CAROUSEL_X       50.0f
#define CAROUSEL_Y       68.0f
#define CAROUSEL_W       220.0f
#define CAROUSEL_H       116.0f
#define ROW_HEIGHT       22.0f
#define VALUE_TEXT_SIZE  16


typedef enum {
	ITEM_TOGGLE,
	ITEM_ENUM,
	ITEM_SLIDER,
} SettingsItemKind;

typedef struct {
	SettingsItemKind   kind;
	const char        *label;
	uint16_t           field_offset;
	uint8_t            enum_count;
	uint8_t            step;
	const char *const *enum_label;
	const char        *format;
} SettingsItem;

#define FIELD(f) (uint16_t)offsetof(Settings, f)

static void *settings_ui_target(const SettingsItem *item)
{
	return (uint8_t *)settings_get() + item->field_offset;
}


static const char *const label_off_on[]     = { "Off", "On" };
static const char *const label_difficulty[] = { "Easy", "Normal", "Hard" };
static const char *const label_language[]   = { "English", "Spanish" };
static const char *const label_aspect[]     = { "4:3", "16:9" };


static const SettingsItem item_gameplay[] = {
	{ ITEM_ENUM,   "Difficulty",     FIELD(difficulty),     DIFFICULTY_COUNT, 0, label_difficulty, NULL   },
	{ ITEM_ENUM,   "Language",       FIELD(language),       LANGUAGE_COUNT,   0, label_language,   NULL   },
	{ ITEM_TOGGLE, "Subtitles",      FIELD(subtitles),      0,                0, label_off_on,     NULL   },
	{ ITEM_TOGGLE, "Auto Save",      FIELD(auto_save),      0,                0, label_off_on,     NULL   },
	{ ITEM_TOGGLE, "HUD",            FIELD(hud_visible),    0,                0, label_off_on,     NULL   },
	{ ITEM_TOGGLE, "Tutorial Hints", FIELD(tutorial_hints), 0,                0, label_off_on,     NULL   },
};

static const SettingsItem item_controls[] = {
	{ ITEM_TOGGLE, "Invert Y",        FIELD(invert_camera_y),  0, 0, label_off_on, NULL   },
	{ ITEM_TOGGLE, "Invert X",        FIELD(invert_camera_x),  0, 0, label_off_on, NULL   },
	{ ITEM_SLIDER, "Camera Speed",    FIELD(camera_max_speed), 0, 5, NULL,         "%u%%" },
	{ ITEM_SLIDER, "Camera Response", FIELD(camera_response),  0, 5, NULL,         "%u%%" },
	{ ITEM_TOGGLE, "Vibration",       FIELD(vibration),        0, 0, label_off_on, NULL   },
};

static const SettingsItem item_video[] = {
	{ ITEM_SLIDER, "Brightness",    FIELD(brightness),    0,            5, NULL,         "%u%%" },
	{ ITEM_SLIDER, "Contrast",      FIELD(contrast),      0,            5, NULL,         "%u%%" },
	{ ITEM_SLIDER, "Gamma",         FIELD(gamma),         0,            5, NULL,         "%u%%" },
	{ ITEM_ENUM,   "Aspect Ratio",  FIELD(aspect_ratio),  ASPECT_COUNT, 0, label_aspect, NULL   },
	{ ITEM_TOGGLE, "Anti-Aliasing", FIELD(anti_aliasing), 0,            0, label_off_on, NULL   },
	{ ITEM_TOGGLE, "V-Sync",        FIELD(vsync),         0,            0, label_off_on, NULL   },
	{ ITEM_TOGGLE, "Screen Shake",  FIELD(screen_shake),  0,            0, label_off_on, NULL   },
};

static const SettingsItem item_audio[] = {
	{ ITEM_SLIDER, "Master", FIELD(master_volume), 0, 5, NULL,         "%u%%" },
	{ ITEM_SLIDER, "Music",  FIELD(music_volume),  0, 5, NULL,         "%u%%" },
	{ ITEM_SLIDER, "SFX",    FIELD(sfx_volume),    0, 5, NULL,         "%u%%" },
	{ ITEM_SLIDER, "Voice",  FIELD(voice_volume),  0, 5, NULL,         "%u%%" },
	{ ITEM_TOGGLE, "Mute",   FIELD(mute),          0, 0, label_off_on, NULL   },
};


static const SettingsItem *tab_items[TAB_COUNT] = {
	item_gameplay, item_controls, item_video, item_audio,
};

static const uint8_t tab_item_count[TAB_COUNT] = {
	sizeof(item_gameplay) / sizeof(item_gameplay[0]),
	sizeof(item_controls) / sizeof(item_controls[0]),
	sizeof(item_video)    / sizeof(item_video[0]),
	sizeof(item_audio)    / sizeof(item_audio[0]),
};


static int8_t settings_active_tab = 0;
static int8_t settings_active_row = 0;
static int8_t settings_context    = 0;

static char value_text[TAB_COUNT][ROW_MAX][VALUE_TEXT_SIZE];

static const rdpq_textparms_t h14_parms = { .char_spacing = 0.95f };


static ScreenAnimationPlayer settings_player;


typedef enum {
	EL_BG,
	EL_TAB_0, EL_TAB_1, EL_TAB_2, EL_TAB_3,
	EL_HINT_MOVE, EL_HINT_ADJUST, EL_HINT_BACK,
	EL_D_UP, EL_D_DOWN, EL_D_LEFT, EL_D_RIGHT, EL_BTN_B,
	EL_HEADER_COUNT,
} HeaderElement;


Screen settings_screen = {
	.section = {
		[0] = {
			.element = {
				[EL_BG]          = { .type = DRAW_RECTANGLE, .position = {   0.0f,   0.0f }, .scale = { 320.0f, 240.0f }, .rectangle = { SHAPE_FILL_GRADIENT } },
				[EL_TAB_0]       = { .type = DRAW_TEXT,      .position = {  50.0f,  44.0f },                              .text = { HEADLINER_14, MENU_STYLE_NORMAL, "Gameplay", &h14_parms } },
				[EL_TAB_1]       = { .type = DRAW_TEXT,      .position = { 112.0f,  44.0f },                              .text = { HEADLINER_14, MENU_STYLE_NORMAL, "Controls", &h14_parms } },
				[EL_TAB_2]       = { .type = DRAW_TEXT,      .position = { 170.0f,  44.0f },                              .text = { HEADLINER_14, MENU_STYLE_NORMAL, "Video",    &h14_parms } },
				[EL_TAB_3]       = { .type = DRAW_TEXT,      .position = { 216.0f,  44.0f },                              .text = { HEADLINER_14, MENU_STYLE_NORMAL, "Audio",    &h14_parms } },
				[EL_HINT_MOVE]   = { .type = DRAW_TEXT,      .position = {  65.0f, 225.0f },                              .text = { HEADLINER_14, 0, "Move",   &h14_parms } },
				[EL_HINT_ADJUST] = { .type = DRAW_TEXT,      .position = { 160.0f, 225.0f },                              .text = { HEADLINER_14, 0, "Adjust", &h14_parms } },
				[EL_HINT_BACK]   = { .type = DRAW_TEXT,      .position = { 240.0f, 225.0f },                              .text = { HEADLINER_14, 0, "Back",   &h14_parms } },
				[EL_D_UP]        = { .type = DRAW_SPRITE,    .position = {  43.0f, 217.0f }, .scale = { 0.48f, 0.48f },   .sprite = { SPRITE_D_UP    } },
				[EL_D_DOWN]      = { .type = DRAW_SPRITE,    .position = {  53.0f, 217.0f }, .scale = { 0.48f, 0.48f },   .sprite = { SPRITE_D_DOWN  } },
				[EL_D_LEFT]      = { .type = DRAW_SPRITE,    .position = { 135.0f, 217.0f }, .scale = { 0.48f, 0.48f },   .sprite = { SPRITE_D_LEFT  } },
				[EL_D_RIGHT]     = { .type = DRAW_SPRITE,    .position = { 145.0f, 217.0f }, .scale = { 0.48f, 0.48f },   .sprite = { SPRITE_D_RIGHT } },
				[EL_BTN_B]       = { .type = DRAW_SPRITE,    .position = { 225.0f, 216.0f }, .scale = { 0.60f, 0.60f },   .sprite = { SPRITE_BTN_B   } },
			},
			.element_count = EL_HEADER_COUNT,
		},
		[1] = {
			.has_scissor = true,
			.scissor_x = CAROUSEL_X, .scissor_y = CAROUSEL_Y,
			.scissor_w = CAROUSEL_W, .scissor_h = CAROUSEL_H,
		},
	},
	.section_count = 2,
};


static const float tab_offset[TAB_COUNT] = { 0.0f, -CAROUSEL_W, -2*CAROUSEL_W, -3*CAROUSEL_W };

static const float tab_style_0[TAB_COUNT] = { MENU_STYLE_SELECTED, MENU_STYLE_NORMAL,   MENU_STYLE_NORMAL,   MENU_STYLE_NORMAL   };
static const float tab_style_1[TAB_COUNT] = { MENU_STYLE_NORMAL,   MENU_STYLE_SELECTED, MENU_STYLE_NORMAL,   MENU_STYLE_NORMAL   };
static const float tab_style_2[TAB_COUNT] = { MENU_STYLE_NORMAL,   MENU_STYLE_NORMAL,   MENU_STYLE_SELECTED, MENU_STYLE_NORMAL   };
static const float tab_style_3[TAB_COUNT] = { MENU_STYLE_NORMAL,   MENU_STYLE_NORMAL,   MENU_STYLE_NORMAL,   MENU_STYLE_SELECTED };

static ScreenAnimationTrack settings_track[192];
static uint8_t               settings_track_count = 0;

static ScreenAnimation settings_animation = {
	.track       = settings_track,
	.track_count = 0,
};


#define SLIDE_OFFSET    400.0f
#define ENTER_DURATION   0.25f
#define ENTER_STAGGER    0.06f

static ScreenAnimationTrack settings_transition_main_menu_track[64];
static uint8_t               settings_transition_main_menu_track_count = 0;

static ScreenAnimation settings_transition_main_menu_animation = {
	.track       = settings_transition_main_menu_track,
	.track_count = 0,
};

static ScreenAnimationTrack settings_transition_pause_track[64];
static uint8_t               settings_transition_pause_track_count = 0;

static ScreenAnimation settings_transition_pause_animation = {
	.track       = settings_transition_pause_track,
	.track_count = 0,
};


void settings_ui_init(void)
{
	ScreenSection *row_section = &settings_screen.section[1];
	uint8_t row_element_count = 0;

	static float row_style_values[TAB_COUNT][ROW_MAX][ROW_MAX];
	static float row_x_by_tab[TAB_COUNT * ROW_MAX * 2][TAB_COUNT];

	for (uint8_t tab = 0; tab < TAB_COUNT; tab++) {
		float tab_x = CAROUSEL_X + tab * CAROUSEL_W;
		for (uint8_t row = 0; row < tab_item_count[tab]; row++) {
			const SettingsItem *item = &tab_items[tab][row];
			float y = CAROUSEL_Y + row * ROW_HEIGHT;

			uint8_t label_idx = row_element_count++;
			uint8_t value_idx = row_element_count++;

			row_section->element[label_idx] = (ScreenElement){
				.type = DRAW_TEXT, .position = { tab_x, y },
				.text = { HEADLINER_14, MENU_STYLE_NORMAL, item->label, &h14_parms },
			};
			row_section->element[value_idx] = (ScreenElement){
				.type = DRAW_TEXT, .position = { tab_x + 140.0f, y },
				.text = { HEADLINER_14, MENU_STYLE_NORMAL, value_text[tab][row], &h14_parms },
			};

			for (uint8_t active = 0; active < TAB_COUNT; active++) {
				row_x_by_tab[label_idx][active] = tab_x + tab_offset[active];
				row_x_by_tab[value_idx][active] = tab_x + 140.0f + tab_offset[active];
			}

			for (uint8_t i = 0; i < ROW_MAX; i++)
				row_style_values[tab][row][i] = (i == row) ? MENU_STYLE_SELECTED : MENU_STYLE_NORMAL;
		}
	}
	row_section->element_count = row_element_count;

	uint8_t track = 0;

	for (uint8_t i = 0; i < row_element_count; i++) {
		settings_track[track++] = (ScreenAnimationTrack){
			.target = &row_section->element[i].position.x,
			.source_int = &settings_active_tab, .values_by_index = row_x_by_tab[i],
		};
	}

	settings_track[track++] = (ScreenAnimationTrack){ .target_u8 = &settings_screen.section[0].element[EL_TAB_0].text.style, .source_int = &settings_active_tab, .values_by_index = tab_style_0 };
	settings_track[track++] = (ScreenAnimationTrack){ .target_u8 = &settings_screen.section[0].element[EL_TAB_1].text.style, .source_int = &settings_active_tab, .values_by_index = tab_style_1 };
	settings_track[track++] = (ScreenAnimationTrack){ .target_u8 = &settings_screen.section[0].element[EL_TAB_2].text.style, .source_int = &settings_active_tab, .values_by_index = tab_style_2 };
	settings_track[track++] = (ScreenAnimationTrack){ .target_u8 = &settings_screen.section[0].element[EL_TAB_3].text.style, .source_int = &settings_active_tab, .values_by_index = tab_style_3 };

	uint8_t element_cursor = 0;
	for (uint8_t tab = 0; tab < TAB_COUNT; tab++) {
		for (uint8_t row = 0; row < tab_item_count[tab]; row++) {
			const SettingsItem *item = &tab_items[tab][row];
			uint8_t label_idx = element_cursor++;
			uint8_t value_idx = element_cursor++;

			settings_track[track++] = (ScreenAnimationTrack){
				.target_u8 = &row_section->element[label_idx].text.style,
				.source_int = &settings_active_row, .values_by_index = row_style_values[tab][row],
			};
			settings_track[track++] = (ScreenAnimationTrack){
				.target_u8 = &row_section->element[value_idx].text.style,
				.source_int = &settings_active_row, .values_by_index = row_style_values[tab][row],
			};

			void *target = settings_ui_target(item);
			settings_track[track++] = (ScreenAnimationTrack){
				.target_text  = value_text[tab][row],
				.target_size  = VALUE_TEXT_SIZE,
				.format       = item->format,
				.source_u8    = (item->kind == ITEM_TOGGLE) ? NULL : (const uint8_t *)target,
				.source_bool  = (item->kind == ITEM_TOGGLE) ? (const bool *)target : NULL,
				.source_label = item->enum_label,
			};
		}
	}

	settings_track_count = track;
	settings_animation.track_count = track;

	Rectangle *bg_rect = &settings_screen.section[0].element[EL_BG].rectangle;
	ScreenSection *header = &settings_screen.section[0];
	float carousel_final_x = settings_screen.section[1].element[0].position.x;

	uint8_t tt;

	tt = 0;

	settings_transition_main_menu_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[0].r, .from = 12.0f, .to = 12.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_main_menu_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[0].g, .from = 14.0f, .to = 14.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_main_menu_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[0].b, .from = 28.0f, .to = 28.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_main_menu_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[0].a, .from = 255.0f, .to = 255.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };

	settings_transition_main_menu_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[1].r, .from = 40.0f, .to = 40.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_main_menu_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[1].g, .from = 30.0f, .to = 30.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_main_menu_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[1].b, .from = 70.0f, .to = 70.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_main_menu_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[1].a, .from = 255.0f, .to = 255.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };

	settings_transition_main_menu_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[2].r, .from = 40.0f, .to = 40.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_main_menu_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[2].g, .from = 30.0f, .to = 30.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_main_menu_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[2].b, .from = 70.0f, .to = 70.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_main_menu_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[2].a, .from = 255.0f, .to = 255.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };

	settings_transition_main_menu_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[3].r, .from = 12.0f, .to = 12.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_main_menu_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[3].g, .from = 14.0f, .to = 14.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_main_menu_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[3].b, .from = 28.0f, .to = 28.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_main_menu_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[3].a, .from = 255.0f, .to = 255.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };

	for (uint8_t i = EL_TAB_0; i < EL_HEADER_COUNT; i++) {
		float final_x = header->element[i].position.x;
		settings_transition_main_menu_track[tt++] = (ScreenAnimationTrack){
			.target   = &header->element[i].position.x,
			.from     = final_x + SLIDE_OFFSET,
			.to       = final_x,
			.delay    = (i - EL_TAB_0) * ENTER_STAGGER,
			.duration = ENTER_DURATION,
			.easing   = SCREEN_ANIMATION_EASING_CUBIC_OUT,
		};
	}

	settings_transition_main_menu_track[tt++] = (ScreenAnimationTrack){
		.target   = &settings_screen.section[1].element[0].position.x,
		.from     = carousel_final_x + SLIDE_OFFSET,
		.to       = carousel_final_x,
		.delay    = ENTER_STAGGER * 2,
		.duration = ENTER_DURATION,
		.easing   = SCREEN_ANIMATION_EASING_CUBIC_OUT,
	};

	settings_transition_main_menu_track_count = tt;
	settings_transition_main_menu_animation.track_count = tt;

	tt = 0;

	settings_transition_pause_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[0].r, .from = 0.0f, .to = 0.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_pause_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[0].g, .from = 0.0f, .to = 0.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_pause_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[0].b, .from = 0.0f, .to = 0.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_pause_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[0].a, .from = 76.5f, .to = 255.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };

	settings_transition_pause_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[1].r, .from = 0.0f, .to = 0.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_pause_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[1].g, .from = 0.0f, .to = 0.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_pause_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[1].b, .from = 0.0f, .to = 0.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_pause_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[1].a, .from = 255.0f, .to = 76.5f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };

	settings_transition_pause_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[2].r, .from = 0.0f, .to = 0.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_pause_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[2].g, .from = 0.0f, .to = 0.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_pause_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[2].b, .from = 0.0f, .to = 0.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_pause_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[2].a, .from = 255.0f, .to = 76.5f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };

	settings_transition_pause_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[3].r, .from = 0.0f, .to = 0.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_pause_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[3].g, .from = 0.0f, .to = 0.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_pause_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[3].b, .from = 0.0f, .to = 0.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };
	settings_transition_pause_track[tt++] = (ScreenAnimationTrack){ .target_u8 = &bg_rect->gradient[3].a, .from = 76.5f, .to = 255.0f, .duration = ENTER_DURATION, .easing = SCREEN_ANIMATION_EASING_LINEAR };

	for (uint8_t i = EL_TAB_0; i < EL_HEADER_COUNT; i++) {
		float final_x = header->element[i].position.x;
		settings_transition_pause_track[tt++] = (ScreenAnimationTrack){
			.target   = &header->element[i].position.x,
			.from     = final_x + SLIDE_OFFSET,
			.to       = final_x,
			.delay    = (i - EL_TAB_0) * ENTER_STAGGER,
			.duration = ENTER_DURATION,
			.easing   = SCREEN_ANIMATION_EASING_CUBIC_OUT,
		};
	}

	settings_transition_pause_track[tt++] = (ScreenAnimationTrack){
		.target   = &settings_screen.section[1].element[0].position.x,
		.from     = carousel_final_x + SLIDE_OFFSET,
		.to       = carousel_final_x,
		.delay    = ENTER_STAGGER * 2,
		.duration = ENTER_DURATION,
		.easing   = SCREEN_ANIMATION_EASING_CUBIC_OUT,
	};

	settings_transition_pause_track_count = tt;
	settings_transition_pause_animation.track_count = tt;
}


static void (*settings_on_close)(void) = NULL;
static bool   settings_in_submenu      = false;

static void settings_ui_onExitFinish(void *ctx)
{
	(void)ctx;
	menuStack_back();
	if (settings_on_close) settings_on_close();
}

void settings_ui_setOnClose(void (*on_close)(void))
{
	settings_on_close = on_close;
}

void settings_ui_startExit(void)
{
	if (settings_player.is_active && settings_player.is_reversed) return;
	ScreenAnimation *transition = (settings_context == SETTINGS_CTX_FROM_MAIN_MENU)
		? &settings_transition_main_menu_animation
		: &settings_transition_pause_animation;
	screenAnimationPlayer_start(&settings_player, transition, SCREEN_ANIMATION_PLAY_ONCE, true);
	settings_player.on_finish     = settings_ui_onExitFinish;
	settings_player.on_finish_ctx = NULL;
}

bool settings_ui_isTransitioning(void)
{
	return settings_player.is_active;
}

void settings_ui_open(SettingsContext context, void (*on_close)(void))
{
	settings_context    = (int8_t)context;
	settings_active_tab = 0;
	settings_active_row = 0;
	settings_in_submenu = false;
	settings_on_close   = on_close;
	menuStack_open(&settings_screen, 0);
	ScreenAnimation *transition = (context == SETTINGS_CTX_FROM_MAIN_MENU)
		? &settings_transition_main_menu_animation
		: &settings_transition_pause_animation;
	screenAnimationPlayer_start(&settings_player, transition, SCREEN_ANIMATION_PLAY_ONCE, false);
	settings_player.on_finish     = NULL;
	settings_player.on_finish_ctx = NULL;
}


void settings_ui_update(void)
{
	if (settings_player.is_active)
		screenAnimationPlayer_update(&settings_player, time_get()->delta);
	screenAnimation_apply(&settings_animation, 0.0f);
}


static void settings_ui_adjustValue(int8_t dir)
{
	const SettingsItem *item = &tab_items[settings_active_tab][settings_active_row];

	if (item->kind == ITEM_TOGGLE) {
		bool *target = settings_ui_target(item);
		*target = !*target;
	} else if (item->kind == ITEM_ENUM) {
		uint8_t *target = settings_ui_target(item);
		*target = (*target + item->enum_count + dir) % item->enum_count;
	} else {
		uint8_t *target = settings_ui_target(item);
		int16_t value = (int16_t)*target + dir * (int16_t)item->step;
		if (value < 0)   value = 0;
		if (value > 100) value = 100;
		*target = (uint8_t)value;
	}
}

static void settings_ui_changeTab(int8_t dir)
{
	settings_active_tab = (settings_active_tab + TAB_COUNT + dir) % TAB_COUNT;
	settings_active_row = 0;
}

void settings_ui_handleInput(const ControllerActions *actions)
{
	if (settings_player.is_active) return;

	if (!settings_in_submenu) {
		if (actions->cancel) {
			settings_ui_startExit();
			return;
		}
		if (actions->confirm) {
			settings_in_submenu = true;
			settings_active_row = 0;
			return;
		}
		if (actions->menu_left)  settings_ui_changeTab(-1);
		if (actions->menu_right) settings_ui_changeTab(+1);
		return;
	}

	if (actions->cancel) {
		settings_in_submenu = false;
		return;
	}
	if (actions->menu_tab_left)  settings_ui_changeTab(-1);
	if (actions->menu_tab_right) settings_ui_changeTab(+1);
	if (actions->menu_up) {
		settings_active_row--;
		if (settings_active_row < 0) settings_active_row = tab_item_count[settings_active_tab] - 1;
	}
	if (actions->menu_down) {
		settings_active_row++;
		if (settings_active_row >= tab_item_count[settings_active_tab]) settings_active_row = 0;
	}
	if (actions->menu_left)  settings_ui_adjustValue(-1);
	if (actions->menu_right) settings_ui_adjustValue(+1);

	if (settings_active_row >= tab_item_count[settings_active_tab])
		settings_active_row = tab_item_count[settings_active_tab] - 1;
}
