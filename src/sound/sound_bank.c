#include "sound/sound.h"
#include "sound/sound_bank.h"


const SoundDef sound_bank[SOUND_COUNT] = {

	[SOUND_FLAG_FLAPPING] = {
		.path         = "rom:/audio/flag-flapping.wav64",
		.volume       = 0.9f,
		.min_distance = 600.0f,
		.max_distance = 3000.0f,
		.loop         = true,
		.priority     = SOUND_PRIORITY_AMBIENCE,
		.preload      = false,
	},

	[SOUND_FOOTSTEP_1] = {
		.path         = "rom:/audio/footstep-01.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_FOOTSTEP_2] = {
		.path         = "rom:/audio/footstep-02.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_FOOTSTEP_3] = {
		.path         = "rom:/audio/footstep-03.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_FOOTSTEP_4] = {
		.path         = "rom:/audio/footstep-04.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_ROLL_1] = {
		.path         = "rom:/audio/roll-01.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1600.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_ROLL_2] = {
		.path         = "rom:/audio/roll-02.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1600.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},


};
