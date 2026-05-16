#include <assert.h>
#include <math.h>
#include <malloc.h>
#include <string.h>
#include "time/time.h"
#include "entity/entity.h"
#include "viewport/viewport.h"


void actorAnimation_addLayer(ActorAnimationBuffer *buffer, const T3DSkeleton *skel, float weight)
{
	buffer->layer[buffer->count] = skel;
	buffer->weight[buffer->count] = weight;
	buffer->count++;
}

void actorAnimation_blendLayers(const T3DSkeleton *main, const ActorAnimationBuffer *buffer)
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

static uint8_t actorAnimation_getSpeedState(float speed, const ActorMotionSettings *motion)
{
	if (speed <= 0.0f)                                              return ACTOR_STATE_IDLE;
	if (speed <= motion->locomotion[LOCOMOTION_WALK].target_speed)  return ACTOR_STATE_WALKING;
	if (speed <= motion->locomotion[LOCOMOTION_RUN].target_speed)   return ACTOR_STATE_RUNNING;
	return ACTOR_STATE_SPRINTING;
}

static float actorAnimation_getLocomotionWeight(float speed, uint8_t speed_state, const ActorMotionSettings *motion)
{
	float walk_speed   = motion->locomotion[LOCOMOTION_WALK].target_speed;
	float run_speed    = motion->locomotion[LOCOMOTION_RUN].target_speed;
	float sprint_speed = motion->locomotion[LOCOMOTION_SPRINT].target_speed;

	float p;
	switch (speed_state) {
		case ACTOR_STATE_WALKING:   p = speed / walk_speed; break;
		case ACTOR_STATE_RUNNING:   p = (speed - walk_speed)  / (run_speed    - walk_speed); break;
		case ACTOR_STATE_SPRINTING: p = (speed - run_speed)   / (sprint_speed - run_speed); break;
		default:        return 0.0f;
	}
	if (p > 1.0f) p = 1.0f;
	if (p < 0.0f) p = 0.0f;
	return p;
}

static float actorAnimation_getLocomotionPhase(float clip_time, float clip_length)
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

static void actorAnimation_setJumpFootingSpeed(const AnimationParamCtx *ctx)
{
	uint8_t cur = ctx->actor->state.current;
	if (cur != ACTOR_STATE_JUMPING && cur != ACTOR_STATE_FALLING) return;

	float jump   = ctx->animation->param[ANIMATION_PARAM_JUMP_L] + ctx->animation->param[ANIMATION_PARAM_JUMP_R];
	float factor = ctx->settings->jump.jump_footing_speed * (1.0f - jump);
	if (factor < 0.0f) factor = 0.0f;

	ctx->animation->clip[ctx->def->walk_animation].speed          *= factor;
	ctx->animation->clip[ctx->def->run_animation].speed           *= factor;
	ctx->animation->clip[ctx->def->sprint_animation].speed        *= factor;
	ctx->animation->clip[ctx->def->turn_walk_animation].speed     *= factor;
	ctx->animation->clip[ctx->def->turn_walk_animation + 1].speed *= factor;
	ctx->animation->clip[ctx->def->turn_run_animation].speed      *= factor;
	ctx->animation->clip[ctx->def->turn_run_animation + 1].speed  *= factor;
}

static void actorAnimation_setIdleSpeed(const AnimationParamCtx *ctx) { (void)ctx; }

static void actorAnimation_setWalkingSpeed(const AnimationParamCtx *ctx)
{
	float p = ctx->locomotion_param;
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->walk_animation],          p);
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->turn_walk_animation],     p);
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->turn_walk_animation + 1], p);
	actorAnimation_setJumpFootingSpeed(ctx);
}

