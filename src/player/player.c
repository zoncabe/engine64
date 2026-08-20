#include <libdragon.h>
#include <t3d/t3d.h>

#include "viewport/viewport.h"
#include "entity/entity.h"
#include "control/character_control.h"
#include "character/character.h"
#include "character/character_movement.h"
#include "character/character_animation.h"
#include "player/player.h"
#include "physics/math/math_functions.h"
#include "scene/scene.h"
#include "time/time.h"


static Player player[PLAYER_COUNT];

Player *player_get(void) { return player; }

void player_init(void)
{
	for (int i = 0; i < PLAYER_COUNT; i++)
		player[i] = (Player){0};
}

void player_setCharacter(Player *player, Character *character)
{
	player->character = character;
	player->entity = character ? character->entity : NULL;
	if (player->entity && player->entity->mesh)
		t3d_matrix_set(player->entity->mesh->matrix_buffer, true);
}

/* Cycles the player through the scene's characters, in either direction. */
void player_switchCharacter(Player *player, int8_t direction)
{
	Scene *scene = scene_get();
	if (scene->character_count < 2) return;

	uint8_t current = 0;
	for (uint8_t i = 0; i < scene->character_count; i++)
		if (scene->character[i] == player->character) { current = i; break; }

	uint8_t next = (uint8_t)((current + scene->character_count + direction) % scene->character_count);
	Character *character = scene->character[next];

	/* Glide the camera over from the body being left behind instead of cutting
	   to the new one. */
	camera_setViewTarget(
		&viewport_get()->camera,
		&player->entity->transform.position,
		CAMERA_VIEW_TARGET_BLEND_TIME
	);

	player_setCharacter(player, character);

	/* Fresh command, facing where this body already faces: anything held over
	   from the previous character would spin the new one on the spot. */
	player->cmd = (MovementCommand){ .target_yaw = character->body.rotation.z };
}


void player_update(void)
{
	const float dt = time_get()->delta;
	for (int i = 0; i < PLAYER_COUNT; i++) {
		characterStats_update(player[i].character, &player[i].cmd, dt);
		character_updateMovement(player[i].character, &player[i].cmd, dt);
		character_setAnimation(player[i].character);
		characterSound_update(player[i].character);
	}

	/* Scene characters nobody drives run on an empty command, so they idle
	   instead of freezing mid pose when the player switches away. */
	static MovementCommand idle_cmd;
	Scene *scene = scene_get();
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
		static const ControllerActions no_actions;

		idle_cmd.target_yaw = character->body.rotation.z;
		characterControl_update(character, &idle_cmd, &no_actions, 0.0f);
		characterStats_update(character, &idle_cmd, dt);
		character_updateMovement(character, &idle_cmd, dt);
		character_setAnimation(character);
		characterSound_update(character);
	}
}

void player_setMatrix(uint8_t fb_index)
{
	for (int i = 0; i < PLAYER_COUNT; i++)
		mesh_setMatrix(player[i].entity->mesh, &player[i].entity->transform, fb_index);
}
