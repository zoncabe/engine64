#include <assert.h>
#include <math.h>
#include <malloc.h>
#include <string.h>
#include "time/time.h"
#include "entity/entity.h"
#include "viewport/viewport.h"


void characterAnimation_addLayer(CharacterAnimationBuffer *buffer, const T3DSkeleton *skel, float weight)
{
	buffer->layer[buffer->count] = skel;
	buffer->weight[buffer->count] = weight;
	buffer->count++;
}

void characterAnimation_blendLayers(const T3DSkeleton *main, const CharacterAnimationBuffer *buffer)
{
	for (int i = 0; i < main->skeletonRef->boneCount; i++)
	{
		T3DBone *bone = &main->bones[i];
		bone->hasChanged = true;

		for (int j = 0; j < buffer->count; j++)
		{
			T3DBone *layer = &buffer->layer[j]->bones[i];
			t3d_quat_nlerp(&bone->rotation, &bone->rotation, &layer->rotation, buffer->weight[j]);
			t3d_vec3_lerp(&bone->position,  &bone->position,  &layer->position,  buffer->weight[j]);
			t3d_vec3_lerp(&bone->scale,     &bone->scale,     &layer->scale,     buffer->weight[j]);
		}
	}
}

static uint8_t characterAnimation_getSpeedState(float speed, const CharacterMovementSettings *movement)
{
	if (speed <= 0.0f)                        return MOVEMENT_STATE_IDLE;
	if (speed <= movement->walk_target_speed) return MOVEMENT_STATE_WALKING;
	if (speed <= movement->run_target_speed)  return MOVEMENT_STATE_RUNNING;
	return MOVEMENT_STATE_SPRINTING;
}

static float characterAnimation_getLocomotionWeight(float speed, uint8_t speed_state, const CharacterMovementSettings *movement)
{
	float walk_speed   = movement->walk_target_speed;
	float run_speed    = movement->run_target_speed;
	float sprint_speed = movement->sprint_target_speed;

	float p;
	switch (speed_state) {
		case MOVEMENT_STATE_WALKING:   p = speed / walk_speed; break;
		case MOVEMENT_STATE_RUNNING:   p = (speed - walk_speed)  / (run_speed    - walk_speed); break;
		case MOVEMENT_STATE_SPRINTING: p = (speed - run_speed)   / (sprint_speed - run_speed); break;
		default:        return 0.0f;
	}
	if (p > 1.0f) p = 1.0f;
	if (p < 0.0f) p = 0.0f;
	return p;
}

static float characterAnimation_getLocomotionPhase(float clip_time, float clip_length)
{
	float phase = clip_time / clip_length;
	float f;
	if      (phase <= 0.25f) f = 0.5f - 2.0f * phase;
	else if (phase <= 0.75f) f = 2.0f * (phase - 0.25f);
	else                     f = 1.0f - 2.0f * (phase - 0.75f);
	if (f > 0.9999999f) f = 0.9999999f;
	if (f < 0.0000001f) f = 0.0000001f;
	return f;
}

static void characterAnimation_setJumpFootingSpeed(const CharacterAnimationParamCtx *ctx)
{
	uint8_t cur = ctx->character->movement.current;
	if (cur != MOVEMENT_STATE_JUMPING && cur != MOVEMENT_STATE_FALLING) return;

	float jump   = ctx->animation->param[ANIMATION_PARAM_JUMP_L] + ctx->animation->param[ANIMATION_PARAM_JUMP_R];
	float factor = ctx->settings->jump_footing_speed * (1.0f - jump);
	if (factor < 0.0f) factor = 0.0f;

	ctx->animation->clip[ctx->def->walk_animation].speed          *= factor;
	ctx->animation->clip[ctx->def->run_animation].speed           *= factor;
	ctx->animation->clip[ctx->def->sprint_animation].speed        *= factor;
	ctx->animation->clip[ctx->def->turn_walk_animation].speed     *= factor;
	ctx->animation->clip[ctx->def->turn_walk_animation + 1].speed *= factor;
	ctx->animation->clip[ctx->def->turn_run_animation].speed      *= factor;
	ctx->animation->clip[ctx->def->turn_run_animation + 1].speed  *= factor;
}

