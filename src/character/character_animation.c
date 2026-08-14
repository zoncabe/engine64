#include <assert.h>
#include <math.h>
#include <fmath.h>
#include <malloc.h>
#include <string.h>
#include "time/time.h"
#include "entity/entity.h"
#include "viewport/viewport.h"
#include "physics/math/math_common.h"


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

static uint8_t characterAnimation_blendSegment(float weight, uint8_t count, float *t);
static void characterAnimation_syncGridClips(const CharacterAnimationParamCtx *ctx, const CharacterAnimationNode *node, float cols_value, float rows_value);

/* gait axis: gait i sits at i / (count - 1) */
static float characterAnimation_getGaitParam(float speed, const CharacterMovementSettings *movement)
{
	uint8_t last = movement->gait_count - 1;
	if (last == 0 || speed <= movement->gait[0].target_speed) return 0.0f;

	for (uint8_t i = 0; i < last; i++) {
		float lo = movement->gait[i].target_speed;
		float hi = movement->gait[i + 1].target_speed;
		if (speed > hi) continue;
		return (i + (speed - lo) / (hi - lo)) / last;
	}
	return 1.0f;
}

/* grid weight over the idle: covers 0 to the first gait */
static float characterAnimation_getWalkWeight(float speed, const CharacterMovementSettings *movement)
{
	float first = movement->gait[0].target_speed;
	if (first <= 0.0f) return (speed > 0.0f) ? 1.0f : 0.0f;
	if (speed >= first) return 1.0f;
	return speed / first;
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

/* the grid runs at the cycle rate the current gait asks for: real speed over
   target speed, and every clip gets the speed that makes its cycle last that long */
static void characterAnimation_setLocomotionSpeed(const CharacterAnimationParamCtx *ctx)
{
	const CharacterAnimationNode *node = &ctx->def->node[ctx->def->locomotion_node];
	const CharacterMovementSettings *movement = ctx->character->movement.settings;
	CharacterAnimation *animation = ctx->animation;
	uint8_t center = node->cols / 2;

	float t;
	uint8_t row = characterAnimation_blendSegment(ctx->gait_param, node->rows, &t);

	float low  = t3d_anim_get_length(&animation->clip[node->animation[row * node->cols + center]]);
	float high = t3d_anim_get_length(&animation->clip[node->animation[(row + 1) * node->cols + center]]);
	float length = low + t * (high - low);

	float target = movement->gait[row].target_speed
	             + t * (movement->gait[row + 1].target_speed - movement->gait[row].target_speed);

	if (length <= 0.0f || target <= 0.0f) return;

	float scale = (ctx->character->movement.data.horizontal_speed / target) / length;

	for (int i = 0; i < node->cols * node->rows; i++) {
		T3DAnim *clip = &animation->clip[node->animation[i]];
		t3d_anim_set_speed(clip, t3d_anim_get_length(clip) * scale);
	}

	characterAnimation_setJumpFootingSpeed(ctx);
}

static void characterAnimation_snapRollToLocomotion(const CharacterAnimationParamCtx *ctx, bool left)
{
	const CharacterAnimationNode *node = &ctx->def->node[ctx->def->locomotion_node];
	float phase = left ? 0.0f : 0.5f;

	for (int i = 0; i < node->cols * node->rows; i++) {
		T3DAnim *clip = &ctx->animation->clip[node->animation[i]];
		t3d_anim_set_time(clip, phase * t3d_anim_get_length(clip));
	}
}

static void characterAnimation_setLocomotionParam(const CharacterAnimationParamCtx *ctx)
{
	const CharacterAnimationNode *node = &ctx->def->node[ctx->def->locomotion_node];
	const CharacterMovementSettings *movement = ctx->character->movement.settings;
	float speed = ctx->character->movement.data.horizontal_speed;

	/* turn axis: 0 left, 0.5 straight, 1 right */
	float turn = (ctx->turning + 1.0f) * 0.5f;

	characterAnimation_syncGridClips(ctx, node, turn, ctx->gait_param);

	ctx->animation->param[ANIMATION_PARAM_WALK]      = characterAnimation_getWalkWeight(speed, movement);
	ctx->animation->param[ANIMATION_PARAM_WALK_GAIT] = ctx->gait_param;
	ctx->animation->param[ANIMATION_PARAM_WALK_TURN] = turn;
}

/* standing still the footing no longer moves, so the idle profile holds */
static void characterAnimation_setIdleRightParam(const CharacterAnimationParamCtx *ctx)
{
	if (ctx->character->movement.data.horizontal_speed <= 0.0f) return;

	ctx->animation->param[ANIMATION_PARAM_IDLE_RIGHT] =
		ctx->settings->action_idle_max_blending_ratio * ctx->locomotion_phase;
}

static float characterAnimation_getTurningAvg(CharacterAnimation *animation, const CharacterAnimationSettings *settings, float current_yaw, float previous_yaw)
{
	float delta_yaw = current_yaw - previous_yaw;
	if (delta_yaw >  180.0f) delta_yaw -= 360.0f;
	if (delta_yaw <= -180.0f) delta_yaw += 360.0f;

	animation->turn_avg[animation->turn_avg_idx] = delta_yaw;
	animation->turn_avg_idx = (animation->turn_avg_idx + 1) % ANIMATION_TURN_AVG_COUNT;

	float sum = 0.0f;
	for (int i = 0; i < ANIMATION_TURN_AVG_COUNT; i++) sum += animation->turn_avg[i];
	float avg_delta_yaw = sum / ANIMATION_TURN_AVG_COUNT;

	float r = avg_delta_yaw / settings->turn_max_angle;

	if (r >  1.0f) r =  1.0f;
	if (r < -1.0f) r = -1.0f;
	if (fabsf(r) < 0.001f) r = 0.0f;
	return r * settings->turn_max_weight;
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

static float characterAnimation_getStrafeDirectionWeight(const CharacterAnimationParamCtx *ctx)
{
	const CharacterAnimationSettings *s = ctx->settings;
	const KinematicBody *body = &ctx->character->body;

	if (body->velocity.x == 0.0f && body->velocity.y == 0.0f)
		return ctx->animation->param[ANIMATION_PARAM_STRAFE_DIR];

	float velocity_yaw = rad_to_deg(fm_atan2f(-body->velocity.x, -body->velocity.y));
	float rel = angle_wrap_relative(velocity_yaw, body->rotation.z) - body->rotation.z;

	// eje:    back 0 | back_l 1/6 | strafe_l 2/6 | fwd 3/6 | strafe_r 4/6 | back_r 5/6 | back 1
	// anclas: fwd 0º, strafe ±90º, back_l/back_r ±90º del lado trasero, back ±180º
	// los tramos 1/6-2/6 y 4/6-5/6 no se recorren por direccion: son el giro de cadera
	float raw;
	if      (rel < -90.0f) raw = (rel + 180.0f) / 90.0f          * (1.0f / 6.0f);
	else if (rel <   0.0f) raw = (2.0f + (rel + 90.0f) / 90.0f)  * (1.0f / 6.0f);
	else if (rel <  90.0f) raw = (3.0f + rel / 90.0f)            * (1.0f / 6.0f);
	else                   raw = (5.0f + (rel - 90.0f) / 90.0f)  * (1.0f / 6.0f);

	if (ctx->animation->param[ANIMATION_PARAM_STRAFE] == 0.0f) return raw;

	float out = ctx->animation->param[ANIMATION_PARAM_STRAFE_DIR];

	bool front_raw = (raw >= 2.0f / 6.0f && raw <= 4.0f / 6.0f);
	bool front_out = (out >= 2.0f / 6.0f && out <= 4.0f / 6.0f);

	if (!ctx->animation->strafe_turning) {
		if (front_raw == front_out) return raw;
		ctx->animation->strafe_turning = true;
	}

	// ends 0 and 1 of the axis are the same clip: if the target is more than half
	// the axis away, the real shortest path crosses the seam
	if (raw - out > 0.5f) raw -= 1.0f;
	if (out - raw > 0.5f) raw += 1.0f;

	// exponential lerp toward the live weight, released once it lands
	float factor = fm_expf(-s->strafe_turn_rate * ctx->delta);
	out = out * factor + raw * (1.0f - factor);

	if (fabsf(out - raw) < 0.001f) {
		out = raw;
		ctx->animation->strafe_turning = false;
	}

	if (out < 0.0f) out += 1.0f;
	if (out > 1.0f) out -= 1.0f;

	return out;
}

static void characterAnimation_snapStrafeEntry(const CharacterAnimationParamCtx *ctx)
{
	const CharacterAnimationNode *node = &ctx->def->node[ctx->def->strafe_node];

	T3DAnim *src = &ctx->animation->clip[ctx->def->walk_animation];
	float src_length = t3d_anim_get_length(src);
	if (src_length <= 0.0f) return;
	float phase = src->time / src_length;

	for (int c = 0; c < node->cols * node->rows; c++) {
		T3DAnim *dst = &ctx->animation->clip[node->animation[c]];
		if (dst == src) continue;
		t3d_anim_set_time(dst, phase * t3d_anim_get_length(dst));
	}
}

static void characterAnimation_snapStrafeExit(const CharacterAnimationParamCtx *ctx)
{
	const CharacterAnimationNode *node = &ctx->def->node[ctx->def->strafe_node];

	// fuente: el clip de la fila walk mas cercano al weight actual, que venia activo
	float out = ctx->animation->param[ANIMATION_PARAM_STRAFE_DIR];
	uint8_t src_col = (uint8_t)(out * (node->cols - 1) + 0.5f);
	if (src_col > node->cols - 1) src_col = node->cols - 1;

	T3DAnim *src = &ctx->animation->clip[node->animation[src_col]];
	float src_length = t3d_anim_get_length(src);
	if (src_length <= 0.0f) return;
	float phase = src->time / src_length;

	const CharacterAnimationDef *def = ctx->def;
	const uint8_t target[] = {
		def->walk_animation, def->run_animation, def->sprint_animation,
		def->turn_walk_animation, (uint8_t)(def->turn_walk_animation + 1),
		def->turn_run_animation,  (uint8_t)(def->turn_run_animation + 1),
	};

	for (unsigned t = 0; t < sizeof(target); t++) {
		T3DAnim *dst = &ctx->animation->clip[target[t]];
		t3d_anim_set_time(dst, phase * t3d_anim_get_length(dst));
	}
}

/* phase carry: clips entering the grid start where the ones leaving it were */
static void characterAnimation_syncGridClips(const CharacterAnimationParamCtx *ctx, const CharacterAnimationNode *node, float cols_value, float rows_value)
{
	CharacterAnimation *animation = ctx->animation;
	float tx, ty;
	uint8_t col, row;

	// esquinas del frame anterior
	uint8_t prev[4];
	uint8_t prev_count = 0;
	col = characterAnimation_blendSegment(animation->param[node->param_cols], node->cols, &tx);
	row = characterAnimation_blendSegment(animation->param[node->param_rows], node->rows, &ty);
	prev[prev_count++] = node->animation[row * node->cols + col];
	if (tx > 0.0f)              prev[prev_count++] = node->animation[row * node->cols + col + 1];
	if (ty > 0.0f)              prev[prev_count++] = node->animation[(row + 1) * node->cols + col];
	if (tx > 0.0f && ty > 0.0f) prev[prev_count++] = node->animation[(row + 1) * node->cols + col + 1];

	// esquinas del frame nuevo
	uint8_t curr[4];
	uint8_t curr_count = 0;
	col = characterAnimation_blendSegment(cols_value, node->cols, &tx);
	row = characterAnimation_blendSegment(rows_value, node->rows, &ty);
	curr[curr_count++] = node->animation[row * node->cols + col];
	if (tx > 0.0f)              curr[curr_count++] = node->animation[row * node->cols + col + 1];
	if (ty > 0.0f)              curr[curr_count++] = node->animation[(row + 1) * node->cols + col];
	if (tx > 0.0f && ty > 0.0f) curr[curr_count++] = node->animation[(row + 1) * node->cols + col + 1];

	// referencia de fase: una esquina que siga participando; si no queda ninguna, la base anterior
	T3DAnim *ref = NULL;
	for (uint8_t m = 0; m < curr_count && !ref; m++)
		for (uint8_t p = 0; p < prev_count; p++)
			if (curr[m] == prev[p]) { ref = &animation->clip[curr[m]]; break; }
	if (!ref) ref = &animation->clip[prev[0]];

	float ref_length = t3d_anim_get_length(ref);
	if (ref_length <= 0.0f) return;
	float phase = ref->time / ref_length;

	for (uint8_t m = 0; m < curr_count; m++) {
		bool carried = false;
		for (uint8_t p = 0; p < prev_count; p++)
			if (curr[m] == prev[p]) carried = true;
		if (carried) continue;

		T3DAnim *dst = &animation->clip[curr[m]];
		t3d_anim_set_time(dst, phase * t3d_anim_get_length(dst));
	}
}

static void characterAnimation_setStrafeParams(const CharacterAnimationParamCtx *ctx)
{
	CharacterAnimation *animation = ctx->animation;
	const CharacterMovementData *data = &ctx->character->movement.data;
	const CharacterMovementSettings *movement = ctx->character->movement.settings;

	bool strafing = data->strafe
		&& characterMovement_isLocomotion(ctx->character->movement.current)
		&& data->horizontal_speed > 0.0f;

	/* the strafe grid takes over the locomotion one gradually */
	float prev_blend = animation->strafe_blend;
	float factor = fm_expf(-ctx->settings->strafe_blend_rate * ctx->delta);
	float blend  = strafing ? 1.0f - (1.0f - prev_blend) * factor : prev_blend * factor;
	if (blend > 0.999f) blend = 1.0f;
	if (blend < 0.001f) blend = 0.0f;

	if (blend == 0.0f) {
		if (prev_blend > 0.0f) characterAnimation_snapStrafeExit(ctx);
		animation->strafe_blend = 0.0f;
		animation->param[ANIMATION_PARAM_STRAFE] = 0.0f;
		animation->strafe_turning = false;
		return;
	}

	if (prev_blend == 0.0f)
		characterAnimation_snapStrafeEntry(ctx);

	const CharacterAnimationNode *node = &ctx->def->node[ctx->def->strafe_node];
	float walk_speed = animation->clip[ctx->def->walk_animation].speed;
	float run_speed  = animation->clip[ctx->def->run_animation].speed;
	for (uint8_t r = 0; r < node->rows; r++) {
		float row_speed = (r == 0) ? walk_speed : run_speed;
		for (uint8_t c = 0; c < node->cols; c++)
			t3d_anim_set_speed(&animation->clip[node->animation[r * node->cols + c]], row_speed);
	}

	float dir = characterAnimation_getStrafeDirectionWeight(ctx);

	float gait = (data->horizontal_speed - movement->gait[0].target_speed)
	           / (movement->gait[1].target_speed - movement->gait[0].target_speed);
	if (gait < 0.0f) gait = 0.0f;
	if (gait > 1.0f) gait = 1.0f;

	characterAnimation_syncGridClips(ctx, node, dir, gait);

	float weight = characterAnimation_getWalkWeight(data->horizontal_speed, movement);

	animation->strafe_blend = blend;
	animation->param[ANIMATION_PARAM_STRAFE]      = weight * blend;
	animation->param[ANIMATION_PARAM_STRAFE_DIR]  = dir;
	animation->param[ANIMATION_PARAM_STRAFE_GAIT] = gait;

	animation->param[ANIMATION_PARAM_WALK] = weight * (1.0f - blend);
}

// eje: back 0 | left 1/4 | fwd 2/4 | right 3/4 | back 1
static float characterAnimation_getStrafeLockedDirectionWeight(const CharacterAnimationParamCtx *ctx)
{
	const KinematicBody *body = &ctx->character->body;

	if (body->velocity.x == 0.0f && body->velocity.y == 0.0f)
		return ctx->animation->param[ANIMATION_PARAM_STRAFE_LOCKED_DIR];

	float velocity_yaw = rad_to_deg(fm_atan2f(-body->velocity.x, -body->velocity.y));
	float rel = angle_wrap_relative(velocity_yaw, body->rotation.z) - body->rotation.z;

	return (rel + 180.0f) / 360.0f;
}

static void characterAnimation_snapStrafeLockedEntry(const CharacterAnimationParamCtx *ctx)
{
	const CharacterAnimationNode *node = &ctx->def->node[ctx->def->strafe_locked_node];

	T3DAnim *src = &ctx->animation->clip[ctx->def->walk_animation];
	float src_length = t3d_anim_get_length(src);
	if (src_length <= 0.0f) return;
	float phase = src->time / src_length;

	for (int c = 0; c < node->cols * node->rows; c++) {
		T3DAnim *dst = &ctx->animation->clip[node->animation[c]];
		t3d_anim_set_time(dst, phase * t3d_anim_get_length(dst));
	}
}

static void characterAnimation_snapStrafeLockedExit(const CharacterAnimationParamCtx *ctx)
{
	const CharacterAnimationNode *node = &ctx->def->node[ctx->def->strafe_locked_node];

	// fuente: el clip de la fila walk mas cercano al weight actual
	float out = ctx->animation->param[ANIMATION_PARAM_STRAFE_LOCKED_DIR];
	uint8_t src_col = (uint8_t)(out * (node->cols - 1) + 0.5f);
	if (src_col > node->cols - 1) src_col = node->cols - 1;

	T3DAnim *src = &ctx->animation->clip[node->animation[src_col]];
	float src_length = t3d_anim_get_length(src);
	if (src_length <= 0.0f) return;
	float phase = src->time / src_length;

	const CharacterAnimationDef *def = ctx->def;
	const uint8_t target[] = {
		def->walk_animation, def->run_animation, def->sprint_animation,
		def->turn_walk_animation, (uint8_t)(def->turn_walk_animation + 1),
		def->turn_run_animation,  (uint8_t)(def->turn_run_animation + 1),
	};

	for (unsigned t = 0; t < sizeof(target); t++) {
		T3DAnim *dst = &ctx->animation->clip[target[t]];
		t3d_anim_set_time(dst, phase * t3d_anim_get_length(dst));
	}
}

// fase del col dominante (fila walk) de un grid hacia todos los clips del otro
static void characterAnimation_snapGridFromGrid(const CharacterAnimationParamCtx *ctx, uint8_t src_node_idx, float src_dir, uint8_t dst_node_idx)
{
	const CharacterAnimationNode *src_node = &ctx->def->node[src_node_idx];
	const CharacterAnimationNode *dst_node = &ctx->def->node[dst_node_idx];

	uint8_t src_col = (uint8_t)(src_dir * (src_node->cols - 1) + 0.5f);
	if (src_col > src_node->cols - 1) src_col = src_node->cols - 1;

	T3DAnim *src = &ctx->animation->clip[src_node->animation[src_col]];
	float src_length = t3d_anim_get_length(src);
	if (src_length <= 0.0f) return;
	float phase = src->time / src_length;

	for (int c = 0; c < dst_node->cols * dst_node->rows; c++) {
		T3DAnim *dst = &ctx->animation->clip[dst_node->animation[c]];
		t3d_anim_set_time(dst, phase * t3d_anim_get_length(dst));
	}
}

static void characterAnimation_setStrafeLockedParams(const CharacterAnimationParamCtx *ctx)
{
	CharacterAnimation *animation = ctx->animation;
	const CharacterMovementData *data = &ctx->character->movement.data;
	const CharacterMovementSettings *movement = ctx->character->movement.settings;

	bool locked = data->strafe_locked
		&& characterMovement_isLocomotion(ctx->character->movement.current)
		&& data->horizontal_speed > 0.0f;

	float prev_blend = animation->strafe_locked_blend;
	float factor = fm_expf(-ctx->settings->strafe_locked_blend_rate * ctx->delta);
	float blend  = locked ? 1.0f - (1.0f - prev_blend) * factor : prev_blend * factor;
	if (blend > 0.999f) blend = 1.0f;
	if (blend < 0.001f) blend = 0.0f;

	if (blend == 0.0f) {
		if (prev_blend > 0.0f) characterAnimation_snapStrafeLockedExit(ctx);
		animation->strafe_locked_blend = 0.0f;
		animation->param[ANIMATION_PARAM_STRAFE_LOCKED] = 0.0f;
		return;
	}

	if (prev_blend == 0.0f)
		characterAnimation_snapStrafeLockedEntry(ctx);

	const CharacterAnimationNode *node = &ctx->def->node[ctx->def->strafe_locked_node];
	float walk_speed = animation->clip[ctx->def->walk_animation].speed;
	float run_speed  = animation->clip[ctx->def->run_animation].speed;
	for (uint8_t r = 0; r < node->rows; r++) {
		float row_speed = (r == 0) ? walk_speed : run_speed;
		for (uint8_t c = 0; c < node->cols; c++)
			t3d_anim_set_speed(&animation->clip[node->animation[r * node->cols + c]], row_speed);
	}

	float dir = characterAnimation_getStrafeLockedDirectionWeight(ctx);

	float gait = (data->horizontal_speed - movement->gait[0].target_speed)
	           / (movement->gait[1].target_speed - movement->gait[0].target_speed);
	if (gait < 0.0f) gait = 0.0f;
	if (gait > 1.0f) gait = 1.0f;

	characterAnimation_syncGridClips(ctx, node, dir, gait);

	float weight = characterAnimation_getWalkWeight(data->horizontal_speed, movement);

	animation->strafe_locked_blend = blend;
	animation->param[ANIMATION_PARAM_STRAFE_LOCKED]      = weight * blend;
	animation->param[ANIMATION_PARAM_STRAFE_LOCKED_DIR]  = dir;
	animation->param[ANIMATION_PARAM_STRAFE_LOCKED_GAIT] = gait;

	animation->param[ANIMATION_PARAM_WALK]   *= (1.0f - blend);
	animation->param[ANIMATION_PARAM_STRAFE] *= (1.0f - blend);
}

static void characterAnimation_setActiveNodes(const CharacterAnimationParamCtx *ctx)
{
	const CharacterAnimationDef *def = ctx->def;
	CharacterAnimation *animation = ctx->animation;

	for (int i = 0; i < def->node_count; i++) {
		CharacterAnimationNodeType t = def->node[i].type;
		if (t == ANIMATION_NODE_SELECT || t == ANIMATION_NODE_SEQUENCE)
			animation->node_active[i] = (animation->param[def->node[i].param_cols] != 0.0f);
		if (t == ANIMATION_NODE_BLEND_2D)
			animation->node_active[i] = (animation->param[def->node[i].param_weight] != 0.0f);
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

	ctx.gait_param = characterAnimation_getGaitParam(speed, movement);

	/* the footing is read from the clip that is actually running: the center
	   column of the row the gait sits on */
	const CharacterAnimationNode *locomotion = &def->node[def->locomotion_node];
	float row_t;
	uint8_t row = characterAnimation_blendSegment(ctx.gait_param, locomotion->rows, &row_t);
	if (row_t > 0.5f) row++;

	T3DAnim *base = &animation->clip[locomotion->animation[row * locomotion->cols + locomotion->cols / 2]];

	ctx.locomotion_phase = characterAnimation_getLocomotionPhase(base->time, t3d_anim_get_length(base));
	ctx.turning          = characterAnimation_getTurningAvg(animation, ctx.settings, character->body.rotation.z, character->movement.data.previous_yaw);

	characterAnimation_setLocomotionSpeed (&ctx);

	characterAnimation_setLocomotionParam (&ctx);
	characterAnimation_setIdleRightParam (&ctx);
	characterAnimation_setJumpParams (&ctx);
	characterAnimation_setRollParam (&ctx);
	characterAnimation_setStrafeParams (&ctx);
	characterAnimation_setStrafeLockedParams (&ctx);
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
	animation->strafe_turning = false;
	animation->strafe_blend = 0.0f;
	animation->strafe_locked_blend = 0.0f;
	animation->bow_walk_aiming_blend = 0.0f;

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

static T3DSkeleton *characterAnimation_clipBuffer(const CharacterAnimationDef *def, CharacterAnimation *animation, uint8_t clip)
{
	uint8_t buffer = def->clip[clip].buffer;
	return (buffer == ANIMATION_SLOT_MAIN) ? &animation->main : &animation->buffer[buffer];
}

static uint8_t characterAnimation_blendSegment(float weight, uint8_t count, float *t)
{
	if (count < 2) { *t = 0.0f; return 0; }
	if (weight < 0.0f) weight = 0.0f;
	if (weight > 1.0f) weight = 1.0f;

	float s = weight * (count - 1);
	uint8_t i = (uint8_t)s;
	if (i > count - 2) i = count - 2;
	*t = s - i;
	return i;
}

/* a clip shared by two nodes must advance once per frame */
static void characterAnimation_updateClip(CharacterAnimation *animation, bool *updated, uint8_t clip, float delta)
{
	if (updated[clip]) return;
	updated[clip] = true;
	t3d_anim_update(&animation->clip[clip], delta);
}

void characterAnimation_evaluateGraph(const CharacterAnimationDef *def, CharacterAnimation *animation, float delta)
{
	CharacterAnimationBuffer blend_buffer;
	blend_buffer.count = 0;

	bool updated[def->clip_count];
	memset(updated, false, sizeof(updated));

	for (int i = 0; i < def->node_count; i++)
	{
		if (!animation->node_active[i]) continue;

		const CharacterAnimationNode *node = &def->node[i];
		float param_val = animation->param[node->param_cols];

		switch (node->type)
		{
			case ANIMATION_NODE_CLIP:
			{
				characterAnimation_updateClip(animation, updated, node->animation[0], delta);
				break;
			}

			case ANIMATION_NODE_SELECT:
			{
				uint8_t active = (param_val < 0.0f) ? node->animation[0] : node->animation[1];
				uint8_t inactive = (param_val < 0.0f) ? node->animation[1] : node->animation[0];

				if (animation->node_state[i] != active)
				{
					animation->node_state[i] = active;
					t3d_anim_set_time(&animation->clip[inactive], animation->clip[active].time);
				}

				characterAnimation_updateClip(animation, updated, active, delta);
				break;
			}

			case ANIMATION_NODE_SEQUENCE:
			{
				T3DAnim *clip = &animation->clip[node->animation[0]];
				if (clip->isPlaying)
				{
					float limit = t3d_anim_get_length(clip);
					if ((clip->time + delta) < limit)
						characterAnimation_updateClip(animation, updated, node->animation[0], delta);
					else
						characterAnimation_updateClip(animation, updated, node->animation[1], delta);
				}
				else
					characterAnimation_updateClip(animation, updated, node->animation[1], delta);

				break;
			}

			case ANIMATION_NODE_BLEND:
			{
				if (param_val > 0.0f) {
					T3DSkeleton *buf = (node->buffer == ANIMATION_SLOT_MAIN) ? &animation->main : &animation->buffer[node->buffer];
					characterAnimation_updateClip(animation, updated, node->animation[0], delta);
					characterAnimation_addLayer(&blend_buffer, buf, param_val);
				}

				break;
			}

			case ANIMATION_NODE_BLEND_2D:
			{
				float weight = animation->param[node->param_weight];
				if (weight <= 0.0f) break;

				float tx, ty;
				uint8_t col = characterAnimation_blendSegment(param_val, node->cols, &tx);
				uint8_t row = characterAnimation_blendSegment(animation->param[node->param_rows], node->rows, &ty);

				// bilinear share of each corner, adds up to 1
				uint8_t corner[4];
				float   share[4];
				uint8_t count = 0;

				corner[count] = node->animation[row * node->cols + col];
				share[count++] = (1.0f - tx) * (1.0f - ty);

				if (tx > 0.0f) {
					corner[count] = node->animation[row * node->cols + col + 1];
					share[count++] = tx * (1.0f - ty);
				}

				if (ty > 0.0f) {
					corner[count] = node->animation[(row + 1) * node->cols + col];
					share[count++] = (1.0f - tx) * ty;
				}

				if (tx > 0.0f && ty > 0.0f) {
					corner[count] = node->animation[(row + 1) * node->cols + col + 1];
					share[count++] = tx * ty;
				}

				// every layer is diluted by the ones applied after it, so each one
				// is divided by what those leave: the main keeps 1 - weight
				float layer[4];
				float remain = 1.0f;
				for (int m = count - 1; m >= 0; m--) {
					layer[m] = (remain > 0.0000001f) ? weight * share[m] / remain : 1.0f;
					if (layer[m] > 1.0f) layer[m] = 1.0f;
					remain *= 1.0f - layer[m];
				}

				for (uint8_t m = 0; m < count; m++) {
					characterAnimation_updateClip(animation, updated, corner[m], delta);
					if (layer[m] > 0.0f)
						characterAnimation_addLayer(&blend_buffer, characterAnimation_clipBuffer(def, animation, corner[m]), layer[m]);
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
	characterWeapon_setBones(character);
	t3d_skeleton_update(&character->animation.main);
}