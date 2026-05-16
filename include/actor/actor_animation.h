#ifndef ACTOR_ANIMATION_H
#define ACTOR_ANIMATION_H

#include <stdbool.h>
#include <stdint.h>
#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>


typedef struct Entity Entity;
typedef struct Actor Actor;
typedef struct ActorAnimation ActorAnimation;

#define LAND_ANIMATION_STARTING_HEIGHT 130
#define ANIMATION_MAX_LAYERS 16

#define ANIMATION_SLOT_MAIN          0xFF
#define ANIMATION_TURN_AVG_COUNT  5
#define ANIMATION_TURN_MAX_ANGLE  5.0f

typedef enum {

	ANIMATION_PARAM_WALK,
	ANIMATION_PARAM_RUN,
	ANIMATION_PARAM_SPRINT,
	ANIMATION_PARAM_IDLE_RIGHT,
	ANIMATION_PARAM_TURNING,
	ANIMATION_PARAM_JUMP_L,
	ANIMATION_PARAM_JUMP_R,
	ANIMATION_PARAM_LAND_L,
	ANIMATION_PARAM_LAND_R,
	ANIMATION_PARAM_ROLL_RUN,
	ANIMATION_PARAM_ROLL_STAND,
	ANIMATION_PARAM_ROLL_DIR,
	ANIMATION_PARAM_STAND_ROLL_DIR,
	ANIMATION_PARAM_TURN_WALK,
	ANIMATION_PARAM_TURN_RUN,
	ANIMATION_PARAM_COUNT

} AnimationParam;

typedef enum {

	ANIMATION_NODE_CLIP,
	ANIMATION_NODE_SELECT,
	ANIMATION_NODE_SEQUENCE,
	ANIMATION_NODE_BLEND,
	ANIMATION_NODE_LAYER,

} AnimationNodeType;

typedef struct {

	const char *name;
	uint8_t buffer;
	bool is_looping;
	
} AnimationClipDef;

typedef struct {

	AnimationNodeType type;
	uint8_t animation;
	uint8_t animation2;
	uint8_t buffer;
	uint8_t param;

} AnimationNode;

typedef struct {

	const AnimationClipDef *clip;
	const AnimationNode *node;

	uint8_t clip_count;
	uint8_t node_count;
	uint8_t buffer_count;
	uint8_t walk_animation;
	uint8_t run_animation;
	uint8_t sprint_animation;
	uint8_t turn_walk_animation;
	uint8_t turn_run_animation;
	uint8_t jump_animation;
	uint8_t fall_animation;
	uint8_t land_animation;
	uint8_t roll_animation;
	uint8_t stand_roll_animation;

} AnimationDef;

typedef struct {

	float action_idle_max_blending_ratio;
	float run_to_walk_ratio;
	float walk_to_run_ratio;
	float sprint_to_run_ratio;
	float run_to_sprint_ratio;
	float sprint_to_walk_ratio;
	float walk_to_sprint_ratio;
	float walking_anim_length_half;
	float walking_anim_length;
	float running_anim_length_half;
	float running_anim_length;
	float sprinting_anim_length_half;
	float sprinting_anim_length;

} ActorAnimationLocomotionSettings;

typedef struct {

	float jump_max_blending_ratio;
	float jump_anim_length;
	float jump_anim_crouch;
	float jump_anim_air;
	float jump_footing_speed;
	float land_anim_length;
	float land_anim_ground;
	float land_anim_crouch;
	float land_anim_stand;

} ActorAnimationJumpSettings;

typedef struct {

	float run_to_rolling_anim_ground;
	float run_to_rolling_anim_grip;
	float run_to_rolling_anim_stand;
	float run_to_rolling_anim_length;
	float stand_to_rolling_anim_ground;
	float stand_to_rolling_anim_grip;
	float stand_to_rolling_anim_stand;
	float stand_to_rolling_anim_length;

} ActorAnimationRollSettings;

typedef struct {

	ActorAnimationLocomotionSettings standing_locomotion;
	ActorAnimationJumpSettings       jump;
	ActorAnimationRollSettings       roll;

} ActorAnimationSettings;

typedef struct {

	float delta;

	Entity *entity;
	Actor *actor;

	ActorAnimation *animation;
	const AnimationDef *def;
	const ActorAnimationSettings *settings;

	uint8_t speed_state;
	float locomotion_param;
	float locomotion_phase;
	float turning;

} AnimationParamCtx;


typedef struct {

	const T3DSkeleton *layer[ANIMATION_MAX_LAYERS];
	float weight[ANIMATION_MAX_LAYERS];
	uint8_t count;

} ActorAnimationBuffer;

typedef struct ActorAnimation {

	T3DSkeleton  main;
	T3DSkeleton *buffer;
	T3DAnim     *clip;
	uint8_t     *node_state;
	bool        *node_active;
	float        param[ANIMATION_PARAM_COUNT];
	uint8_t      prev_speed_state;
	uint8_t      action_state;
	float        turn_avg[ANIMATION_TURN_AVG_COUNT];
	uint8_t      turn_avg_idx;
	ActorAnimationSettings settings;

} ActorAnimation;


void actorAnimation_addLayer(ActorAnimationBuffer *buffer, const T3DSkeleton *skel, float weight);
void actorAnimation_blendLayers(const T3DSkeleton *main, const ActorAnimationBuffer *buffer);
void actorAnimation_initGraph(Entity *entity, const AnimationDef *def);
void actorAnimation_setParams(Entity *entity, const AnimationDef *def);
void actorAnimation_evaluateGraph(const AnimationDef *def, const ActorAnimationSettings *settings, ActorAnimation *animation, float delta);

void actor_setAnimation(Entity *entity);

#endif