static void actorAnimation_setRunningSpeed(const AnimationParamCtx *ctx)
{
	const ActorAnimationLocomotionSettings *l = &ctx->settings->standing_locomotion;
	float run  = l->walk_to_run_ratio + (1.0f - l->walk_to_run_ratio) * ctx->locomotion_param;
	float walk = run * l->run_to_walk_ratio;
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->run_animation],           run);
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->walk_animation],          walk);
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->turn_run_animation],      run);
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->turn_run_animation  + 1], run);
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->turn_walk_animation],     walk);
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->turn_walk_animation + 1], walk);
	actorAnimation_setJumpFootingSpeed(ctx);
}

static void actorAnimation_setSprintingSpeed(const AnimationParamCtx *ctx)
{
	const ActorAnimationLocomotionSettings *l = &ctx->settings->standing_locomotion;
	float sprint = l->run_to_sprint_ratio + (1.0f - l->run_to_sprint_ratio) * ctx->locomotion_param;
	float run    = sprint * l->sprint_to_run_ratio;
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->sprint_animation],        sprint);
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->run_animation],           run);
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->turn_run_animation],      run);
	t3d_anim_set_speed(&ctx->animation->clip[ctx->def->turn_run_animation  + 1], run);
	actorAnimation_setJumpFootingSpeed(ctx);
}

typedef void (*SpeedSyncFn)(const AnimationParamCtx *);
static const SpeedSyncFn actorAnimation_speedSync[ACTOR_STATE_SPRINTING + 1] = {
	[ACTOR_STATE_IDLE] = actorAnimation_setIdleSpeed,
	[ACTOR_STATE_WALKING]       = actorAnimation_setWalkingSpeed,
	[ACTOR_STATE_RUNNING]       = actorAnimation_setRunningSpeed,
	[ACTOR_STATE_SPRINTING]     = actorAnimation_setSprintingSpeed,
};

static void actorAnimation_snapWalkToRun(const AnimationParamCtx *ctx)
{
	const ActorAnimationLocomotionSettings *l = &ctx->settings->standing_locomotion;
	T3DAnim *walk = &ctx->animation->clip[ctx->def->walk_animation];
	T3DAnim *run  = &ctx->animation->clip[ctx->def->run_animation];

	t3d_anim_set_time(run, walk->time * l->walk_to_run_ratio);
	t3d_anim_set_time(&ctx->animation->clip[ctx->def->turn_run_animation],     run->time);
	t3d_anim_set_time(&ctx->animation->clip[ctx->def->turn_run_animation + 1], run->time);
}

static void actorAnimation_snapRunToSprint(const AnimationParamCtx *ctx)
{
	const ActorAnimationLocomotionSettings *l = &ctx->settings->standing_locomotion;
	T3DAnim *run    = &ctx->animation->clip[ctx->def->run_animation];
	T3DAnim *sprint = &ctx->animation->clip[ctx->def->sprint_animation];

	t3d_anim_set_time(sprint, run->time * l->run_to_sprint_ratio);
}

static void actorAnimation_snapSprintToRun(const AnimationParamCtx *ctx)
{
	const ActorAnimationLocomotionSettings *l = &ctx->settings->standing_locomotion;
	T3DAnim *sprint = &ctx->animation->clip[ctx->def->sprint_animation];
	T3DAnim *walk   = &ctx->animation->clip[ctx->def->walk_animation];
	T3DAnim *run    = &ctx->animation->clip[ctx->def->run_animation];

	t3d_anim_set_time(walk, sprint->time * l->sprint_to_walk_ratio);
	t3d_anim_set_time(&ctx->animation->clip[ctx->def->turn_walk_animation],     walk->time);
	t3d_anim_set_time(&ctx->animation->clip[ctx->def->turn_walk_animation + 1], walk->time);
	t3d_anim_set_time(&ctx->animation->clip[ctx->def->turn_run_animation],      run->time);
	t3d_anim_set_time(&ctx->animation->clip[ctx->def->turn_run_animation + 1],  run->time);
}

static void actorAnimation_snapRunToWalk(const AnimationParamCtx *ctx)
{
	T3DAnim *walk = &ctx->animation->clip[ctx->def->walk_animation];

	t3d_anim_set_time(&ctx->animation->clip[ctx->def->turn_walk_animation],     walk->time);
	t3d_anim_set_time(&ctx->animation->clip[ctx->def->turn_walk_animation + 1], walk->time);
}

