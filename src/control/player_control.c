#include <t3d/t3danim.h>

#include "../../include/physics/physics.h"
#include "../../include/control/control.h"
#include "../../include/actor/actor.h"
#include "../../include/actor/actor_states.h"
#include "../../include/player/player.h"
#include "../../include/control/player_control.h"
#include "../../include/player/player_animation.h"
#include "../../include/physics/physics.h"
#include "../../include/control/control.h"
#include "../../include/light/lighting.h"
#include "../../include/camera/camera.h"
#include "../../include/viewport/viewport.h"
#include "../../include/time/time.h"
#include "../../include/scene/scenery.h"
#include "../../include/ui/ui.h"
#include "../../include/ui/menu.h"
#include "../../include/game/game.h"
#include "../../include/control/camera_control.h"
#include "../../include/game/game_states.h"
#include "../../include/render/render.h"


void playerControl_setJump(Player* player)
{    
    if (player->control.pressed.a && player->actor.state.current != ROLLING && player->actor.state.current != JUMPING && player->actor.state.current != FALLING){
        
        player->actor.motion.input.jump_hold = true;
        
        player->actor.motion.data.jump_initial_velocity = player->actor.body.velocity;

        actor_setState(&player->actor.state, JUMPING);
    }
    
    else if (player->control.held.a) return;
    
    else {
        player->actor.motion.input.jump_hold = false;
    }
}

void playerControl_setRoll(Player* player)
{
    if (player->control.pressed.b 
        && player->actor.state.current != ROLLING
        && player->actor.state.current != STANDING_IDLE
        && player->actor.state.current != WALKING
        && player->actor.state.current != JUMPING
        && player->actor.state.current != FALLING){

        actor_setState(&player->actor.state, ROLLING);
    }
}

void playerControl_setLocomotionWithStick(Player* player, float camera_angle_around, float camera_offset_angle)
{
    int deadzone = 6;
    float stick_magnitude = 0; 

    if (fabs(player->control.input.stick_x) >= deadzone || fabs(player->control.input.stick_y) >= deadzone) {

        Vector2 stick = {player->control.input.stick_x, player->control.input.stick_y};
        
        stick_magnitude = vector2_magnitude(&stick);
        
        float target_yaw = deg(atan2(player->control.input.stick_x, -player->control.input.stick_y) - rad(camera_angle_around - (0.5 * camera_offset_angle)));
        float target_speed = stick_magnitude * 5;

        player->actor.motion.data.target_yaw = target_yaw;

        player->actor.motion.data.horizontal_target_speed = target_speed;
    }

    
    if (stick_magnitude == 0 && player->actor.state.current != ROLLING && player->actor.state.current != JUMPING && player->actor.state.current != FALLING){
        actor_setState(&player->actor.state, STANDING_IDLE);
    }

    else if (stick_magnitude > 0 && stick_magnitude <= 65 && player->actor.state.current != ROLLING && player->actor.state.current != JUMPING && player->actor.state.current != FALLING){
        actor_setState(&player->actor.state, WALKING);
    }

    else if (player->control.held.r && stick_magnitude > 65 && player->actor.state.current != ROLLING && player->actor.state.current != JUMPING && player->actor.state.current != FALLING){
        actor_setState(&player->actor.state, SPRINTING);
    }

    else if (stick_magnitude > 65 && player->actor.state.current != ROLLING && player->actor.state.current != JUMPING && player->actor.state.current != FALLING){
        actor_setState(&player->actor.state, RUNNING);
    }
}

void player_setActorControl(Player* player)
{
    playerControl_setRoll(player);
    playerControl_setJump(player);
    playerControl_setLocomotionWithStick(player, viewport.camera.angle_around_barycenter, viewport.camera.offset_angle);
}

void playerControl_setPauseState(Player* player)
{
    if (player->control.pressed.start) game_setState(PAUSE);
}

void player_controlPauseState(Player* player)
{
    if (player->control.pressed.start || player->control.pressed.b || (player->control.pressed.a && menu.index == 0)){
        
        game_setState(GAMEPLAY);
        menu.index = 0;
    }

    if (player->control.pressed.a && menu.index == 2){
    
        game_setState(MAIN_MENU);
        menu.index = 0;
    }

    if (player->control.pressed.d_up) --menu.index;
    if (player->control.pressed.d_down) ++menu.index;
    if (menu.index < 0) menu.index = 2;
    if (menu.index > 2) menu.index = 0;
}

void player_controlMainMenu(Player* player)
{
    if ((player->control.pressed.a && menu.index == 0)) game_setState(GAMEPLAY);
    if (player->control.pressed.d_up) --menu.index;
    if (player->control.pressed.d_down) ++menu.index;
    if (menu.index < 0) menu.index = 2;
    if (menu.index > 2) menu.index = 0;
}

void playerControl_handleGameOverMenu(Player* player)
{
}

void player_controlGameState(Player* player)
{
    switch(game.state){

		case INTRO:{
			break;
		}
		case MAIN_MENU:{
            player_controlMainMenu(player);
			break;
		}
		case GAMEPLAY:{
            playerControl_setPauseState(player);
            player_setActorControl(player);
			break;
		}
		case PAUSE:{
            player_controlPauseState(player);
			break;
		}
		case GAME_OVER:{
			break;
		}
	}
}


void player_setControllerData()
{
    joypad_poll();
    
    for (int i = 0; i < PLAYER_COUNT; i++) {
        
        controllerData_getInputs(&player[i]->control, i);
        player_controlGameState(player[i]);
    } 
}