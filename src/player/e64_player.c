#include <libdragon.h>
#include <t3d/t3d.h>

#include "viewport/e64_viewport.h"
#include "entity/e64_entity.h"
#include "control/e64_character_control.h"
#include "character/e64_character.h"
#include "character/e64_character_movement.h"
#include "character/e64_character_animation.h"
#include "player/e64_player.h"
#include "physics/math/e64_math_functions.h"
#include "scene3d/e64_scene3d.h"
#include "time/e64_time.h"


static Player player[PLAYER_COUNT];

Player *player_get(void) { return player; }

void player_init(void)
{
	for (int i = 0; i < PLAYER_COUNT; i++)
		player[i] = (Player){0};
}

/* The seat comes from the binding: the buttons already name whose they are. */
void player_setCharacter(Character *character, const CharacterControlBinding *control)
{
	if (control == NULL) return;

	Player *seat = &player[control->player];

	seat->control   = control;
	seat->character = character;
	seat->entity = character ? character->entity : NULL;
	if (seat->entity && seat->entity->mesh)
		t3d_matrix_set(seat->entity->mesh->matrix_buffer, true);
}

/* Cycles the player through the scene's characters, in either direction. */
void player_switchCharacter(PlayerID id, int8_t direction)
{
	Scene3D *scene = scene3d_get();
	if (scene->character_count < 2) return;

	Player *seat = &player[id];

	uint8_t current = 0;
	for (uint8_t i = 0; i < scene->character_count; i++)
		if (scene->character[i] == seat->character) { current = i; break; }

	uint8_t next = (uint8_t)((current + scene->character_count + direction) % scene->character_count);
	Character *character = scene->character[next];

	/* Same buttons on the new body: switching bodies is not re-binding. */
	player_setCharacter(character, seat->control);

	/* Fresh command, facing where this body already faces: anything held over
	   from the previous character would spin the new one on the spot. */
	seat->cmd = (MovementCommand){ .target_yaw = character->body.rotation.z };
}


void player_update(void)
{
	const float dt = time_get()->delta;
	for (int i = 0; i < PLAYER_COUNT; i++) {
		/* Seats nobody took: a player without a body has nothing to run. */
		if (player[i].character == NULL) continue;

		characterStats_update(player[i].character, &player[i].cmd, dt);
		character_updateMovement(player[i].character, &player[i].cmd, dt);
		character_setAnimation(player[i].character);
		characterSound_update(player[i].character);
	}

	/* Scene3D characters nobody drives run on an empty command, so they idle
	   instead of freezing mid pose when the player switches away. */
	static MovementCommand idle_cmd;
	Scene3D *scene = scene3d_get();
	for (int i = 0; i < scene->character_count; i++) {
		Character *character = scene->character[i];

		bool driven = false;
		for (int p = 0; p < PLAYER_COUNT; p++)
			if (player[p].character == character) driven = true;
		if (driven) continue;

		/* Same pipeline as a driven body, on a controller nobody holds: the
		   released stick idles it through the control's own rule (treading
		   water if it was swimming), and idling never drains, so a body
		   left behind rests and refills on its own. */
		static const CharacterControls no_controls;

		idle_cmd.target_yaw = character->body.rotation.z;
		characterControl_update(character, &idle_cmd, &no_controls, 0.0f);
		characterStats_update(character, &idle_cmd, dt);
		character_updateMovement(character, &idle_cmd, dt);
		character_setAnimation(character);
		characterSound_update(character);
	}
}

void player_setMatrix(uint8_t fb_index)
{
	for (int i = 0; i < PLAYER_COUNT; i++)
		if (player[i].entity)
			mesh_setMatrix(player[i].entity->mesh, &player[i].entity->transform, fb_index);
}
