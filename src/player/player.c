#include <libdragon.h>
#include <t3d/t3d.h>

#include "../../include/viewport/viewport.h"
#include "../../include/entity/entity.h"
#include "../../include/actor/actor.h"
#include "../../include/actor/actor_motion.h"
#include "../../include/actor/actor_animation.h"
#include "../../include/player/player.h"
#include "../../include/time/time.h"


static Player *player[PLAYER_COUNT];

Player **player_get(void) { return player; }

Player *player_create(const char *model_path, const ActorMotionSettings *motion_settings, const ActorAnimationSettings *animation_settings, const AnimDef *anim_def)
{
    Player *p = malloc(sizeof(Player));
    *p = (Player){};

    p->entity = entity_create(ENTITY_ACTOR, model_path, motion_settings, animation_settings, anim_def);
    t3d_matrix_set(p->entity->mesh->matrix_buffer, true);

    return p;
}

void player_destroy(Player *p)
{
    for (uint8_t i = 0; i < PLAYER_COUNT; i++)
        if (player[i] == p) { player[i] = NULL; break; }
    entity_delete(p->entity);
    free(p);
}


void player_update(uint8_t fb_index)
{
    const float dt = time_get()->delta;
    for (uint8_t i = 0; i < PLAYER_COUNT; i++) {
        actor_updateMotion(player[i]->entity, &player[i]->cmd, dt);
        mesh_setMatrix(player[i]->entity->mesh, &player[i]->entity->transform, fb_index);
        actor_setAnimation(player[i]->entity);
    }
}
