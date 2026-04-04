#ifndef SOUND_H
#define SOUND_H

#include <libdragon.h>

#define SFX_CHANNEL_1 0
#define SFX_CHANNEL_2 3
#define MUSIC_CHANNEL_1 6
#define MUSIC_CHANNEL_2 9

void sound_init();
void sound_setMixer();
void sound_play();

#endif