static void characterAnimation_setIdleSpeed(const CharacterAnimationParamCtx *ctx) { (void)ctx; }

static void characterAnimation_setWalkingSpeed(const CharacterAnimationParamCtx *ctx)
{
	float p = ctx->locomotion_param;
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->walk_animation],          p);
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->turn_walk_animation],     p);
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->turn_walk_animation + 1], p);
	characterAnimation_setJumpFootingSpeed(ctx);
}

static void characterAnimation_setRunningSpeed(const CharacterAnimationParamCtx *ctx)
{
	const CharacterAnimationSettings *l = ctx->settings;
	float run  = l->walk_to_run_ratio + (1.0f - l->walk_to_run_ratio) * ctx->locomotion_param;
	float walk = run * l->run_to_walk_ratio;
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->run_animation],           run);
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->walk_animation],          walk);
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->turn_run_animation],      run);
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->turn_run_animation  + 1], run);
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->turn_walk_animation],     walk);
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->turn_walk_animation + 1], walk);
	characterAnimation_setJumpFootingSpeed(ctx);
}

static void characterAnimation_setSprintingSpeed(const CharacterAnimationParamCtx *ctx)
{
	const CharacterAnimationSettings *l = ctx->settings;
	float sprint = l->run_to_sprint_ratio + (1.0f - l->run_to_sprint_ratio) * ctx->locomotion_param;
	float run    = sprint * l->sprint_to_run_ratio;
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->sprint_animation],        sprint);
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->run_animation],           run);
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->turn_run_animation],      run);
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->turn_run_animation  + 1], run);
	characterAnimation_setJumpFootingSpeed(ctx);
}

typedef void (*CharacterAnimationSpeedSyncFn)(const CharacterAnimationParamCtx *);
static const CharacterAnimationSpeedSyncFn characterAnimation_speedSync[MOVEMENT_STATE_SPRINTING + 1] = {
	[MOVEMENT_STATE_IDLE] = characterAnimation_setIdleSpeed,
	[MOVEMENT_STATE_WALKING]       = characterAnimation_setWalkingSpeed,
	[MOVEMENT_STATE_RUNNING]       = characterAnimation_setRunningSpeed,
	[MOVEMENT_STATE_SPRINTING]     = characterAnimation_setSprintingSpeed,
};

static void characterAnimation_snapWalkToRun(const CharacterAnimationParamCtx *ctx)
{
	const CharacterAnimationSettings *l = ctx->settings;
	T3DAnim *walk = &ctx->animation->clip[ctx->def->walk_animation];
	T3DAnim *run  = &ctx->animation->clip[ctx->def->run_animation];

	t3d_anim_set_time(run, walk->time * l->walk_to_run_ratio);
	t3d_anim_set_time(&ctx->animation->clip[ctx->def->turn_run_animation],     run->time);
	t3d_anim_set_time(&ctx->animation->clip[ctx->def->turn_run_animation + 1], run->time);
}

static void characterAnimation_snapRunToSprint(const CharacterAnimationParamCtx *ctx)
{
	const CharacterAnimationSettings *l = ctx->settings;
	T3DAnim *run    = &ctx->animation->clip[ctx->def->run_animation];
	T3DAnim *sprint = &ctx->animation->clip[ctx->def->sprint_animation];

	t3d_anim_set_time(sprint, run->time * l->run_to_sprint_ratio);
}

