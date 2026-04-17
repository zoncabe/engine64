#ifndef TIME_H
#define TIME_H


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


#endif