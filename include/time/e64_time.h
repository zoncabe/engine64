#ifndef ENGINE64_TIME_H
#define ENGINE64_TIME_H


typedef struct
{
	float counter;
	float delta;
	float rate;

} TimeData;


TimeData* time_get(void);

void time_init();
void time_update();
void time_setScale(float scale);

/* After a blocking load: drops the time it took, so the next frame's delta
   is a normal one instead of the whole load measured as gameplay. */
void time_reset();


#endif