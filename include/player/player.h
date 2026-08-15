#ifndef PLAYER_H
#define PLAYER_H

#include "entity/entity.h"
#include "character/character_movement.h"

#define PLAYER_COUNT 1


typedef struct {

	float hp;
	float stamina;
	bool tired;

} PlayerStats;

typedef struct Player {

	Character *character;
	Entity *entity;
	PlayerStats stats;
	MovementCommand cmd;

} Player;


Player *player_get(void);
void player_init(void);
void player_setCharacter(Player *player, Character *character);
void player_switchCharacter(Player *player, int8_t direction);
void player_update(void);
void player_setMatrix(uint8_t fb_index);

#endif
