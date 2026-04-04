#include "../../include/actor/actor_motion.h"
#include "../../include/viewport/viewport.h"
#include "../../include/player/player.h"
#include "../../include/player/player_animation.h"


static Player *player[PLAYER_COUNT];

Player **player_get(void) { return player; }


void player_update(uint8_t fb_index)
{
    for (uint8_t i = 0; i < PLAYER_COUNT; i++) {
        actorMotion_update(player[i]->entity);
        renderMesh_buildMatrix(player[i]->entity->mesh, &player[i]->entity->transform, fb_index);
        player_setAnimation(player[i]);
    }
}
