#include <libdragon.h>
#include <t3d/t3d.h>

#include "../../include/entity/entity.h"
#include "../../include/actor/actor_motion.h"
#include "../../include/viewport/viewport.h"
#include "../../include/player/player.h"
#include "../../include/player/player_animation.h"


static Player *player[PLAYER_COUNT];

Player **player_get(void) { return player; }

Player *player_create(const char *model_path, const ActorMotionSettings *motion_settings, const ActorAnimationSettings *animation_settings)
{
    Player *p = malloc(sizeof(Player));
    *p = (Player){};

    p->entity = entity_create(ENTITY_ACTOR, model_path, motion_settings, animation_settings);
    t3d_matrix_set(p->entity->mesh->matrix_buffer, true);

    playerAnimation_init(p);

    return p;
}

void player_destroy(Player *p)
{
    entity_delete(p->entity);
    free(p);
}


void player_update(uint8_t fb_index)
{
    for (uint8_t i = 0; i < PLAYER_COUNT; i++) {
        actorMotion_update(player[i]->entity);
        renderMesh_buildMatrix(player[i]->entity->mesh, &player[i]->entity->transform, fb_index);
        player_setAnimation(player[i]);
    }
}