static void characterAnimation_snapSprintToRun(const CharacterAnimationParamCtx *ctx)
{
	const CharacterAnimationSettings *l = ctx->settings;
	T3DAnim *sprint = &ctx->animation->clip[ctx->def->sprint_animation];
	T3DAnim *walk   = &ctx->animation->clip[ctx->def->walk_animation];
	T3DAnim *run    = &ctx->animation->clip[ctx->def->run_animation];

	t3d_anim_set_time(walk, sprint->time * l->sprint_to_walk_ratio);
	t3d_anim_set_time(&ctx->animation->clip[ctx->def->turn_walk_animation],     walk->time);
	t3d_anim_set_time(&ctx->animation->clip[ctx->def->turn_walk_animation + 1], walk->time);
	t3d_anim_set_time(&ctx->animation->clip[ctx->def->turn_run_animation],      run->time);
	t3d_anim_set_time(&ctx->animation->clip[ctx->def->turn_run_animation + 1],  run->time);
}

static void characterAnimation_snapRunToWalk(const CharacterAnimationParamCtx *ctx)
{
	T3DAnim *walk = &ctx->animation->clip[ctx->def->walk_animation];

	t3d_anim_set_time(&ctx->animation->clip[ctx->def->turn_walk_animation],     walk->time);
	t3d_anim_set_time(&ctx->animation->clip[ctx->def->turn_walk_animation + 1], walk->time);
}

static void characterAnimation_snapRollToLocomotion(const CharacterAnimationParamCtx *ctx, bool left)
{
	const CharacterAnimationSettings *l = ctx->settings;
	float snap = left ? l->running_anim_length : l->running_anim_length_half;
	float run_time  = snap;
	float walk_time = snap * l->run_to_walk_ratio;
	t3d_anim_set_time(&ctx->animation->clip[ctx->def->run_animation],           run_time);
	t3d_anim_set_time(&ctx->animation->clip[ctx->def->walk_animation],          walk_time);
	t3d_anim_set_time(&ctx->animation->clip[ctx->def->sprint_animation],        snap * l->run_to_sprint_ratio);
	t3d_anim_set_time(&ctx->animation->clip[ctx->def->turn_run_animation],      run_time);
	t3d_anim_set_time(&ctx->animation->clip[ctx->def->turn_run_animation + 1],  run_time);
	t3d_anim_set_time(&ctx->animation->clip[ctx->def->turn_walk_animation],     walk_time);
	t3d_anim_set_time(&ctx->animation->clip[ctx->def->turn_walk_animation + 1], walk_time);
}

static void characterAnimation_syncTurnWalk(const CharacterAnimationParamCtx *ctx, float walk)
{
	uint8_t idx = (walk < 0.0f) ? ctx->def->turn_walk_animation : ctx->def->turn_walk_animation + 1;
	t3d_anim_set_time(&ctx->animation->clip[idx], ctx->animation->clip[ctx->def->walk_animation].time);
}

static void characterAnimation_syncTurnRun(const CharacterAnimationParamCtx *ctx, float run)
{
	uint8_t idx = (run < 0.0f) ? ctx->def->turn_run_animation : ctx->def->turn_run_animation + 1;
	t3d_anim_set_time(&ctx->animation->clip[idx], ctx->animation->clip[ctx->def->run_animation].time);
}

typedef void (*CharacterAnimationEntrySnapFn)(const CharacterAnimationParamCtx *);
static const CharacterAnimationEntrySnapFn characterAnimation_entrySnap[MOVEMENT_STATE_SPRINTING + 1][MOVEMENT_STATE_SPRINTING + 1] = {
	[MOVEMENT_STATE_WALKING  ][MOVEMENT_STATE_RUNNING  ] = characterAnimation_snapWalkToRun,
	[MOVEMENT_STATE_RUNNING  ][MOVEMENT_STATE_WALKING  ] = characterAnimation_snapRunToWalk,
	[MOVEMENT_STATE_RUNNING  ][MOVEMENT_STATE_SPRINTING] = characterAnimation_snapRunToSprint,
	[MOVEMENT_STATE_SPRINTING][MOVEMENT_STATE_RUNNING  ] = characterAnimation_snapSprintToRun,
};

