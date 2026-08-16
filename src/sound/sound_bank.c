#include "sound/sound.h"
#include "sound/sound_bank.h"


const SoundDef sound_bank[SOUND_COUNT] = {

	[SOUND_FLAG_FLAPPING] = {
		.path         = "rom:/audio/flag-large-flapping.wav64",
		.volume       = 0.9f,
		.min_distance = 600.0f,
		.max_distance = 3000.0f,
		.loop         = true,
		.priority     = SOUND_PRIORITY_AMBIENCE,
		.preload      = false,
	},

	[SOUND_FOOTSTEP_WOOD_1] = {
		.path         = "rom:/audio/footstep-wood-light01.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = true,
	},

	[SOUND_FOOTSTEP_WOOD_2] = {
		.path         = "rom:/audio/footstep-wood-light02.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = true,
	},

	[SOUND_FOOTSTEP_WOOD_3] = {
		.path         = "rom:/audio/footstep-wood-light03.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = true,
	},

	[SOUND_FOOTSTEP_WOOD_4] = {
		.path         = "rom:/audio/footstep-wood-light04.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = true,
	},

	[SOUND_FOOTSTEP_WOOD_5] = {
		.path         = "rom:/audio/footstep-wood-light05.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = true,
	},

	[SOUND_FOOTSTEP_WOOD_6] = {
		.path         = "rom:/audio/footstep-wood-light06.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = true,
	},

	[SOUND_DODGE_ROLL_1] = {
		.path         = "rom:/audio/dodge-roll.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1600.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = true,
	},

	[SOUND_DODGE_ROLL_2] = {
		.path         = "rom:/audio/dodge-roll-01.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1600.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = true,
	},

	[SOUND_DODGE_ROLL_3] = {
		.path         = "rom:/audio/dodge-roll-02.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1600.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = true,
	},

	[SOUND_DODGE_ROLL_4] = {
		.path         = "rom:/audio/dodge-roll-03.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1600.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = true,
	},
};
