#include <libdragon.h>
#include <t3d/t3d.h>

#include "../../include/entity/entity.h"
#include "../../include/player/player.h"
#include "../../include/player/player_animation.h"
#include "../../include/memory/memory.h"


Player *player_create(const char *model_path, const ActorMotionSettings *motion_settings, const ActorAnimationSettings *animation_settings)
{
    Player *player = malloc(sizeof(Player));
    *player = (Player){};

    player->entity = entity_create(ENTITY_ACTOR, model_path, motion_settings, animation_settings);
    t3d_matrix_set(player->entity->mesh->matrix_buffer, true);

    playerAnimation_init(player);

    return player;
}