static void actorAnimation_snapRollToLocomotion(const AnimationParamCtx *ctx, bool left)
{
	const ActorAnimationLocomotionSettings *l = &ctx->settings->standing_locomotion;
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

static void actorAnimation_syncTurnWalk(const AnimationParamCtx *ctx, float walk)
{
	uint8_t idx = (walk < 0.0f) ? ctx->def->turn_walk_animation : ctx->def->turn_walk_animation + 1;
	t3d_anim_set_time(&ctx->animation->clip[idx], ctx->animation->clip[ctx->def->walk_animation].time);
}

static void actorAnimation_syncTurnRun(const AnimationParamCtx *ctx, float run)
{
	uint8_t idx = (run < 0.0f) ? ctx->def->turn_run_animation : ctx->def->turn_run_animation + 1;
	t3d_anim_set_time(&ctx->animation->clip[idx], ctx->animation->clip[ctx->def->run_animation].time);
}

typedef void (*EntrySnapFn)(const AnimationParamCtx *);
static const EntrySnapFn actorAnimation_entrySnap[ACTOR_STATE_SPRINTING + 1][ACTOR_STATE_SPRINTING + 1] = {
	[ACTOR_STATE_WALKING  ][ACTOR_STATE_RUNNING  ] = actorAnimation_snapWalkToRun,
	[ACTOR_STATE_RUNNING  ][ACTOR_STATE_WALKING  ] = actorAnimation_snapRunToWalk,
	[ACTOR_STATE_RUNNING  ][ACTOR_STATE_SPRINTING] = actorAnimation_snapRunToSprint,
	[ACTOR_STATE_SPRINTING][ACTOR_STATE_RUNNING  ] = actorAnimation_snapSprintToRun,
};

static void actorAnimation_setLocomotionParam(const AnimationParamCtx *ctx)
{
	float p = ctx->locomotion_param;

	switch (ctx->speed_state)
	{
		case ACTOR_STATE_IDLE:
			ctx->animation->param[ANIMATION_PARAM_WALK]   = 0.0f;
			ctx->animation->param[ANIMATION_PARAM_RUN]    = 0.0f;
			ctx->animation->param[ANIMATION_PARAM_SPRINT] = 0.0f;
			break;

		case ACTOR_STATE_WALKING:
			ctx->animation->param[ANIMATION_PARAM_WALK]   = p;
			ctx->animation->param[ANIMATION_PARAM_RUN]    = 0.0f;
			ctx->animation->param[ANIMATION_PARAM_SPRINT] = 0.0f;
			break;

		case ACTOR_STATE_RUNNING:
			ctx->animation->param[ANIMATION_PARAM_WALK]   = 1.0f - p;
			ctx->animation->param[ANIMATION_PARAM_RUN]    = p;
			ctx->animation->param[ANIMATION_PARAM_SPRINT] = 0.0f;
			break;

		case ACTOR_STATE_SPRINTING:
			ctx->animation->param[ANIMATION_PARAM_WALK]   = 0.0f;
			ctx->animation->param[ANIMATION_PARAM_RUN]    = 1.0f - p;
			ctx->animation->param[ANIMATION_PARAM_SPRINT] = p;
			break;
	}
}

static void actorAnimation_setIdleRightParam(const AnimationParamCtx *ctx)
{
	ctx->animation->param[ANIMATION_PARAM_IDLE_RIGHT] =
		ctx->settings->standing_locomotion.action_idle_max_blending_ratio * ctx->locomotion_phase;
}

static float actorAnimation_getTurningAvg(ActorAnimation *animation, float current_yaw, float previous_yaw)
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

static void actorAnimation_setTurningParam(const AnimationParamCtx *ctx)
{
	float r = ctx->turning;
	float walk, run;

	switch (ctx->speed_state) {
		case ACTOR_STATE_WALKING:
			walk = r * ctx->locomotion_param;
			run  = 0.0f;
			break;
		case ACTOR_STATE_RUNNING:
			walk = r * (1.0f - ctx->locomotion_param);
			run  = r * ctx->locomotion_param;
			break;
		case ACTOR_STATE_SPRINTING:
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
		actorAnimation_syncTurnWalk(ctx, walk);

	float prev_run = ctx->animation->param[ANIMATION_PARAM_TURN_RUN];
	if (run != 0.0f && (prev_run == 0.0f || (prev_run < 0.0f) != (run < 0.0f)))
		actorAnimation_syncTurnRun(ctx, run);

	ctx->animation->param[ANIMATION_PARAM_TURN_WALK] = walk;
	ctx->animation->param[ANIMATION_PARAM_TURN_RUN]  = run;
}

static void actorAnimation_setRollParam(const AnimationParamCtx *ctx)
{
	uint8_t  cur = ctx->actor->state.current;
	uint8_t *as  = &ctx->animation->action_state;

	if (cur != ACTOR_STATE_ROLLING) {
		if (*as == ACTOR_STATE_ROLLING) *as = cur;
		ctx->animation->param[ANIMATION_PARAM_ROLL_RUN]       = 0.0f;
		ctx->animation->param[ANIMATION_PARAM_ROLL_STAND]     = 0.0f;
		ctx->animation->param[ANIMATION_PARAM_ROLL_DIR]       = 0.0f;
		ctx->animation->param[ANIMATION_PARAM_STAND_ROLL_DIR] = 0.0f;
		return;
	}

	if (*as != ACTOR_STATE_ROLLING) {
		bool    stand = (ctx->actor->state.locomotion == ACTOR_STATE_IDLE);
		uint8_t base  = stand ? ctx->def->stand_roll_animation : ctx->def->roll_animation;

		t3d_anim_set_playing(&ctx->animation->clip[base],     true);
		t3d_anim_set_time   (&ctx->animation->clip[base],     0.0f);
		t3d_anim_set_playing(&ctx->animation->clip[base + 1], true);
		t3d_anim_set_time   (&ctx->animation->clip[base + 1], 0.0f);

		float dir = (ctx->locomotion_phase < 0.5f) ? -1.0f : 1.0f;
		ctx->animation->param[ANIMATION_PARAM_ROLL_RUN]       = 0.0f;
		ctx->animation->param[ANIMATION_PARAM_ROLL_STAND]     = 0.0f;
		ctx->animation->param[ANIMATION_PARAM_ROLL_DIR]       = stand ? 0.0f : dir;
		ctx->animation->param[ANIMATION_PARAM_STAND_ROLL_DIR] = stand ? dir  : 0.0f;
		*as = ACTOR_STATE_ROLLING;
	}

	const ActorAnimationRollSettings *r = &ctx->settings->roll;
	bool  stand = ctx->animation->param[ANIMATION_PARAM_STAND_ROLL_DIR] != 0.0f;
	float dir   = stand ? ctx->animation->param[ANIMATION_PARAM_STAND_ROLL_DIR] : ctx->animation->param[ANIMATION_PARAM_ROLL_DIR];
	bool  left  = dir < 0.0f;
	uint8_t weight_param = stand ? ANIMATION_PARAM_ROLL_STAND : ANIMATION_PARAM_ROLL_RUN;

	uint8_t base      = stand ? ctx->def->stand_roll_animation : ctx->def->roll_animation;
	uint8_t roll_idx  = left ? base : base + 1;
	float   roll_time = ctx->animation->clip[roll_idx].time;
	float   ratio     = ctx->animation->param[weight_param];

	float ground  = stand ? r->stand_to_rolling_anim_ground : r->run_to_rolling_anim_ground;
	float stand_t = stand ? r->stand_to_rolling_anim_stand  : r->run_to_rolling_anim_stand;
	float length  = stand ? r->stand_to_rolling_anim_length : r->run_to_rolling_anim_length;

	if (roll_time < ground && ratio <= 1.0f)
		ratio += ctx->delta / ground;

	if (roll_time > stand_t && ratio > 0.0f)
		ratio -= ctx->delta / (length - stand_t);

	if (ratio > 1.0f) {
		ratio = 1.0f;
		actorAnimation_snapRollToLocomotion(ctx, left);
	}

	if (ratio < 0.0f) ratio = 0.0f;

	ctx->animation->param[weight_param] = ratio;
}

static void actorAnimation_syncLandToJump(const AnimationParamCtx *ctx)
{
	const ActorAnimationJumpSettings *j = &ctx->settings->jump;
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

static void actorAnimation_snapToJump(const AnimationParamCtx *ctx)
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
		actorAnimation_syncLandToJump(ctx);
	}

	ctx->animation->param[ANIMATION_PARAM_JUMP_L] = 0.0f;
	ctx->animation->param[ANIMATION_PARAM_JUMP_R] = 0.0f;
}

static void actorAnimation_snapToLand(const AnimationParamCtx *ctx)
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

static void actorAnimation_setJumpParams(const AnimationParamCtx *ctx)
{
	const ActorAnimationJumpSettings *j   = &ctx->settings->jump;
	float    delta     = ctx->delta;
	T3DAnim *land_animation = &ctx->animation->clip[ctx->def->land_animation];
	uint8_t  cur       = ctx->actor->state.current;
	uint8_t *as        = &ctx->animation->action_state;
	float    footing   = ctx->locomotion_phase;

	float jump = ctx->animation->param[ANIMATION_PARAM_JUMP_L] + ctx->animation->param[ANIMATION_PARAM_JUMP_R];
	float land = ctx->animation->param[ANIMATION_PARAM_LAND_L] + ctx->animation->param[ANIMATION_PARAM_LAND_R];

	if (cur == ACTOR_STATE_JUMPING && *as != ACTOR_STATE_JUMPING) {
		actorAnimation_snapToJump(ctx);
		jump = 0.0f;
		*as  = ACTOR_STATE_JUMPING;
	}

	if (cur == ACTOR_STATE_FALLING && *as != ACTOR_STATE_FALLING) {
		actorAnimation_snapToLand(ctx);
		land = 0.0f;
		*as  = ACTOR_STATE_FALLING;
	}

	if ((*as == ACTOR_STATE_JUMPING || *as == ACTOR_STATE_FALLING) && cur != ACTOR_STATE_JUMPING && cur != ACTOR_STATE_FALLING)
		*as = cur;

	if (land_animation->isPlaying) {
		float crouch_rate = j->jump_max_blending_ratio * delta / j->land_anim_crouch;

		if (land_animation->time < j->land_anim_crouch) {
			if (ctx->entity->transform.position.z < LAND_ANIMATION_STARTING_HEIGHT) {
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

	if (cur == ACTOR_STATE_JUMPING && jump < j->jump_max_blending_ratio) {
		jump += j->jump_max_blending_ratio * delta / j->jump_anim_crouch;
		if (jump > j->jump_max_blending_ratio) jump = j->jump_max_blending_ratio;
	}

	ctx->animation->param[ANIMATION_PARAM_JUMP_L] = jump * (1.0f - footing);
	ctx->animation->param[ANIMATION_PARAM_JUMP_R] = jump * footing;
	ctx->animation->param[ANIMATION_PARAM_LAND_L] = land * (1.0f - footing);
	ctx->animation->param[ANIMATION_PARAM_LAND_R] = land * footing;
}

static void actorAnimation_setActiveNodes(const AnimationParamCtx *ctx)
{
	const AnimationDef *def = ctx->def;
	ActorAnimation *animation = ctx->animation;

	for (int i = 0; i < def->node_count; i++) {
		AnimationNodeType t = def->node[i].type;
		if (t == ANIMATION_NODE_SELECT || t == ANIMATION_NODE_SEQUENCE)
			animation->node_active[i] = (animation->param[def->node[i].param] != 0.0f);
	}
}

void actorAnimation_setParams(Entity *entity, const AnimationDef *def)
{
	Actor *actor  = entity->actor;
	ActorAnimation *animation = &actor->animation;
	const ActorMotionSettings *motion = &actor->motion.settings;
	float speed  = actor->motion.data.horizontal_speed;

	AnimationParamCtx ctx = {

		.entity = entity,
		.actor = actor,
		.animation = animation,
		.def = def,
		.settings = &animation->settings,
		.delta = time_get()->delta,
	};

	ctx.speed_state      = actorAnimation_getSpeedState(speed, motion);
	ctx.locomotion_param = actorAnimation_getLocomotionWeight(speed, ctx.speed_state, motion);
	ctx.locomotion_phase = actorAnimation_getLocomotionPhase(animation->clip[def->walk_animation].time, ctx.settings->standing_locomotion.walking_anim_length);
	ctx.turning          = actorAnimation_getTurningAvg(animation, entity->transform.rotation.z, actor->motion.data.previous_yaw);

	actorAnimation_speedSync[ctx.speed_state](&ctx);

	uint8_t prev_speed = animation->prev_speed_state;
	if (ctx.speed_state != prev_speed) {
		EntrySnapFn snap = actorAnimation_entrySnap[prev_speed][ctx.speed_state];
		if (snap) snap(&ctx);
		animation->prev_speed_state = ctx.speed_state;
	}

	actorAnimation_setLocomotionParam (&ctx);
	actorAnimation_setIdleRightParam (&ctx);
	actorAnimation_setTurningParam (&ctx);
	actorAnimation_setJumpParams (&ctx);
	actorAnimation_setRollParam (&ctx);
	actorAnimation_setActiveNodes (&ctx);
}

void actorAnimation_initGraph(Entity *entity, const AnimationDef *def)
{
	ActorAnimation *animation = &entity->actor->animation;
	const T3DModel *model = entity->mesh->model;
	entity->actor->animation_def = def;

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
		const AnimationClipDef *clip_def = &def->clip[i];

		animation->clip[i] = t3d_anim_create(model, clip_def->name);

		T3DSkeleton *target = (clip_def->buffer == ANIMATION_SLOT_MAIN)
			? &animation->main
			: &animation->buffer[clip_def->buffer];

		t3d_anim_attach(&animation->clip[i], target);
		t3d_anim_set_looping(&animation->clip[i], clip_def->is_looping);
		t3d_anim_set_playing(&animation->clip[i], clip_def->is_looping);
	}
}

void actorAnimation_evaluateGraph( const AnimationDef *def, const ActorAnimationSettings *settings, ActorAnimation *animation, float delta)
{
	ActorAnimationBuffer blend_buffer;
	blend_buffer.count = 0;

	for (int i = 0; i < def->node_count; i++)
	{
		if (!animation->node_active[i]) continue;

		const AnimationNode *node = &def->node[i];
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
					actorAnimation_addLayer(&blend_buffer, buf, param_val);
				}

				break;
			}

			case ANIMATION_NODE_LAYER:
			{
				float abs_val = fabsf(param_val);
				if (abs_val > 0.0f) {
					T3DSkeleton *buf = (node->buffer == ANIMATION_SLOT_MAIN) ? &animation->main : &animation->buffer[node->buffer];
					actorAnimation_addLayer(&blend_buffer, buf, abs_val);
				}

				break;
			}
		}
	}

	actorAnimation_blendLayers(&animation->main, &blend_buffer);

	(void)settings;
}

void actor_setAnimation(Entity *entity)
{
	Actor *actor = entity->actor;
	actorAnimation_setParams(entity, actor->animation_def);
	actorAnimation_evaluateGraph(actor->animation_def, &actor->animation.settings, &actor->animation, time_get()->delta);
	t3d_skeleton_update(&actor->animation.main);
}