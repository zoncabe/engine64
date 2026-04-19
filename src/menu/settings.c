#include "menu/settings.h"


static Settings settings;


Settings *settings_get(void) { return &settings; }

void settings_init(void)
{
	settings = (Settings){
		.master_volume    = 80,
		.music_volume     = 70,
		.sfx_volume       = 80,
		.voice_volume     = 80,
		.mute             = false,

		.invert_camera_y  = false,
		.invert_camera_x  = false,
		.camera_max_speed = 50,
		.camera_response  = 50,
		.vibration        = true,

		.difficulty       = DIFFICULTY_NORMAL,
		.language         = LANGUAGE_EN,
		.subtitles        = true,
		.auto_save        = true,
		.hud_visible      = true,
		.tutorial_hints   = true,

		.brightness       = 50,
		.contrast         = 50,
		.gamma            = 50,
		.aspect_ratio     = ASPECT_4_3,
		.anti_aliasing    = true,
		.vsync            = true,
		.screen_shake     = true,
	};
}

void settings_reset(void) { settings_init(); }
