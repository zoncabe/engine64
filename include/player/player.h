#ifndef PLAYER_H
#define PLAYER_H

#include "entity/entity.h"
#include "character/character_movement.h"
#include "control/character_control.h"


typedef struct Player {

	Character *character;
	Entity *entity;
	MovementCommand cmd;

	/* Which buttons the command above is built from. Written once, with the
	   character. */
	const CharacterControlBinding *control;

} Player;


Player *player_get(void);
void player_init(void);
/* Seats a player: the body it drives and the buttons that drive it, together. */
void player_setCharacter(PlayerID id, Character *character, const CharacterControlBinding *control);
void player_switchCharacter(PlayerID id, int8_t direction);
void player_update(void);
void player_setMatrix(uint8_t fb_index);

#endif
