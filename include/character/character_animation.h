#ifndef CHARACTER_ANIMATION_H
#define CHARACTER_ANIMATION_H

#include <stdbool.h>
#include <stdint.h>
#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>


typedef struct Entity Entity;
typedef struct Character Character;
typedef struct CharacterAnimation CharacterAnimation;

#define LAND_ANIMATION_STARTING_HEIGHT 130
#define ANIMATION_MAX_LAYERS 16

#define ANIMATION_SLOT_MAIN          0xFF
#define ANIMATION_TURN_AVG_COUNT  5

typedef enum {

	ANIMATION_PARAM_IDLE_RIGHT,
	ANIMATION_PARAM_WALK,   /* weight of the locomotion grid over the idle */
	ANIMATION_PARAM_WALK_GAIT,   /* position on the gait axis, [0,1] over the movement table */
	ANIMATION_PARAM_WALK_TURN,   /* turn axis: 0 left, 0.5 straight, 1 right */
	ANIMATION_PARAM_STRAFE,
	ANIMATION_PARAM_STRAFE_GAIT,
	ANIMATION_PARAM_STRAFE_DIR,
	ANIMATION_PARAM_STRAFE_LOCKED,
	ANIMATION_PARAM_STRAFE_LOCKED_GAIT,
	ANIMATION_PARAM_STRAFE_LOCKED_DIR,
	ANIMATION_PARAM_BOW_WALK_AIMING,
	ANIMATION_PARAM_BOW_WALK_AIMING_DIR,
	ANIMATION_PARAM_JUMP_L,
	ANIMATION_PARAM_JUMP_R,
	ANIMATION_PARAM_LAND_L,
	ANIMATION_PARAM_LAND_R,
	ANIMATION_PARAM_ROLL_RUN,
	ANIMATION_PARAM_ROLL_DIR,
	ANIMATION_PARAM_COUNT

} CharacterAnimationParam;

typedef enum {

	ANIMATION_NODE_CLIP,
	ANIMATION_NODE_SELECT,
	ANIMATION_NODE_SEQUENCE,
	ANIMATION_NODE_BLEND,
	ANIMATION_NODE_BLEND_2D,
	ANIMATION_NODE_LAYER,

} CharacterAnimationNodeType;

typedef struct {

	const char *name;
	uint8_t buffer;
	bool is_looping;
	
} CharacterAnimationClipDef;

typedef struct {

	CharacterAnimationNodeType type;
	const uint8_t *animation;
	uint8_t cols;
	uint8_t rows;
	uint8_t buffer;
	uint8_t param_cols;
	uint8_t param_rows;
	uint8_t param_weight;   /* BLEND_2D: weight the composed grid enters the main with */

} CharacterAnimationNode;

#define ANIMATION_CLIPS(...) ((const uint8_t[]){ __VA_ARGS__ })

typedef struct {

	float action_idle_max_blending_ratio;

	float turn_max_angle;
	float turn_max_weight;

	float jump_max_blending_ratio;
	float jump_anim_length;
	float jump_anim_crouch;
	float jump_anim_air;
	float jump_footing_speed;
	float land_anim_length;
	float land_anim_ground;
	float land_anim_crouch;
	float land_anim_stand;

	float run_to_rolling_anim_ground;
	float run_to_rolling_anim_grip;
	float run_to_rolling_anim_stand;
	float run_to_rolling_anim_length;

	float strafe_turn_rate;
	float strafe_blend_rate;

	float strafe_locked_blend_rate;
	float bow_walk_aiming_blend_rate;

} CharacterAnimationSettings;


typedef struct {

	const CharacterAnimationClipDef *clip;
	const CharacterAnimationNode *node;
	const CharacterAnimationSettings *settings;

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
	uint8_t locomotion_node;
	uint8_t strafe_node;
	uint8_t strafe_locked_node;
	uint8_t bow_walk_aiming_node;

} CharacterAnimationDef;

typedef struct {

	float delta;

	Entity *entity;
	Character *character;

	CharacterAnimation *animation;
	const CharacterAnimationDef *def;
	const CharacterAnimationSettings *settings;

	uint8_t speed_state;
	float gait_param;
	float locomotion_phase;
	float turning;

} CharacterAnimationParamCtx;


typedef struct {

	const T3DSkeleton *layer[ANIMATION_MAX_LAYERS];
	float weight[ANIMATION_MAX_LAYERS];
	uint8_t count;

} CharacterAnimationBuffer;

typedef struct CharacterAnimation {

	const CharacterAnimationDef *def;
	const T3DModel *model;
	T3DSkeleton  main;
	T3DSkeleton *buffer;
	T3DAnim     *clip;
	uint8_t     *clip_cooldown;   /* frames since the graph last touched each clip */
	uint8_t     *node_state;
	bool        *node_active;
	float        param[ANIMATION_PARAM_COUNT];

	/* Normalized time of the clip carrying the most weight this frame, left
	   behind by the graph for whoever needs to fire on the gait cycle. */
	float        locomotion_cycle;

	uint8_t      prev_speed_state;
	uint8_t      action_state;
	float        turn_avg[ANIMATION_TURN_AVG_COUNT];
	uint8_t      turn_avg_idx;
	bool         strafe_turning;
	float        strafe_blend;
	float        strafe_locked_blend;
	float        bow_walk_aiming_blend;

} CharacterAnimation;


void characterAnimation_addLayer(CharacterAnimationBuffer *buffer, const T3DSkeleton *skel, float weight);
void characterAnimation_blendLayers(const T3DSkeleton *main, const CharacterAnimationBuffer *buffer);
void characterAnimation_initGraph(Character *character, const CharacterAnimationDef *def);
void characterAnimation_setParams(Character *character, const CharacterAnimationDef *def);
void characterAnimation_evaluateGraph(const CharacterAnimationDef *def, CharacterAnimation *animation, float delta);

void character_setAnimation(Character *character);

#endif