static void characterAnimation_setLocomotionParam(const CharacterAnimationParamCtx *ctx)
{
	float p = ctx->locomotion_param;

	switch (ctx->speed_state)
	{
		case MOVEMENT_STATE_IDLE:
			ctx->animation->param[ANIMATION_PARAM_WALK]   = 0.0f;
			ctx->animation->param[ANIMATION_PARAM_RUN]    = 0.0f;
			ctx->animation->param[ANIMATION_PARAM_SPRINT] = 0.0f;
			break;

		case MOVEMENT_STATE_WALKING:
			ctx->animation->param[ANIMATION_PARAM_WALK]   = p;
			ctx->animation->param[ANIMATION_PARAM_RUN]    = 0.0f;
			ctx->animation->param[ANIMATION_PARAM_SPRINT] = 0.0f;
			break;

		case MOVEMENT_STATE_RUNNING:
			ctx->animation->param[ANIMATION_PARAM_WALK]   = 1.0f - p;
			ctx->animation->param[ANIMATION_PARAM_RUN]    = p;
			ctx->animation->param[ANIMATION_PARAM_SPRINT] = 0.0f;
			break;

		case MOVEMENT_STATE_SPRINTING:
			ctx->animation->param[ANIMATION_PARAM_WALK]   = 0.0f;
			ctx->animation->param[ANIMATION_PARAM_RUN]    = 1.0f - p;
			ctx->animation->param[ANIMATION_PARAM_SPRINT] = p;
			break;
	}
}

static void characterAnimation_setIdleRightParam(const CharacterAnimationParamCtx *ctx)
{
	ctx->animation->param[ANIMATION_PARAM_IDLE_RIGHT] =
		ctx->settings->action_idle_max_blending_ratio * ctx->locomotion_phase;
}

static float characterAnimation_getTurningAvg(CharacterAnimation *animation, float current_yaw, float previous_yaw)
{
	float delta_yaw = current_yaw - previous_yaw;
	if (delta_yaw >  180.0f) delta_yaw -= 360.0f;
	if (delta_yaw <= -180.0f) delta_yaw += 360.0f;

	animation->turn_avg[animation->turn_avg_idx] = delta_yaw;
	animation->turn_avg_idx = (animation->turn_avg_idx + 1) % ANIMATION_TURN_AVG_COUNT;

	float sum = 0.0f;
	for (int i = 0; i < ANIMATION_TURN_AVG_COUNT; i++) sum += animation->turn_avg[i];
	float avg_delta_yaw = sum / ANIMATION_TURN_AVG_COUNT;

	float r = avg_delta_yaw / ANIMATION_TURN_MAX_ANGLE;

	if (r >  1.0f) r =  1.0f;
	if (r < -1.0f) r = -1.0f;
	if (fabsf(r) < 0.001f) r = 0.0f;
	return r;
}

static void characterAnimation_setTurningParam(const CharacterAnimationParamCtx *ctx)
{
	float r = ctx->turning;
	float walk, run;

	switch (ctx->speed_state) {
		case MOVEMENT_STATE_WALKING:
			walk = r * ctx->locomotion_param;
			run  = 0.0f;
			break;
		case MOVEMENT_STATE_RUNNING:
			walk = r * (1.0f - ctx->locomotion_param);
			run  = r * ctx->locomotion_param;
			break;
		case MOVEMENT_STATE_SPRINTING:
			walk = 0.0f;
			run  = r;
			break;
		default:
			walk = 0.0f;
			run  = 0.0f;
			break;
	}

	float prev_walk = ctx->animation->param[ANIMATION_PARAM_TURN_WALK];
	if (walk != 0.0f && (prev_walk == 0.0f || (prev_walk < 0.0f) != (walk < 0.0f)))
		characterAnimation_syncTurnWalk(ctx, walk);

	float prev_run = ctx->animation->param[ANIMATION_PARAM_TURN_RUN];
	if (run != 0.0f && (prev_run == 0.0f || (prev_run < 0.0f) != (run < 0.0f)))
		characterAnimation_syncTurnRun(ctx, run);

	ctx->animation->param[ANIMATION_PARAM_TURN_WALK] = walk;
	ctx->animation->param[ANIMATION_PARAM_TURN_RUN]  = run;
}

