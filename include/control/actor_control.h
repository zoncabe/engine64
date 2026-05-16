#ifndef ACTOR_CONTROL_H
#define ACTOR_CONTROL_H

#include "controller.h"
#include "actor/actor.h"
#include "actor/actor_motion.h"

#define PLAYER_STICK_DEADZONE       6
#define PLAYER_STICK_WALK_THRESHOLD 65

void actorControl_update(Actor *actor, MotionCommand *cmd, const ControllerActions *actions, float camera_angle_around);

#endif
