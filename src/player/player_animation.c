#include "../../include/time/time.h"
#include "../../include/actor/actor.h"
#include "../../include/actor/actor_animation.h"
#include "../../include/player/player.h"

void player_setAnimation(Player *player)
{
    Actor *actor = player->entity->actor;
    actorAnimation_setParams(player->entity, actor->anim_def);
    actorAnimation_evaluateGraph(actor->anim_def, &actor->animation.settings, &actor->animation, time_get()->delta);
    t3d_skeleton_update(&actor->animation.main);
}
