#include "sound/sound.h"
#include "sound/sound_bank.h"


const SoundDef sound_bank[SOUND_COUNT] = {

	[SOUND_FLAG_FLAPPING] = {
		.path         = "rom:/audio/flag_flapping.wav64",
		.volume       = 0.9f,
		.min_distance = 600.0f,
		.max_distance = 3000.0f,
		.loop         = true,
		.priority     = SOUND_PRIORITY_AMBIENCE,
		.preload      = false,
	},

	[SOUND_FOOTSTEP_1] = {
		.path         = "rom:/audio/footstep_01.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_FOOTSTEP_2] = {
		.path         = "rom:/audio/footstep_02.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_FOOTSTEP_3] = {
		.path         = "rom:/audio/footstep_03.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_FOOTSTEP_4] = {
		.path         = "rom:/audio/footstep_04.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_ROLL_1] = {
		.path         = "rom:/audio/roll_01.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1600.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_ROLL_2] = {
		.path         = "rom:/audio/roll_02.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1600.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},
	[SOUND_SWIM_STROKE_LIGHT_1] = {
		.path         = "rom:/audio/swim_stroke_light.wav64",
		.volume       = 1.0f,
		.min_distance = 150.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_SWIM_STROKE_LIGHT_2] = {
		.path         = "rom:/audio/swim_stroke_light_2.wav64",
		.volume       = 1.0f,
		.min_distance = 150.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_SWIM_STROKE_HEAVY_1] = {
		.path         = "rom:/audio/swim_stroke_heavy.wav64",
		.volume       = 1.0f,
		.min_distance = 150.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_SWIM_STROKE_HEAVY_2] = {
		.path         = "rom:/audio/swim_stroke_heavy_2.wav64",
		.volume       = 1.0f,
		.min_distance = 150.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_SWIM_SPLASH_1] = {
		.path         = "rom:/audio/swim_splash.wav64",
		.volume       = 1.0f,
		.min_distance = 150.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_SWIM_SPLASH_2] = {
		.path         = "rom:/audio/swim_splash_2.wav64",
		.volume       = 1.0f,
		.min_distance = 150.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_THUD_1] = {
		.path         = "rom:/audio/thud_01.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1600.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_THUD_2] = {
		.path         = "rom:/audio/thud_02.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1600.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_THUD_3] = {
		.path         = "rom:/audio/thud_03.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1600.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_THUD_4] = {
		.path         = "rom:/audio/thud_04.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1600.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

};
