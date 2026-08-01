#include <libdragon.h>
#include <t3d/t3d.h>

#include "viewport/viewport.h"
#include "entity/entity.h"
#include "character/character.h"
#include "character/character_movement.h"
#include "character/character_animation.h"
#include "player/player.h"
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


void player_update(void)
{
	const float dt = time_get()->delta;
	for (int i = 0; i < PLAYER_COUNT; i++) {
		character_updateMovement(player[i].character, &player[i].cmd, dt);
		character_setAnimation(player[i].character);
	}
}

void player_setMatrix(uint8_t fb_index)
{
	for (int i = 0; i < PLAYER_COUNT; i++)
		mesh_setMatrix(player[i].entity->mesh, &player[i].entity->transform, fb_index);
}
