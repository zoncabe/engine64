#include "assets/brew64_flag.h"

/* The flag hangs from a pole (its own asset); the scene places the pair. */
const ClothDef brew64_flag_cloth = {
	.mesh_path  = "rom:/collision/brew_flag.collision",
	/* Jakobsen's paper: damped Verlet x' = 1.99x - 0.99x* (damping 0.01) and
	   3-4 relaxation passes. */
	.damping    = 0.01f,
	.iterations = 4,
	.pin_max_x  = 0.01f,
};

static const SoundID brew64_flag_flapping[] = { SOUND_FLAG_FLAPPING };

static const AssetSound brew64_flag_sound[] = {
	{ ASSET_SOUND_AMBIENT, brew64_flag_flapping, 1 },
};

const Asset brew64_flag = {
	.type        = ASSET_CLOTH,
	.model       = brew64_flag_model,
	.sound       = brew64_flag_sound,
	.sound_count = 1,
	.cloth       = { &brew64_flag_cloth },
};
