#ifndef ASSETS_BALL_H
#define ASSETS_BALL_H

#include "entity/entity.h"
#include "scene/scene.h"

#define green_ball_model  "rom:/models/green_sphere.t3dm"
#define yellow_ball_model "rom:/models/yellow_sphere.t3dm"
#define red_ball_model    "rom:/models/red_sphere.t3dm"

extern const EntityColliderDef green_ball_collider;
extern const EntityColliderDef yellow_ball_collider;
extern const EntityColliderDef red_ball_collider;

extern const RigidBodyDef ball_body;

extern const Asset green_ball;
extern const Asset yellow_ball;
extern const Asset red_ball;

#endif