static void characterAnimation_setRollParam(const CharacterAnimationParamCtx *ctx)
{
	uint8_t  cur = ctx->character->movement.current;
	uint8_t *as  = &ctx->animation->action_state;

	if (cur != MOVEMENT_STATE_ROLLING) {
		if (*as == MOVEMENT_STATE_ROLLING) *as = cur;
		ctx->animation->param[ANIMATION_PARAM_ROLL_RUN] = 0.0f;
		ctx->animation->param[ANIMATION_PARAM_ROLL_DIR] = 0.0f;
		return;
	}

	if (*as != MOVEMENT_STATE_ROLLING) {
		uint8_t base = ctx->def->roll_animation;

		t3d_anim_set_playing(&ctx->animation->clip[base],     true);
		t3d_anim_set_time   (&ctx->animation->clip[base],     0.0f);
		t3d_anim_set_playing(&ctx->animation->clip[base + 1], true);
		t3d_anim_set_time   (&ctx->animation->clip[base + 1], 0.0f);

		float dir = (ctx->locomotion_phase < 0.5f) ? -1.0f : 1.0f;
		ctx->animation->param[ANIMATION_PARAM_ROLL_RUN] = 0.0f;
		ctx->animation->param[ANIMATION_PARAM_ROLL_DIR] = dir;
		*as = MOVEMENT_STATE_ROLLING;
	}

	const CharacterAnimationSettings *r = ctx->settings;
	float dir   = ctx->animation->param[ANIMATION_PARAM_ROLL_DIR];
	bool  left  = dir < 0.0f;

	uint8_t base      = ctx->def->roll_animation;
	uint8_t roll_idx  = left ? base : base + 1;
	float   roll_time = ctx->animation->clip[roll_idx].time;
	float   ratio     = ctx->animation->param[ANIMATION_PARAM_ROLL_RUN];

	if (roll_time < r->run_to_rolling_anim_ground && ratio <= 1.0f)
		ratio += ctx->delta / r->run_to_rolling_anim_ground;

	if (roll_time > r->run_to_rolling_anim_stand && ratio > 0.0f)
		ratio -= ctx->delta / (r->run_to_rolling_anim_length - r->run_to_rolling_anim_stand);

	if (ratio > 1.0f) {
		ratio = 1.0f;
		characterAnimation_snapRollToLocomotion(ctx, left);
	}

	if (ratio < 0.0f) ratio = 0.0f;

	ctx->animation->param[ANIMATION_PARAM_ROLL_RUN] = ratio;
}

static void characterAnimation_syncLandToJump(const CharacterAnimationParamCtx *ctx)
{
	const CharacterAnimationSettings *j = ctx->settings;
	T3DAnim *jump_l   = &ctx->animation->clip[ctx->def->jump_animation];
	T3DAnim *jump_r   = &ctx->animation->clip[ctx->def->jump_animation + 1];
	float    land_t   = ctx->animation->clip[ctx->def->land_animation].time;
	float    jump_t;

	if (land_t < j->land_anim_crouch)
		jump_t = (land_t / j->land_anim_crouch) * j->jump_anim_crouch;
	else
		jump_t = (1.0f - (land_t - j->land_anim_crouch) / (j->land_anim_stand - j->land_anim_crouch)) * j->jump_anim_crouch;

	if (jump_t < 0.0f)              jump_t = 0.0f;
	if (jump_t > j->jump_anim_crouch) jump_t = j->jump_anim_crouch;

	t3d_anim_set_time(jump_l, jump_t);
	t3d_anim_set_time(jump_r, jump_t);
}

