#include <t3d/t3danim.h>

#include "../../include/time/time.h"
#include "../../include/physics/physics.h"
#include "../../include/actor/actor.h"
#include "../../include/actor/actor_states.h"
#include "../../include/actor/actor_animation.h"
#include "../../include/player/player.h"
#include "../../include/light/lighting.h"
#include "../../include/camera/camera.h"
#include "../../include/viewport/viewport.h"


void playerAnimation_init(Player *player)
{
    actorAnimation_initStandingLocomotionSet(player->entity);
    actorAnimation_initJumpSet(player->entity);
    actorAnimation_initRollSet(player->entity);
}

void player_setAnimation(Player *player)
{
    actorAnimation_set(player->entity);
}
