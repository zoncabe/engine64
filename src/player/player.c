#include <libdragon.h>
#include <t3d/t3d.h>

#include "viewport/viewport.h"
#include "entity/entity.h"
#include "actor/actor.h"
#include "actor/actor_motion.h"
#include "actor/actor_animation.h"
#include "player/player.h"
#include "time/time.h"


static Player player[PLAYER_COUNT];

Player *player_get(void) { return player; }

void player_init(void)
{
	for (int i = 0; i < PLAYER_COUNT; i++)
		player[i] = (Player){0};
}

void player_setEntity(Player *player, Entity *entity)
{
	player->entity = entity;
	if (entity && entity->mesh)
		t3d_matrix_set(entity->mesh->matrix_buffer, true);
}


void player_update(void)
{
	const float dt = time_get()->delta;
	for (int i = 0; i < PLAYER_COUNT; i++) {
		actor_updateMotion(player[i].entity, &player[i].cmd, dt);
		actor_setAnimation(player[i].entity);
	}
}

void player_setMatrix(uint8_t fb_index)
{
	for (int i = 0; i < PLAYER_COUNT; i++)
		mesh_setMatrix(player[i].entity->mesh, &player[i].entity->transform, fb_index);
}