static void characterAnimation_snapToJump(const CharacterAnimationParamCtx *ctx)
{
	T3DAnim *jump_l   = &ctx->animation->clip[ctx->def->jump_animation];
	T3DAnim *jump_r   = &ctx->animation->clip[ctx->def->jump_animation + 1];
	T3DAnim *land_animation = &ctx->animation->clip[ctx->def->land_animation];

	T3DAnim *fall_l = &ctx->animation->clip[ctx->def->fall_animation];
	T3DAnim *fall_r = &ctx->animation->clip[ctx->def->fall_animation + 1];

	t3d_anim_set_playing(jump_l, true);
	t3d_anim_set_playing(jump_r, true);
	
	t3d_anim_set_time(fall_l, 0.0f);
	t3d_anim_set_time(fall_r, 0.0f);

	if (land_animation->isPlaying) {
		t3d_anim_set_time(jump_l, 0.0f);
		t3d_anim_set_time(jump_r, 0.0f);
	} else {
		characterAnimation_syncLandToJump(ctx);
	}

	ctx->animation->param[ANIMATION_PARAM_JUMP_L] = 0.0f;
	ctx->animation->param[ANIMATION_PARAM_JUMP_R] = 0.0f;
}

static void characterAnimation_snapToLand(const CharacterAnimationParamCtx *ctx)
{
	T3DAnim *land_l = &ctx->animation->clip[ctx->def->land_animation];
	T3DAnim *land_r = &ctx->animation->clip[ctx->def->land_animation + 1];
	t3d_anim_set_time(land_l, 0.0f);
	t3d_anim_set_time(land_r, 0.0f);
	t3d_anim_set_playing(land_l, true);
	t3d_anim_set_playing(land_r, true);
	ctx->animation->param[ANIMATION_PARAM_LAND_L] = 0.0f;
	ctx->animation->param[ANIMATION_PARAM_LAND_R] = 0.0f;
}

static void characterAnimation_setJumpParams(const CharacterAnimationParamCtx *ctx)
{
	const CharacterAnimationSettings *j   = ctx->settings;
	float    delta     = ctx->delta;
	T3DAnim *land_animation = &ctx->animation->clip[ctx->def->land_animation];
	uint8_t  cur       = ctx->character->movement.current;
	uint8_t *as        = &ctx->animation->action_state;
	float    footing   = ctx->locomotion_phase;

	float jump = ctx->animation->param[ANIMATION_PARAM_JUMP_L] + ctx->animation->param[ANIMATION_PARAM_JUMP_R];
	float land = ctx->animation->param[ANIMATION_PARAM_LAND_L] + ctx->animation->param[ANIMATION_PARAM_LAND_R];

	if (cur == MOVEMENT_STATE_JUMPING && *as != MOVEMENT_STATE_JUMPING) {
		characterAnimation_snapToJump(ctx);
		jump = 0.0f;
		*as  = MOVEMENT_STATE_JUMPING;
	}

	if (cur == MOVEMENT_STATE_FALLING && *as != MOVEMENT_STATE_FALLING) {
		characterAnimation_snapToLand(ctx);
		land = 0.0f;
		*as  = MOVEMENT_STATE_FALLING;
	}

	if ((*as == MOVEMENT_STATE_JUMPING || *as == MOVEMENT_STATE_FALLING) && cur != MOVEMENT_STATE_JUMPING && cur != MOVEMENT_STATE_FALLING)
		*as = cur;

	if (land_animation->isPlaying) {
		float crouch_rate = j->jump_max_blending_ratio * delta / j->land_anim_crouch;

		if (land_animation->time < j->land_anim_crouch) {
			if (ctx->character->body.position.z < LAND_ANIMATION_STARTING_HEIGHT) {
				land += crouch_rate;
				if (land > j->jump_max_blending_ratio) land = j->jump_max_blending_ratio;
			}
		} else {
			float stand_rate = j->jump_max_blending_ratio * delta / (j->land_anim_length - j->land_anim_crouch);
			land -= stand_rate;
			if (land < 0.0f) {
				land = 0.0f;
				t3d_anim_set_playing(&ctx->animation->clip[ctx->def->land_animation],     false);
				t3d_anim_set_playing(&ctx->animation->clip[ctx->def->land_animation + 1], false);
			}
		}

		jump -= crouch_rate;
		if (jump < 0.0f) jump = 0.0f;
	}

	if (cur == MOVEMENT_STATE_JUMPING && jump < j->jump_max_blending_ratio) {
		jump += j->jump_max_blending_ratio * delta / j->jump_anim_crouch;
		if (jump > j->jump_max_blending_ratio) jump = j->jump_max_blending_ratio;
	}

	ctx->animation->param[ANIMATION_PARAM_JUMP_L] = jump * (1.0f - footing);
	ctx->animation->param[ANIMATION_PARAM_JUMP_R] = jump * footing;
	ctx->animation->param[ANIMATION_PARAM_LAND_L] = land * (1.0f - footing);
	ctx->animation->param[ANIMATION_PARAM_LAND_R] = land * footing;
}

