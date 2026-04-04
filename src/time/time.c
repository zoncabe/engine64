#include <libdragon.h>

#include "../../include/time/time.h"


static TimeData timer;

TimeData* time_get(void) { return &timer; }


// functions implementations

/* sets time data values to zero */
void time_init()
{
    timer.counter = 1.0f;
    timer.delta = 0.0f;
    timer.rate = 0.0f;

    timer.intro_counter = 0.0f;
    timer.transition_counter = 0.0f;
}


/* sets timing data */
void time_update()
{
    timer.delta = display_get_delta_time();
    timer.counter += timer.delta;
    timer.rate = display_get_fps();
}