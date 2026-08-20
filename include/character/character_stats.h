/*
	The body's own condition: hp, stamina, and whatever it earns later.

	The stats belong to the character, not to the player driving it —
	switching bodies leaves the fatigue with the one that earned it, and a
	body left behind recovers on its own idle command.
*/
#ifndef CHARACTER_STATS_H
#define CHARACTER_STATS_H

#include <stdbool.h>

#include "character/character_movement.h"


typedef struct Character Character;


/* Per asset tuning. Stamina is normalized 0..1 and the rates are per
   second; tired caps the reachable speed at this fraction of the top
   gait, through the command's speed scale. */
typedef struct {

	float stamina_drain_rate;
	float stamina_regen_rate;
	float tired_speed_scale;

} CharacterStatsSettings;

typedef struct {

	const CharacterStatsSettings *settings;

	float hp;
	float stamina;
	bool  tired;

} CharacterStats;


/* Runs before the movement update: the speed scale it writes into the
   command is what the movement consumes on the same frame. */
void characterStats_update(Character *character, MovementCommand *cmd, float dt);

#endif
