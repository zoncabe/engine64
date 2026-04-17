#include <libdragon.h>

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
