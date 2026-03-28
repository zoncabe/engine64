#ifndef PLAYER_H
#define PLAYER_H

#define PLAYER_COUNT 1

typedef struct {

    float hp;
    float stamina;

} PlayerStats;

typedef struct {

    ControllerData control;

    Actor actor;

    PlayerStats stats;
	
} Player;


extern Player* player[PLAYER_COUNT];

void player_init(Player* player, ActorMotionSettings* motion_settings, ActorAnimationSettings* animation_settings);

void player_update();


#endif