#include "../../include/player/player.h"
#include "../../include/actor/actor_states.h"
#include "../../include/control/player_control.h"
#include "../../include/control/controller.h"
#include "../../include/viewport/viewport.h"
#include "../../include/game/game.h"
#include "../../include/ui/menu.h"


static void playerControl_setJump(Player *player, const ControllerActions *actions)
{
    if (actions->jump
        && player->entity->actor->state.current != ROLLING
        && player->entity->actor->state.current != JUMPING
        && player->entity->actor->state.current != FALLING) {

        player->entity->actor->motion.input.jump_hold      = true;
        player->entity->actor->motion.input.jump_triggered = true;
        actor_setState(&player->entity->actor->state, JUMPING);
    } else if (actions->jump_hold) {
        return;
    } else {
        player->entity->actor->motion.input.jump_hold = false;
    }
}

static void playerControl_setRoll(Player *player, const ControllerActions *actions)
{
    if (actions->roll
        && player->entity->actor->state.current != ROLLING
        && player->entity->actor->state.current != STANDING_IDLE
        && player->entity->actor->state.current != WALKING
        && player->entity->actor->state.current != JUMPING
        && player->entity->actor->state.current != FALLING) {

        actor_setState(&player->entity->actor->state, ROLLING);
    }
}

static void playerControl_setLocomotionWithStick(Player *player, const ControllerActions *actions, float camera_angle_around, float camera_offset_angle)
{
    float stick_magnitude = 0;

    if (fabs(actions->stick_x) >= PLAYER_STICK_DEADZONE || fabs(actions->stick_y) >= PLAYER_STICK_DEADZONE) {
        Vector2 stick   = {actions->stick_x, actions->stick_y};
        stick_magnitude = vector2_magnitude(&stick);
        player->entity->actor->motion.input.target_yaw = deg(atan2(actions->stick_x, -actions->stick_y) - rad(camera_angle_around - (0.5 * camera_offset_angle)));
    }

    if (stick_magnitude == 0
        && player->entity->actor->state.current != ROLLING
        && player->entity->actor->state.current != JUMPING
        && player->entity->actor->state.current != FALLING) {
        actor_setState(&player->entity->actor->state, STANDING_IDLE);
    } else if (stick_magnitude > 0 && stick_magnitude <= PLAYER_STICK_WALK_THRESHOLD
        && player->entity->actor->state.current != ROLLING
        && player->entity->actor->state.current != JUMPING
        && player->entity->actor->state.current != FALLING) {
        actor_setState(&player->entity->actor->state, WALKING);
    } else if (actions->sprint && stick_magnitude > PLAYER_STICK_WALK_THRESHOLD
        && player->entity->actor->state.current != ROLLING
        && player->entity->actor->state.current != JUMPING
        && player->entity->actor->state.current != FALLING) {
        actor_setState(&player->entity->actor->state, SPRINTING);
    } else if (stick_magnitude > PLAYER_STICK_WALK_THRESHOLD
        && player->entity->actor->state.current != ROLLING
        && player->entity->actor->state.current != JUMPING
        && player->entity->actor->state.current != FALLING) {
        actor_setState(&player->entity->actor->state, RUNNING);
    }
}

void player_setActorControl(Player *player, const ControllerActions *actions, Viewport *viewport)
{
    playerControl_setRoll(player, actions);
    playerControl_setJump(player, actions);
    playerControl_setLocomotionWithStick(player, actions, viewport->camera.angle_around_barycenter, viewport->camera.offset_angle);
}


static void playerControl_handleIntro(Player *player, const ControllerActions *actions, Game *game) { (void)player; (void)actions; (void)game; }

static void playerControl_handleMainMenu(Player *player, const ControllerActions *actions, Game *game)
{
    (void)player;
    if (actions->confirm && menu_getIndex() == 0) game_setState(game, GAMEPLAY);
    if (actions->menu_up)   menu_moveIndex(-1, 2);
    if (actions->menu_down) menu_moveIndex(1,  2);
}

static void playerControl_handleGameplay(Player *player, const ControllerActions *actions, Game *game)
{
    (void)player;
    if (actions->pause) game_setState(game, PAUSE);
}

static void playerControl_handlePause(Player *player, const ControllerActions *actions, Game *game)
{
    (void)player;
    if (actions->pause || actions->cancel || (actions->confirm && menu_getIndex() == 0)) {
        game_setState(game, GAMEPLAY);
        menu_setIndex(0);
        return;
    }
    if (actions->confirm && menu_getIndex() == 2) {
        game_setState(game, MAIN_MENU);
        menu_setIndex(0);
    }
    if (actions->menu_up)   menu_moveIndex(-1, 2);
    if (actions->menu_down) menu_moveIndex(1,  2);
}

static void playerControl_handleGameOver(Player *player, const ControllerActions *actions, Game *game) { (void)player; (void)actions; (void)game; }

typedef void (*PlayerControlHandler)(Player *, const ControllerActions *, Game *);

static const PlayerControlHandler playerControl_handler[GAME_STATE_COUNT] = {
    [INTRO]      = playerControl_handleIntro,
    [MAIN_MENU]  = playerControl_handleMainMenu,
    [GAMEPLAY]   = playerControl_handleGameplay,
    [PAUSE]      = playerControl_handlePause,
    [GAME_OVER]  = playerControl_handleGameOver,
};


void player_setControllerData(Player **players, Game *game)
{
    controller_poll();
    Controller **ctrl = controller_get();
    for (uint8_t i = 0; i < PLAYER_COUNT; i++)
        playerControl_handler[game->state](players[i], &ctrl[i]->actions, game);
}
