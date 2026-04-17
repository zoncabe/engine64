#include <libdragon.h>

#include "time/time.h"


static TimeData timer;
static float time_scale = 1.0f;


TimeData* time_get(void) { return &timer; }

void time_setScale(float scale) { time_scale = scale; }

void time_init()
{
	timer.counter = 1.0f;
	timer.delta = 0.0f;
	timer.rate = 0.0f;

}

void time_update()
{
	timer.delta = display_get_delta_time() * time_scale;
	timer.counter += timer.delta;
	timer.rate = display_get_fps();
}