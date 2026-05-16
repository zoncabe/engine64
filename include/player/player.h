#ifndef PLAYER_H
#define PLAYER_H

#include "entity/entity.h"
#include "actor/actor_motion.h"

#define PLAYER_COUNT 1


typedef struct {

	float hp;
	float stamina;

} PlayerStats;

typedef struct Player {

	Entity *entity;
	PlayerStats stats;
	MotionCommand cmd;

} Player;


Player *player_get(void);
void player_init(void);
void player_setEntity(Player *player, Entity *entity);
void player_update(void);
void player_setMatrix(uint8_t fb_index);

#endif
