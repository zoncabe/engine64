#include <math.h>
#include <malloc.h>
#include <string.h>
#include "../../include/time/time.h"
#include "../../include/entity/entity.h"
#include "../../include/viewport/viewport.h"


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

static uint8_t actorAnimation_getSpeedState(float speed, const ActorMotionSettings *m)
{
	if (speed <= 0.0f)                 return STANDING_IDLE;
	if (speed <= m->walk_target_speed) return WALKING;
	if (speed <= m->run_target_speed)  return RUNNING;
	return SPRINTING;
}

static float actorAnimation_getLocomotionWeight(float speed, uint8_t speed_state, const ActorMotionSettings *m)
{
	float p;
	switch (speed_state) {
		case WALKING:   p = speed / m->walk_target_speed; break;
		case RUNNING:   p = (speed - m->walk_target_speed) / (m->run_target_speed    - m->walk_target_speed); break;
		case SPRINTING: p = (speed - m->run_target_speed)  / (m->sprint_target_speed - m->run_target_speed); break;
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

static void actorAnimation_setJumpFootingSpeed(const AnimParamCtx *ctx)
{
	uint8_t cur = ctx->actor->state.current;
	if (cur != JUMPING && cur != FALLING) return;

	float jump   = ctx->anim->param[ANIM_PARAM_JUMP_L] + ctx->anim->param[ANIM_PARAM_JUMP_R];
	float factor = ctx->settings->jump.jump_footing_speed * (1.0f - jump);
	if (factor < 0.0f) factor = 0.0f;

	ctx->anim->animation[ctx->def->walk_anim].speed          *= factor;
	ctx->anim->animation[ctx->def->run_anim].speed           *= factor;
	ctx->anim->animation[ctx->def->sprint_anim].speed        *= factor;
	ctx->anim->animation[ctx->def->turn_walk_anim].speed     *= factor;
	ctx->anim->animation[ctx->def->turn_walk_anim + 1].speed *= factor;
	ctx->anim->animation[ctx->def->turn_run_anim].speed      *= factor;
	ctx->anim->animation[ctx->def->turn_run_anim + 1].speed  *= factor;
}

static void actorAnimation_setIdleSpeed(const AnimParamCtx *ctx) { (void)ctx; }

static void actorAnimation_setWalkingSpeed(const AnimParamCtx *ctx)
{
	float p = ctx->locomotion_param;
	t3d_anim_set_speed(&ctx->anim->animation[ctx->def->walk_anim],          p);
	t3d_anim_set_speed(&ctx->anim->animation[ctx->def->turn_walk_anim],     p);
	t3d_anim_set_speed(&ctx->anim->animation[ctx->def->turn_walk_anim + 1], p);
	actorAnimation_setJumpFootingSpeed(ctx);
}

static void actorAnimation_setRunningSpeed(const AnimParamCtx *ctx)
{
	const ActorAnimationLocomotionSettings *l = &ctx->settings->standing_locomotion;
	float run  = l->walk_to_run_ratio + (1.0f - l->walk_to_run_ratio) * ctx->locomotion_param;
	float walk = run * l->run_to_walk_ratio;
	t3d_anim_set_speed(&ctx->anim->animation[ctx->def->run_anim],           run);
	t3d_anim_set_speed(&ctx->anim->animation[ctx->def->walk_anim],          walk);
	t3d_anim_set_speed(&ctx->anim->animation[ctx->def->turn_run_anim],      run);
	t3d_anim_set_speed(&ctx->anim->animation[ctx->def->turn_run_anim  + 1], run);
	t3d_anim_set_speed(&ctx->anim->animation[ctx->def->turn_walk_anim],     walk);
	t3d_anim_set_speed(&ctx->anim->animation[ctx->def->turn_walk_anim + 1], walk);
	actorAnimation_setJumpFootingSpeed(ctx);
}

static void actorAnimation_setSprintingSpeed(const AnimParamCtx *ctx)
{
	const ActorAnimationLocomotionSettings *l = &ctx->settings->standing_locomotion;
	float sprint = l->run_to_sprint_ratio + (1.0f - l->run_to_sprint_ratio) * ctx->locomotion_param;
	float run    = sprint * l->sprint_to_run_ratio;
	t3d_anim_set_speed(&ctx->anim->animation[ctx->def->sprint_anim],        sprint);
	t3d_anim_set_speed(&ctx->anim->animation[ctx->def->run_anim],           run);
	t3d_anim_set_speed(&ctx->anim->animation[ctx->def->turn_run_anim],      run);
	t3d_anim_set_speed(&ctx->anim->animation[ctx->def->turn_run_anim  + 1], run);
	actorAnimation_setJumpFootingSpeed(ctx);
}

typedef void (*SpeedSyncFn)(const AnimParamCtx *);
static const SpeedSyncFn actorAnimation_speedSync[4] = {
	[STANDING_IDLE] = actorAnimation_setIdleSpeed,
	[WALKING]       = actorAnimation_setWalkingSpeed,
	[RUNNING]       = actorAnimation_setRunningSpeed,
	[SPRINTING]     = actorAnimation_setSprintingSpeed,
};

static void actorAnimation_snapWalkToRun(const AnimParamCtx *ctx)
{
	const ActorAnimationLocomotionSettings *l = &ctx->settings->standing_locomotion;
	T3DAnim *walk = &ctx->anim->animation[ctx->def->walk_anim];
	T3DAnim *run  = &ctx->anim->animation[ctx->def->run_anim];

	t3d_anim_set_time(run, walk->time * l->walk_to_run_ratio);
	t3d_anim_set_time(&ctx->anim->animation[ctx->def->turn_run_anim],     run->time);
	t3d_anim_set_time(&ctx->anim->animation[ctx->def->turn_run_anim + 1], run->time);
}

static void actorAnimation_snapRunToSprint(const AnimParamCtx *ctx)
{
	const ActorAnimationLocomotionSettings *l = &ctx->settings->standing_locomotion;
	T3DAnim *run    = &ctx->anim->animation[ctx->def->run_anim];
	T3DAnim *sprint = &ctx->anim->animation[ctx->def->sprint_anim];

	t3d_anim_set_time(sprint, run->time * l->run_to_sprint_ratio);
}

static void actorAnimation_snapSprintToRun(const AnimParamCtx *ctx)
{
	const ActorAnimationLocomotionSettings *l = &ctx->settings->standing_locomotion;
	T3DAnim *sprint = &ctx->anim->animation[ctx->def->sprint_anim];
	T3DAnim *walk   = &ctx->anim->animation[ctx->def->walk_anim];

	t3d_anim_set_time(walk, sprint->time * l->sprint_to_walk_ratio);
	t3d_anim_set_time(&ctx->anim->animation[ctx->def->turn_walk_anim],     walk->time);
	t3d_anim_set_time(&ctx->anim->animation[ctx->def->turn_walk_anim + 1], walk->time);
}

static void actorAnimation_snapRunToWalk(const AnimParamCtx *ctx)
{
	T3DAnim *walk = &ctx->anim->animation[ctx->def->walk_anim];

	t3d_anim_set_time(&ctx->anim->animation[ctx->def->turn_walk_anim],     walk->time);
	t3d_anim_set_time(&ctx->anim->animation[ctx->def->turn_walk_anim + 1], walk->time);
}

static void actorAnimation_snapRollToLocomotion(const AnimParamCtx *ctx, bool left)
{
	const ActorAnimationLocomotionSettings *l = &ctx->settings->standing_locomotion;
	float snap = left ? l->running_anim_length : l->running_anim_length_half;
	t3d_anim_set_time(&ctx->anim->animation[ctx->def->run_anim],    snap);
	t3d_anim_set_time(&ctx->anim->animation[ctx->def->walk_anim],   snap * l->run_to_walk_ratio);
	t3d_anim_set_time(&ctx->anim->animation[ctx->def->sprint_anim], snap * l->run_to_sprint_ratio);
}

static void actorAnimation_syncTurnWalk(const AnimParamCtx *ctx, float walk)
{
	uint8_t idx = (walk < 0.0f) ? ctx->def->turn_walk_anim : ctx->def->turn_walk_anim + 1;
	t3d_anim_set_time(&ctx->anim->animation[idx], ctx->anim->animation[ctx->def->walk_anim].time);
}

static void actorAnimation_syncTurnRun(const AnimParamCtx *ctx, float run)
{
	uint8_t idx = (run < 0.0f) ? ctx->def->turn_run_anim : ctx->def->turn_run_anim + 1;
	t3d_anim_set_time(&ctx->anim->animation[idx], ctx->anim->animation[ctx->def->run_anim].time);
}

typedef void (*EntrySnapFn)(const AnimParamCtx *);
static const EntrySnapFn actorAnimation_entrySnap[4][4] = {
	[WALKING  ][RUNNING  ] = actorAnimation_snapWalkToRun,
	[RUNNING  ][WALKING  ] = actorAnimation_snapRunToWalk,
	[RUNNING  ][SPRINTING] = actorAnimation_snapRunToSprint,
	[SPRINTING][RUNNING  ] = actorAnimation_snapSprintToRun,
};

static void actorAnimation_setLocomotionParam(const AnimParamCtx *ctx)
{
	float p = ctx->locomotion_param;

	switch (ctx->speed_state)
	{
		case STANDING_IDLE:
			ctx->anim->param[ANIM_PARAM_WALK]   = 0.0f;
			ctx->anim->param[ANIM_PARAM_RUN]    = 0.0f;
			ctx->anim->param[ANIM_PARAM_SPRINT] = 0.0f;
			break;

		case WALKING:
			ctx->anim->param[ANIM_PARAM_WALK]   = p;
			ctx->anim->param[ANIM_PARAM_RUN]    = 0.0f;
			ctx->anim->param[ANIM_PARAM_SPRINT] = 0.0f;
			break;

		case RUNNING:
			ctx->anim->param[ANIM_PARAM_WALK]   = 1.0f - p;
			ctx->anim->param[ANIM_PARAM_RUN]    = p;
			ctx->anim->param[ANIM_PARAM_SPRINT] = 0.0f;
			break;

		case SPRINTING:
			ctx->anim->param[ANIM_PARAM_WALK]   = 0.0f;
			ctx->anim->param[ANIM_PARAM_RUN]    = 1.0f - p;
			ctx->anim->param[ANIM_PARAM_SPRINT] = p;
			break;
	}
}

static void actorAnimation_setIdleRightParam(const AnimParamCtx *ctx)
{
	ctx->anim->param[ANIM_PARAM_IDLE_RIGHT] =
		ctx->settings->standing_locomotion.action_idle_max_blending_ratio * ctx->locomotion_phase;
}

static float actorAnimation_getTurningAvg(ActorAnimation *anim, float current_yaw, float previous_yaw)
{
	float delta_yaw = current_yaw - previous_yaw;
	if (delta_yaw >  180.0f) delta_yaw -= 360.0f;
	if (delta_yaw <= -180.0f) delta_yaw += 360.0f;

	anim->turn_avg[anim->turn_avg_idx] = delta_yaw / 5.0f;
	anim->turn_avg_idx = (anim->turn_avg_idx + 1) % ANIM_TURN_AVG_COUNT;

	float sum = 0.0f;
	for (int i = 0; i < ANIM_TURN_AVG_COUNT; i++) sum += anim->turn_avg[i];
	float r = sum / ANIM_TURN_AVG_COUNT;

	if (r >  1.0f) r =  1.0f;
	if (r < -1.0f) r = -1.0f;
	if (fabsf(r) < 0.001f) r = 0.0f;
	return r;
}

static void actorAnimation_setTurningParam(const AnimParamCtx *ctx)
{
	float r = ctx->turning;
	float walk, run;

	switch (ctx->speed_state) {
		case WALKING:
			walk = r * ctx->locomotion_param;
			run  = 0.0f;
			break;
		case RUNNING:
			walk = r * (1.0f - ctx->locomotion_param);
			run  = r * ctx->locomotion_param;
			break;
		case SPRINTING:
			walk = 0.0f;
			run  = r;
			break;
		default:
			walk = 0.0f;
			run  = 0.0f;
			break;
	}

	float prev_walk = ctx->anim->param[ANIM_PARAM_TURN_WALK];
	if (walk != 0.0f && (prev_walk == 0.0f || (prev_walk < 0.0f) != (walk < 0.0f)))
		actorAnimation_syncTurnWalk(ctx, walk);

	float prev_run = ctx->anim->param[ANIM_PARAM_TURN_RUN];
	if (run != 0.0f && (prev_run == 0.0f || (prev_run < 0.0f) != (run < 0.0f)))
		actorAnimation_syncTurnRun(ctx, run);

	ctx->anim->param[ANIM_PARAM_TURN_WALK] = walk;
	ctx->anim->param[ANIM_PARAM_TURN_RUN]  = run;
}

static void actorAnimation_setRollParam(const AnimParamCtx *ctx)
{
	uint8_t  cur = ctx->actor->state.current;
	uint8_t *as  = &ctx->anim->action_state;

	if (cur != ROLLING) {
		if (*as == ROLLING) *as = cur;
		ctx->anim->param[ANIM_PARAM_ROLL_RUN]       = 0.0f;
		ctx->anim->param[ANIM_PARAM_ROLL_STAND]     = 0.0f;
		ctx->anim->param[ANIM_PARAM_ROLL_DIR]       = 0.0f;
		ctx->anim->param[ANIM_PARAM_STAND_ROLL_DIR] = 0.0f;
		return;
	}

	if (*as != ROLLING) {
		bool    stand = (ctx->actor->state.locomotion == STANDING_IDLE);
		uint8_t base  = stand ? ctx->def->stand_roll_anim : ctx->def->roll_anim;

		t3d_anim_set_playing(&ctx->anim->animation[base],     true);
		t3d_anim_set_time   (&ctx->anim->animation[base],     0.0f);
		t3d_anim_set_playing(&ctx->anim->animation[base + 1], true);
		t3d_anim_set_time   (&ctx->anim->animation[base + 1], 0.0f);

		float dir = (ctx->locomotion_phase < 0.5f) ? -1.0f : 1.0f;
		ctx->anim->param[ANIM_PARAM_ROLL_RUN]       = 0.0f;
		ctx->anim->param[ANIM_PARAM_ROLL_STAND]     = 0.0f;
		ctx->anim->param[ANIM_PARAM_ROLL_DIR]       = stand ? 0.0f : dir;
		ctx->anim->param[ANIM_PARAM_STAND_ROLL_DIR] = stand ? dir  : 0.0f;
		*as = ROLLING;
	}

	const ActorAnimationRollSettings *r = &ctx->settings->roll;
	bool  stand = ctx->anim->param[ANIM_PARAM_STAND_ROLL_DIR] != 0.0f;
	float dir   = stand ? ctx->anim->param[ANIM_PARAM_STAND_ROLL_DIR] : ctx->anim->param[ANIM_PARAM_ROLL_DIR];
	bool  left  = dir < 0.0f;
	uint8_t weight_param = stand ? ANIM_PARAM_ROLL_STAND : ANIM_PARAM_ROLL_RUN;

	uint8_t base      = stand ? ctx->def->stand_roll_anim : ctx->def->roll_anim;
	uint8_t roll_idx  = left ? base : base + 1;
	float   roll_time = ctx->anim->animation[roll_idx].time;
	float   ratio     = ctx->anim->param[weight_param];

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

	ctx->anim->param[weight_param] = ratio;
}

static void actorAnimation_syncLandToJump(const AnimParamCtx *ctx)
{
	const ActorAnimationJumpSettings *j = &ctx->settings->jump;
	T3DAnim *jump_l   = &ctx->anim->animation[ctx->def->jump_anim];
	T3DAnim *jump_r   = &ctx->anim->animation[ctx->def->jump_anim + 1];
	float    land_t   = ctx->anim->animation[ctx->def->land_anim].time;
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

static void actorAnimation_snapToJump(const AnimParamCtx *ctx)
{
	T3DAnim *jump_l   = &ctx->anim->animation[ctx->def->jump_anim];
	T3DAnim *jump_r   = &ctx->anim->animation[ctx->def->jump_anim + 1];
	T3DAnim *land_anim = &ctx->anim->animation[ctx->def->land_anim];

	T3DAnim *fall_l = &ctx->anim->animation[ctx->def->fall_anim];
	T3DAnim *fall_r = &ctx->anim->animation[ctx->def->fall_anim + 1];

	t3d_anim_set_playing(jump_l, true);
	t3d_anim_set_playing(jump_r, true);
	
	t3d_anim_set_time(fall_l, 0.0f);
	t3d_anim_set_time(fall_r, 0.0f);

	if (land_anim->isPlaying) {
		t3d_anim_set_time(jump_l, 0.0f);
		t3d_anim_set_time(jump_r, 0.0f);
	} else {
		actorAnimation_syncLandToJump(ctx);
	}

	ctx->anim->param[ANIM_PARAM_JUMP_L] = 0.0f;
	ctx->anim->param[ANIM_PARAM_JUMP_R] = 0.0f;
}

static void actorAnimation_snapToLand(const AnimParamCtx *ctx)
{
	T3DAnim *land_l = &ctx->anim->animation[ctx->def->land_anim];
	T3DAnim *land_r = &ctx->anim->animation[ctx->def->land_anim + 1];
	t3d_anim_set_time(land_l, 0.0f);
	t3d_anim_set_time(land_r, 0.0f);
	t3d_anim_set_playing(land_l, true);
	t3d_anim_set_playing(land_r, true);
	ctx->anim->param[ANIM_PARAM_LAND_L] = 0.0f;
	ctx->anim->param[ANIM_PARAM_LAND_R] = 0.0f;
}

static void actorAnimation_setJumpParams(const AnimParamCtx *ctx)
{
	const ActorAnimationJumpSettings *j   = &ctx->settings->jump;
	float    delta     = ctx->delta;
	T3DAnim *land_anim = &ctx->anim->animation[ctx->def->land_anim];
	uint8_t  cur       = ctx->actor->state.current;
	uint8_t *as        = &ctx->anim->action_state;
	float    footing   = ctx->locomotion_phase;

	float jump = ctx->anim->param[ANIM_PARAM_JUMP_L] + ctx->anim->param[ANIM_PARAM_JUMP_R];
	float land = ctx->anim->param[ANIM_PARAM_LAND_L] + ctx->anim->param[ANIM_PARAM_LAND_R];

	if (cur == JUMPING && *as != JUMPING) {
		actorAnimation_snapToJump(ctx);
		jump = 0.0f;
		*as  = JUMPING;
	}

	if (cur == FALLING && *as != FALLING) {
		actorAnimation_snapToLand(ctx);
		land = 0.0f;
		*as  = FALLING;
	}

	if ((*as == JUMPING || *as == FALLING) && cur != JUMPING && cur != FALLING)
		*as = cur;

	if (land_anim->isPlaying) {
		float crouch_rate = j->jump_max_blending_ratio * delta / j->land_anim_crouch;

		if (land_anim->time < j->land_anim_crouch) {
			if (ctx->entity->transform.position.z < LAND_ANIM_STARTING_HEIGHT) {
				land += crouch_rate;
				if (land > j->jump_max_blending_ratio) land = j->jump_max_blending_ratio;
			}
		} else {
			float stand_rate = j->jump_max_blending_ratio * delta / (j->land_anim_length - j->land_anim_crouch);
			land -= stand_rate;
			if (land < 0.0f) {
				land = 0.0f;
				t3d_anim_set_playing(&ctx->anim->animation[ctx->def->land_anim],     false);
				t3d_anim_set_playing(&ctx->anim->animation[ctx->def->land_anim + 1], false);
			}
		}

		jump -= crouch_rate;
		if (jump < 0.0f) jump = 0.0f;
	}

	if (cur == JUMPING && jump < j->jump_max_blending_ratio) {
		jump += j->jump_max_blending_ratio * delta / j->jump_anim_crouch;
		if (jump > j->jump_max_blending_ratio) jump = j->jump_max_blending_ratio;
	}

	ctx->anim->param[ANIM_PARAM_JUMP_L] = jump * (1.0f - footing);
	ctx->anim->param[ANIM_PARAM_JUMP_R] = jump * footing;
	ctx->anim->param[ANIM_PARAM_LAND_L] = land * (1.0f - footing);
	ctx->anim->param[ANIM_PARAM_LAND_R] = land * footing;
}

static void actorAnimation_setActiveNodes(const AnimParamCtx *ctx)
{
	const AnimDef *def = ctx->def;
	ActorAnimation *anim = ctx->anim;

	for (uint8_t i = 0; i < def->node_count; i++) {
		AnimNodeType t = def->node[i].type;
		if (t == ANIM_NODE_SELECT || t == ANIM_NODE_SEQUENCE)
			anim->node_active[i] = (anim->param[def->node[i].param] != 0.0f);
	}
}

void actorAnimation_setParams(Entity *entity, const AnimDef *def)
{
	Actor *actor  = entity->actor;
	ActorAnimation *anim   = &actor->animation;
	const ActorMotionSettings *motion = &actor->motion.settings;
	float speed  = actor->motion.data.horizontal_speed;

	AnimParamCtx ctx = {

		.entity = entity,
		.actor = actor,
		.anim = anim,
		.def = def,
		.settings = &anim->settings,
		.delta = time_get()->delta,
	};

	ctx.speed_state      = actorAnimation_getSpeedState(speed, motion);
	ctx.locomotion_param = actorAnimation_getLocomotionWeight(speed, ctx.speed_state, motion);
	ctx.locomotion_phase = actorAnimation_getLocomotionPhase(anim->animation[def->walk_anim].time, ctx.settings->standing_locomotion.walking_anim_length);
	ctx.turning          = actorAnimation_getTurningAvg(anim, entity->transform.rotation.z, actor->motion.data.previous_yaw);

	actorAnimation_speedSync[ctx.speed_state](&ctx);

	uint8_t prev_speed = anim->prev_speed_state;
	if (ctx.speed_state != prev_speed) {
		EntrySnapFn snap = actorAnimation_entrySnap[prev_speed][ctx.speed_state];
		if (snap) snap(&ctx);
		anim->prev_speed_state = ctx.speed_state;
	}

	actorAnimation_setLocomotionParam (&ctx);
	actorAnimation_setIdleRightParam (&ctx);
	actorAnimation_setTurningParam (&ctx);
	actorAnimation_setJumpParams (&ctx);
	actorAnimation_setRollParam (&ctx);
	actorAnimation_setActiveNodes (&ctx);
}

void actorAnimation_initGraph(Entity *entity, const AnimDef *def)
{
	ActorAnimation *anim  = &entity->actor->animation;
	const T3DModel *model = entity->mesh->model;
	entity->actor->anim_def = def;

	anim->main = t3d_skeleton_create_buffered(model, FB_COUNT);

	anim->buffer = malloc(def->buffer_count * sizeof(T3DSkeleton));
	anim->animation = malloc(def->clip_count * sizeof(T3DAnim));
	anim->node_state = malloc(def->node_count * sizeof(uint8_t));
	anim->node_active = malloc(def->node_count * sizeof(bool));

	for (uint8_t i = 0; i < def->buffer_count; i++)
		anim->buffer[i] = t3d_skeleton_clone(&anim->main, false);

	memset(anim->node_state,   0,    def->node_count * sizeof(uint8_t));
	memset(anim->node_active,  true, def->node_count * sizeof(bool));
	memset(anim->turn_avg,  0,    sizeof(anim->turn_avg));
	anim->turn_avg_idx = 0;

	for (uint8_t i = 0; i < def->clip_count; i++)
	{
		const AnimClipDef *clip_def = &def->clip[i];

		anim->animation[i] = t3d_anim_create(model, clip_def->name);

		T3DSkeleton *target = (clip_def->buffer == ANIM_SLOT_MAIN)
			? &anim->main
			: &anim->buffer[clip_def->buffer];

		t3d_anim_attach(&anim->animation[i], target);
		t3d_anim_set_looping(&anim->animation[i], clip_def->looping);
		t3d_anim_set_playing(&anim->animation[i], clip_def->looping);
	}
}

void actorAnimation_evaluateGraph( const AnimDef *def, const ActorAnimationSettings *settings, ActorAnimation *anim, float delta)
{
	ActorAnimationBuffer blend_buffer;
	blend_buffer.count = 0;

	for (uint8_t i = 0; i < def->node_count; i++)
	{
		if (!anim->node_active[i]) continue;

		const AnimNode *node = &def->node[i];
		T3DAnim *clip = &anim->animation[node->anim];
		float param_val = anim->param[node->param];

		switch (node->type)
		{
			case ANIM_NODE_CLIP:
			{
				t3d_anim_update(clip, delta);
				break;
			}

			case ANIM_NODE_SELECT:
			{
				uint8_t active = (param_val < 0.0f) ? node->anim : node->anim2;
				uint8_t inactive = (param_val < 0.0f) ? node->anim2 : node->anim;

				if (anim->node_state[i] != active)
				{
					anim->node_state[i] = active;
					t3d_anim_set_time(&anim->animation[inactive], anim->animation[active].time);
				}

				t3d_anim_update(&anim->animation[active], delta);
				break;
			}

			case ANIM_NODE_SEQUENCE:
			{
				T3DAnim *clip2 = &anim->animation[node->anim2];
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

			case ANIM_NODE_BLEND:
			{
				if (param_val > 0.0f) {
					T3DSkeleton *buf = (node->buffer == ANIM_SLOT_MAIN) ? &anim->main : &anim->buffer[node->buffer];
					t3d_anim_update(clip, delta);
					actorAnimation_addLayer(&blend_buffer, buf, param_val);
				}

				break;
			}

			case ANIM_NODE_LAYER:
			{
				float abs_val = fabsf(param_val);
				if (abs_val > 0.0f) {
					T3DSkeleton *buf = (node->buffer == ANIM_SLOT_MAIN) ? &anim->main : &anim->buffer[node->buffer];
					actorAnimation_addLayer(&blend_buffer, buf, abs_val);
				}

				break;
			}
		}
	}

	actorAnimation_blendLayers(&anim->main, &blend_buffer);

	(void)settings;
}

void actor_setAnimation(Entity *entity)
{
    Actor *actor = entity->actor;
    actorAnimation_setParams(entity, actor->anim_def);
    actorAnimation_evaluateGraph(actor->anim_def, &actor->animation.settings, &actor->animation, time_get()->delta);
    t3d_skeleton_update(&actor->animation.main);
}