#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>
#include <stdint.h>


typedef enum {
	DIFFICULTY_EASY,
	DIFFICULTY_NORMAL,
	DIFFICULTY_HARD,
	DIFFICULTY_COUNT,
} Difficulty;

typedef enum {
	LANGUAGE_EN,
	LANGUAGE_ES,
	LANGUAGE_COUNT,
} Language;

typedef enum {
	ASPECT_4_3,
	ASPECT_16_9,
	ASPECT_COUNT,
} AspectRatio;


typedef struct {

	uint8_t master_volume;
	uint8_t music_volume;
	uint8_t sfx_volume;
	uint8_t voice_volume;
	bool    mute;

	bool    invert_camera_y;
	bool    invert_camera_x;
	uint8_t camera_max_speed;
	uint8_t camera_response;
	bool    vibration;

	uint8_t    difficulty;
	uint8_t    language;
	bool       subtitles;
	bool       auto_save;
	bool       hud_visible;
	bool       tutorial_hints;

	uint8_t brightness;
	uint8_t contrast;
	uint8_t gamma;
	uint8_t aspect_ratio;
	bool    anti_aliasing;
	bool    vsync;
	bool    screen_shake;

} Settings;


Settings *settings_get(void);
void      settings_init(void);
void      settings_reset(void);

#endif
