#ifndef CHARACTER_CONTROL_H
#define CHARACTER_CONTROL_H

#include "controller.h"
#include "character/character.h"
#include "character/character_movement.h"

#define PLAYER_STICK_DEADZONE       6
#define PLAYER_STICK_WALK_THRESHOLD 65

void characterControl_update(Character *character, MovementCommand *cmd, const ControllerActions *actions, float camera_angle_around);

#endif
