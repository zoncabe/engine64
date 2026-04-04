#ifndef TIME_H
#define TIME_H


// structures

typedef struct
{
    float counter;
    float delta;
    float rate;

    float intro_counter;
    float transition_counter;

} TimeData;

TimeData* time_get(void);


// functions prototypes

void time_init();
void time_update();


#endif