#include <libdragon.h>

#include "time/time.h"


static TimeData timer;
static float time_scale = 1.0f;
static uint32_t last_ticks;


TimeData* time_get(void) { return &timer; }

void time_setScale(float scale) { time_scale = scale; }

void time_init()
{
	timer.counter = 1.0f;
	timer.delta = 0.0f;
	timer.rate = 0.0f;

	last_ticks = TICKS_READ();
}

void time_reset()
{
	last_ticks = TICKS_READ();
}

void time_update()
{
	uint32_t now = TICKS_READ();
	timer.delta = (float)TICKS_DISTANCE(last_ticks, now) / TICKS_PER_SECOND * time_scale;
	last_ticks = now;

	timer.counter += timer.delta;
	timer.rate = display_get_fps();
}