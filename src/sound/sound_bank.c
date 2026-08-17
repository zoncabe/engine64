#include "sound/sound.h"
#include "sound/sound_bank.h"


const SoundDef sound_bank[SOUND_COUNT] = {

	[SOUND_FLAG_FLAPPING] = {
		.path         = "rom:/audio/80079__benboncan__cotton-flapping.wav64",
		.volume       = 0.9f,
		.min_distance = 600.0f,
		.max_distance = 3000.0f,
		.loop         = true,
		.priority     = SOUND_PRIORITY_AMBIENCE,
		.preload      = false,
	},

	[SOUND_FOOTSTEP_WOOD_1] = {
		.path         = "rom:/audio/458326__dwoboyle__footsteps-tile-run-01.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_FOOTSTEP_WOOD_2] = {
		.path         = "rom:/audio/458361__dwoboyle__footsteps-tile-run-09.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_FOOTSTEP_WOOD_3] = {
		.path         = "rom:/audio/458365__dwoboyle__footsteps-tile-run-10.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_FOOTSTEP_WOOD_4] = {
		.path         = "rom:/audio/458366__dwoboyle__footsteps-tile-run-13.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_FOOTSTEP_WOOD_5] = {
		.path         = "rom:/audio/458335__dwoboyle__footsteps-tile-run-24.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_FOOTSTEP_WOOD_6] = {
		.path         = "rom:/audio/458336__dwoboyle__footsteps-tile-run-25.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1400.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_DODGE_ROLL_1] = {
		.path         = "rom:/audio/458355__dwoboyle__footsteps-tile-light-13.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1600.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},

	[SOUND_DODGE_ROLL_2] = {
		.path         = "rom:/audio/458337__dwoboyle__footsteps-tile-light-14.wav64",
		.volume       = 1.0f,
		.min_distance = 80.0f,
		.max_distance = 1600.0f,
		.loop         = false,
		.priority     = SOUND_PRIORITY_ONESHOT,
		.preload      = false,
	},


};
