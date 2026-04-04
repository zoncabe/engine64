#include "../../include/player/player.h"
#include "../../include/actor/actor_states.h"
#include "../../include/control/player_control.h"
#include "../../include/viewport/viewport.h"
#include "../../include/game/game.h"
#include "../../include/ui/menu.h"


void player_controlIntro(Player *player, Game *game) {}

void playerControl_setJump(Player *player)
{
    if (player->control.pressed.a && player->entity->state.current != ROLLING && player->entity->state.current != JUMPING && player->entity->state.current != FALLING){

        player->entity->motion->input.jump_hold = true;
        player->entity->motion->input.jump_triggered = true;

        actor_setState(&player->entity->state, JUMPING);
    }

    else if (player->control.held.a) return;

    else {
        player->entity->motion->input.jump_hold = false;
    }
}

void playerControl_setRoll(Player *player)
{
    if (player->control.pressed.b
        && player->entity->state.current != ROLLING
        && player->entity->state.current != STANDING_IDLE
        && player->entity->state.current != WALKING
        && player->entity->state.current != JUMPING
        && player->entity->state.current != FALLING){

        actor_setState(&player->entity->state, ROLLING);
    }
}

void playerControl_setLocomotionWithStick(Player *player, float camera_angle_around, float camera_offset_angle)
{
    float stick_magnitude = 0;

    if (fabs(player->control.input.stick_x) >= PLAYER_STICK_DEADZONE || fabs(player->control.input.stick_y) >= PLAYER_STICK_DEADZONE) {

        Vector2 stick = {player->control.input.stick_x, player->control.input.stick_y};

        stick_magnitude = vector2_magnitude(&stick);

        float target_yaw = deg(atan2(player->control.input.stick_x, -player->control.input.stick_y) - rad(camera_angle_around - (0.5 * camera_offset_angle)));

        player->entity->motion->input.target_yaw = target_yaw;
    }


    if (stick_magnitude == 0 && player->entity->state.current != ROLLING && player->entity->state.current != JUMPING && player->entity->state.current != FALLING){
        actor_setState(&player->entity->state, STANDING_IDLE);
    }

    else if (stick_magnitude > 0 && stick_magnitude <= PLAYER_STICK_WALK_THRESHOLD && player->entity->state.current != ROLLING && player->entity->state.current != JUMPING && player->entity->state.current != FALLING){
        actor_setState(&player->entity->state, WALKING);
    }

    else if (player->control.held.r && stick_magnitude > PLAYER_STICK_WALK_THRESHOLD && player->entity->state.current != ROLLING && player->entity->state.current != JUMPING && player->entity->state.current != FALLING){
        actor_setState(&player->entity->state, SPRINTING);
    }

    else if (stick_magnitude > PLAYER_STICK_WALK_THRESHOLD && player->entity->state.current != ROLLING && player->entity->state.current != JUMPING && player->entity->state.current != FALLING){
        actor_setState(&player->entity->state, RUNNING);
    }
}

void player_setActorControl(Player *player, Viewport *viewport)
{
    playerControl_setRoll(player);
    playerControl_setJump(player);
    playerControl_setLocomotionWithStick(player, viewport->camera.angle_around_barycenter, viewport->camera.offset_angle);
}

void playerControl_setPauseState(Player *player, Game *game)
{
    if (player->control.pressed.start) game_setState(game, PAUSE);
}

void player_controlPause(Player *player, Game *game)
{
    if (player->control.pressed.start || player->control.pressed.b || (player->control.pressed.a && menu_getIndex() == 0)){
        game_setState(game, GAMEPLAY);
        menu_setIndex(0);
        player->control.pressed = (joypad_buttons_t){0};
    }

    if (player->control.pressed.a && menu_getIndex() == 2){
        game_setState(game, MAIN_MENU);
        menu_setIndex(0);
    }

    if (player->control.pressed.d_up) menu_moveIndex(-1, 2);
    if (player->control.pressed.d_down) menu_moveIndex(1, 2);
}

void player_controlMainMenu(Player *player, Game *game)
{
    if (player->control.pressed.a && menu_getIndex() == 0) game_setState(game, GAMEPLAY);
    if (player->control.pressed.d_up) menu_moveIndex(-1, 2);
    if (player->control.pressed.d_down) menu_moveIndex(1, 2);
}

void playerControl_handleGameOverMenu(Player *player, Game *game) {}

void player_controlGameOver(Player *player, Game *game) {}

void player_controlGameplay(Player *player, Game *game)
{
    playerControl_setPauseState(player, game);
}

static void (*playerControl_handler[])(Player *, Game *) = {

    [INTRO]      = player_controlIntro,
    [MAIN_MENU]  = player_controlMainMenu,
    [GAMEPLAY]   = player_controlGameplay,
    [PAUSE]      = player_controlPause,
    [GAME_OVER]  = player_controlGameOver,
};

static void player_controlGameState(Player *player, Game *game)
{
    playerControl_handler[game->state](player, game);
}


void player_setControllerData(Player **players, Game *game)
{
    joypad_poll();

    for (int i = 0; i < PLAYER_COUNT; i++) {
        controllerData_getInputs(&players[i]->control, i);
        player_controlGameState(players[i], game);
    }
}
