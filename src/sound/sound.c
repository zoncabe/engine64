#include <libdragon.h>
#include <t3d/t3danim.h>

#include "../../include/sound/sound.h"
#include "../../include/time/time.h"
#include "../../include/game/game.h"
#include "../../include/game/game_states.h"
#include "../../include/physics/physics.h"
#include "../../include/actor/actor.h"
#include "../../include/player/player.h"
#include "../../include/player/player_animation.h"

#define NUM_CHANNELS 16

void sound_init()
{
	audio_init(48000, 4);
	mixer_init(NUM_CHANNELS);
	wav64_init_compression(3);	
}

void sound_play()
{
	mixer_try_play();
}

void sound_setMixer()
{	
}
