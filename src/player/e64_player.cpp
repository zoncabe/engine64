#include "character3d/e64_character3d.h"
#include "character2d/e64_character2d.h"
#include "player/e64_player.h"
#include "time/e64_time.h"

namespace e64 {

namespace player {

static Player player[PLAYER_COUNT];

Player *get(void) { return player; }

void init(void)
{
	for (int i = 0; i < PLAYER_COUNT; i++)
		player[i] = (Player){};
}

void setCharacter3D(PlayerID id, Character3D *character)
{
	Player *seat = &player[id];

	seat->type = CHARACTER_3D;
	seat->character3d.character = character;
}

void setCharacter2D(PlayerID id, Character2D *character)
{
	Player *seat = &player[id];

	seat->type = CHARACTER_2D;
	seat->character2d.character = character;
}


/* Only the seated bodies move here. A character nobody drives is the game's
   to move, from the update of the entity that carries it. */
void update(void)
{
	const float dt = time::get()->delta;

	/* Seats nobody took: a player without a body has nothing to run. */
	for (int i = 0; i < PLAYER_COUNT; i++) {
		if (player[i].type == CHARACTER_3D && player[i].character3d.character)
			player[i].character3d.character->updateMovement(&player[i].character3d.cmd, dt);

		else if (player[i].type == CHARACTER_2D && player[i].character2d.character)
			player[i].character2d.character->updateMovement(&player[i].character2d.cmd, dt);
	}
}

}

}