static void characterAnimation_setActiveNodes(const CharacterAnimationParamCtx *ctx)
{
	const CharacterAnimationDef *def = ctx->def;
	CharacterAnimation *animation = ctx->animation;

	for (int i = 0; i < def->node_count; i++) {
		CharacterAnimationNodeType t = def->node[i].type;
		if (t == ANIMATION_NODE_SELECT || t == ANIMATION_NODE_SEQUENCE)
			animation->node_active[i] = (animation->param[def->node[i].param] != 0.0f);
	}
}

void characterAnimation_setParams(Character *character, const CharacterAnimationDef *def)
{
	CharacterAnimation *animation = &character->animation;
	const CharacterMovementSettings *movement = character->movement.settings;
	float speed  = character->movement.data.horizontal_speed;

	CharacterAnimationParamCtx ctx = {

		.entity = character->entity,
		.character = character,
		.animation = animation,
		.def = def,
		.settings = def->settings,
		.delta = time_get()->delta,
	};

	ctx.speed_state      = characterAnimation_getSpeedState(speed, movement);
	ctx.locomotion_param = characterAnimation_getLocomotionWeight(speed, ctx.speed_state, movement);
	ctx.locomotion_phase = characterAnimation_getLocomotionPhase(animation->clip[def->walk_animation].time, ctx.settings->walking_anim_length);
	ctx.turning          = characterAnimation_getTurningAvg(animation, character->body.rotation.z, character->movement.data.previous_yaw);

	characterAnimation_speedSync[ctx.speed_state](&ctx);

	uint8_t prev_speed = animation->prev_speed_state;
	if (ctx.speed_state != prev_speed) {
		CharacterAnimationEntrySnapFn snap = characterAnimation_entrySnap[prev_speed][ctx.speed_state];
		if (snap) snap(&ctx);
		animation->prev_speed_state = ctx.speed_state;
	}

	characterAnimation_setLocomotionParam (&ctx);
	characterAnimation_setIdleRightParam (&ctx);
	characterAnimation_setTurningParam (&ctx);
	characterAnimation_setJumpParams (&ctx);
	characterAnimation_setRollParam (&ctx);
	characterAnimation_setActiveNodes (&ctx);
}

