#ifndef ACTOR_H
#define ACTOR_H

#include <stdbool.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>

#include "physics/physics.h"
#include "graphics/mesh.h"
#include "actor/actor_motion.h"
#include "actor/actor_animation.h"
#include "actor/actor_states.h"

typedef struct Actor {

	ActorMotion     motion;
	ActorAnimation  animation;
	ActorStateData  state;
	const AnimationDef  *animation_def;

} Actor;

typedef struct ActorDef {

	const ActorMotionSettings *motion_settings;
	const ActorAnimationSettings *animation_settings;
	const AnimationDef *animation_def;

} ActorDef;


#endif
