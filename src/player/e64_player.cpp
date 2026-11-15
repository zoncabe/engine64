#include "character3d/e64_character3d.h"
#include "character2d/e64_character2d.h"
#include "player/e64_player.h"
#include "scene3d/e64_scene3d.h"
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

/* Cycles the player through the scene's characters, in either direction. */
void switchCharacter3D(PlayerID id, int8_t direction)
{
	Scene3D *scene = scene3d::get();
	if (scene->character3d_count < 2) return;

	Player *seat = &player[id];

	/* Only a seat already driving a 3D body switches between them. */
	if (seat->type != CHARACTER_3D) return;

	uint8_t current = 0;
	for (uint8_t i = 0; i < scene->character3d_count; i++)
		if (scene->character[i] == seat->character3d.character) { current = i; break; }

	uint8_t next = (uint8_t)((current + scene->character3d_count + direction) % scene->character3d_count);

	setCharacter3D(id, scene->character[next]);

	/* Fresh command: anything held over from the previous character would
	   carry into the new one. */
	seat->character3d.cmd = (character3d::MovementCommand){};
}


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

	/* Scene3D characters nobody drives move on an empty command: they brake,
	   fall and land like any body, instead of freezing in place when the
	   player switches away. */
	Scene3D *scene = scene3d::get();
	for (int i = 0; i < scene->character3d_count; i++) {
		Character3D *character = scene->character[i];

		bool driven = false;
		for (int p = 0; p < PLAYER_COUNT; p++)
			if (player[p].type == CHARACTER_3D && player[p].character3d.character == character) driven = true;
		if (driven) continue;

		character3d::MovementCommand idle_cmd = {};
		character->updateMovement(&idle_cmd, dt);
	}
}

}

}