void characterAnimation_initGraph(Character *character, const CharacterAnimationDef *def)
{
	CharacterAnimation *animation = &character->animation;
	const T3DModel *model = character->entity->mesh->model;

	animation->main = t3d_skeleton_create_buffered(model, FB_COUNT);

	animation->buffer = malloc(def->buffer_count * sizeof(T3DSkeleton));
	assert(animation->buffer);
	animation->clip = malloc(def->clip_count * sizeof(T3DAnim));
	assert(animation->clip);
	animation->node_state = malloc(def->node_count * sizeof(uint8_t));
	assert(animation->node_state);
	animation->node_active = malloc(def->node_count * sizeof(bool));
	assert(animation->node_active);

	for (int i = 0; i < def->buffer_count; i++)
		animation->buffer[i] = t3d_skeleton_clone(&animation->main, false);

	memset(animation->node_state,   0,    def->node_count * sizeof(uint8_t));
	memset(animation->node_active,  true, def->node_count * sizeof(bool));
	memset(animation->turn_avg,  0,    sizeof(animation->turn_avg));
	animation->turn_avg_idx = 0;

	for (int i = 0; i < def->clip_count; i++)
	{
		const CharacterAnimationClipDef *clip_def = &def->clip[i];

		animation->clip[i] = t3d_anim_create(model, clip_def->name);

		T3DSkeleton *target = (clip_def->buffer == ANIMATION_SLOT_MAIN)
			? &animation->main
			: &animation->buffer[clip_def->buffer];

		t3d_anim_attach(&animation->clip[i], target);
		t3d_anim_set_looping(&animation->clip[i], clip_def->is_looping);
		t3d_anim_set_playing(&animation->clip[i], clip_def->is_looping);
	}
}

void characterAnimation_evaluateGraph(const CharacterAnimationDef *def, CharacterAnimation *animation, float delta)
{
	CharacterAnimationBuffer blend_buffer;
	blend_buffer.count = 0;

	for (int i = 0; i < def->node_count; i++)
	{
		if (!animation->node_active[i]) continue;

		const CharacterAnimationNode *node = &def->node[i];
		T3DAnim *clip = &animation->clip[node->animation];
		float param_val = animation->param[node->param];

		switch (node->type)
		{
			case ANIMATION_NODE_CLIP:
			{
				t3d_anim_update(clip, delta);
				break;
			}

			case ANIMATION_NODE_SELECT:
			{
				uint8_t active = (param_val < 0.0f) ? node->animation : node->animation2;
				uint8_t inactive = (param_val < 0.0f) ? node->animation2 : node->animation;

				if (animation->node_state[i] != active)
				{
					animation->node_state[i] = active;
					t3d_anim_set_time(&animation->clip[inactive], animation->clip[active].time);
				}

				t3d_anim_update(&animation->clip[active], delta);
				break;
			}

			case ANIMATION_NODE_SEQUENCE:
			{
				T3DAnim *clip2 = &animation->clip[node->animation2];
				if (clip->isPlaying)
				{
					float limit = t3d_anim_get_length(clip);
					if ((clip->time + delta) < limit)
						t3d_anim_update(clip, delta);
					else
						t3d_anim_update(clip2, delta);
				}
				else
					t3d_anim_update(clip2, delta);

				break;
			}

			case ANIMATION_NODE_BLEND:
			{
				if (param_val > 0.0f) {
					T3DSkeleton *buf = (node->buffer == ANIMATION_SLOT_MAIN) ? &animation->main : &animation->buffer[node->buffer];
					t3d_anim_update(clip, delta);
					characterAnimation_addLayer(&blend_buffer, buf, param_val);
				}

				break;
			}

			case ANIMATION_NODE_LAYER:
			{
				float abs_val = fabsf(param_val);
				if (abs_val > 0.0f) {
					T3DSkeleton *buf = (node->buffer == ANIMATION_SLOT_MAIN) ? &animation->main : &animation->buffer[node->buffer];
					characterAnimation_addLayer(&blend_buffer, buf, abs_val);
				}

				break;
			}
		}
	}

	characterAnimation_blendLayers(&animation->main, &blend_buffer);

}

void character_setAnimation(Character *character)
{
	characterAnimation_setParams(character, character->animation.def);
	characterAnimation_evaluateGraph(character->animation.def, &character->animation, time_get()->delta);
	t3d_skeleton_update(&character->animation.main);
}