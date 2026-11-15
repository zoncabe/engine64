#include <assert.h>
#include <math.h>
#include <malloc.h>
#include <libdragon.h>

#include "viewport/e64_viewport.h"
#include "model/e64_animation.h"

namespace e64 {
namespace animation {

/* --- layers ----------------------------------------------------------------- */

/* A layer at this weight or above replaces the pose outright: the first
   corner of a grid at full weight always lands here through the dilution in
   evaluate, and the nlerp it would get costs a sqrt and a divide per bone to
   hand back the layer's own quaternion. What the copy leaves out of the blend
   is a hundredth of the pose underneath, below what the eye sees. */
#define ANIMATION_LAYER_COPY_WEIGHT 0.99f

namespace buffer {

/* t3d_quat_nlerp without the normalize: the same blend along the shorter
   arc, leaving the length to whoever normalizes at the end. */
static inline void quat_lerp(T3DQuat *res, const T3DQuat *a, const T3DQuat *b, float t)
{
	float blend = 1.0f - t;
	if (t3d_quat_dot(a, b) < 0.0f) blend = -blend;

	res->v[0] = blend * a->v[0] + t * b->v[0];
	res->v[1] = blend * a->v[1] + t * b->v[1];
	res->v[2] = blend * a->v[2] + t * b->v[2];
	res->v[3] = blend * a->v[3] + t * b->v[3];
}

void addLayer(Animation::Buffer *stack, const T3DSkeleton *skel, float weight)
{
	/* Never write past the stack: a dropped layer is a pose glitch, an
	   overflow is garbage quaternions frames later. */
	assert(stack->count < Animation::MAX_LAYERS);
	if (stack->count >= Animation::MAX_LAYERS) return;

	stack->layer[stack->count] = skel;
	stack->weight[stack->count] = weight;
	stack->count++;
}

void blendLayers(const Animation::Buffer *stack, const T3DSkeleton *main)
{
	/* A copying layer discards everything blended before it, so the stack is
	   applied from the last one that copies: the layers below it would cost a
	   nlerp per bone each and change nothing. Weights are per layer, so this
	   is settled once, not per bone. */
	int first = 0;
	for (int j = stack->count - 1; j > 0; j--) {
		if (stack->weight[j] >= ANIMATION_LAYER_COPY_WEIGHT) { first = j; break; }
	}

	for (int i = 0; i < main->skeletonRef->boneCount; i++)
	{
		T3DBone *bone = &main->bones[i];
		bone->hasChanged = true;

		for (int j = first; j < stack->count; j++)
		{
			T3DBone *layer = &stack->layer[j]->bones[i];

			if (stack->weight[j] >= ANIMATION_LAYER_COPY_WEIGHT) {
				bone->rotation = layer->rotation;
				bone->position = layer->position;
				bone->scale = layer->scale;
				continue;
			}

			quat_lerp(&bone->rotation, &bone->rotation, &layer->rotation, stack->weight[j]);
			t3d_vec3_lerp(&bone->position, &bone->position, &layer->position, stack->weight[j]);
			t3d_vec3_lerp(&bone->scale, &bone->scale, &layer->scale, stack->weight[j]);
		}

		t3d_quat_normalize(&bone->rotation);
	}
}

}


/* --- clips ------------------------------------------------------------------ */

T3DSkeleton *clipBuffer(Animation *animation, uint8_t clip)
{
	uint8_t buffer = animation->def->clip[clip].buffer;
	return (buffer == Animation::SLOT_MAIN) ? &animation->main : &animation->buffer[buffer];
}

T3DAnim *getClip(Animation *animation, uint8_t index)
{
	return &animation->clip[index];
}


/* --- grids ------------------------------------------------------------------ */

uint8_t blendSegment(float weight, uint8_t count, float *t)
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

uint8_t getGridClips(const Animation::Node *node, float cols_value, float rows_value, uint8_t clip[4])
{
	float tx, ty;
	uint8_t col = blendSegment(cols_value, node->cols, &tx);
	uint8_t row = blendSegment(rows_value, node->rows, &ty);
	uint8_t count = 0;

	clip[count++] = node->animation[row * node->cols + col];
	if (tx > 0.0f) clip[count++] = node->animation[row * node->cols + col + 1];
	if (ty > 0.0f) clip[count++] = node->animation[(row + 1) * node->cols + col];
	if (tx > 0.0f && ty > 0.0f) clip[count++] = node->animation[(row + 1) * node->cols + col + 1];

	return count;
}

void syncGridClips(Animation *animation, const Animation::Node *node, float cols_value, float rows_value)
{
	const float *param = animation->param;

	uint8_t prev[4], curr[4];
	uint8_t prev_count = getGridClips(node, param[node->param_cols], param[node->param_rows], prev);
	uint8_t curr_count = getGridClips(node, cols_value, rows_value, curr);

	/* phase reference: a corner still taking part, or the previous base if
	   none is. Among those, the centre column wins: it holds the clip the
	   grids share (walk, run, sprint), the one most likely to have kept
	   advancing under another grid. A side corner may have been frozen the
	   whole time, and copying its phase puts the newcomer out of step. */
	uint8_t centre = node->cols / 2;
	int16_t ref_clip = -1;
	for (uint8_t m = 0; m < curr_count; m++) {
		bool carried = false;
		for (uint8_t p = 0; p < prev_count; p++)
			if (curr[m] == prev[p]) carried = true;
		if (!carried) continue;

		bool centred = false;
		for (uint8_t r = 0; r < node->rows; r++)
			if (curr[m] == node->animation[r * node->cols + centre]) centred = true;

		if (ref_clip < 0 || centred) ref_clip = curr[m];
		if (centred) break;
	}
	T3DAnim *ref = getClip(animation, ref_clip >= 0 ? (uint8_t)ref_clip : prev[0]);

	float ref_length = t3d_anim_get_length(ref);
	if (ref_length <= 0.0f) return;
	float phase = ref->time / ref_length;

	for (uint8_t m = 0; m < curr_count; m++) {
		bool carried = false;
		for (uint8_t p = 0; p < prev_count; p++)
			if (curr[m] == prev[p]) carried = true;
		if (carried) continue;

		T3DAnim *dst = getClip(animation, curr[m]);
		t3d_anim_set_time(dst, phase * t3d_anim_get_length(dst));
	}
}

void snapGridFromClip(Animation *animation, uint8_t src_clip, uint8_t dst_node)
{
	const Animation::Node *node = &animation->def->node[dst_node];

	T3DAnim *src = getClip(animation, src_clip);
	float src_length = t3d_anim_get_length(src);
	if (src_length <= 0.0f) return;
	float phase = src->time / src_length;

	for (int c = 0; c < node->cols * node->rows; c++) {
		T3DAnim *dst = getClip(animation, node->animation[c]);
		t3d_anim_set_time(dst, phase * t3d_anim_get_length(dst));
	}
}

void snapGridFromGrid(Animation *animation, uint8_t src_node, float src_dir, uint8_t dst_node)
{
	const Animation::Node *src_grid = &animation->def->node[src_node];
	const Animation::Node *dst_grid = &animation->def->node[dst_node];

	uint8_t src_col = (uint8_t)(src_dir * (src_grid->cols - 1) + 0.5f);
	if (src_col > src_grid->cols - 1) src_col = src_grid->cols - 1;

	T3DAnim *src = getClip(animation, src_grid->animation[src_col]);
	float src_length = t3d_anim_get_length(src);
	if (src_length <= 0.0f) return;
	float phase = src->time / src_length;

	for (int c = 0; c < dst_grid->cols * dst_grid->rows; c++) {
		T3DAnim *dst = getClip(animation, dst_grid->animation[c]);
		t3d_anim_set_time(dst, phase * t3d_anim_get_length(dst));
	}
}


/* --- graph ------------------------------------------------------------------ */

void setActiveNodes(Animation *animation)
{
	const Animation::Def *def = animation->def;
	const float *param = animation->param;

	for (int i = 0; i < def->node_count; i++) {
		const Animation::Node *node = &def->node[i];
		Animation::NodeType t = node->type;
		if (t == Animation::NODE_SELECT || t == Animation::NODE_SEQUENCE)
			animation->node_active[i] = (param[node->param_cols] != 0.0f);
		if (t != Animation::NODE_BLEND_2D) continue;

		bool was = animation->node_active[i];
		bool now = (param[node->param_weight] != 0.0f);
		animation->node_active[i] = now;
		if (!now || was) continue;

		/* A grid coming back on has sat frozen while another one drove the
		   body: only the clips it shares with that grid kept advancing, the
		   rest hold the phase they stopped at. Its clips resume on the live
		   cycle, the heaviest corner of the previous frame (not advanced yet
		   this frame), so the two grids agree from the first frame of the
		   crossfade. One write per clip, on this frame only. */
		uint8_t clip[4];
		uint8_t count = getGridClips(node, param[node->param_cols], param[node->param_rows], clip);
		for (uint8_t m = 0; m < count; m++) {
			T3DAnim *anim = getClip(animation, clip[m]);
			t3d_anim_set_time(anim, animation->cycle * t3d_anim_get_length(anim));
		}
	}
}

/* a clip shared by two nodes must advance once per frame */
static void updateClip(Animation *animation, bool *updated, uint8_t clip, float delta)
{
	if (updated[clip]) return;
	updated[clip] = true;
	t3d_anim_update(getClip(animation, clip), delta);
}

void evaluate(Animation *animation, float delta)
{
	const Animation::Def *def = animation->def;
	const float *param = animation->param;

	Animation::Buffer stack;
	stack.count = 0;

	bool updated[def->clip_count];
	for (int i = 0; i < def->clip_count; i++) updated[i] = false;

	/* the heaviest grid corner of the frame; its cycle is read once, after
	   the walk, instead of dividing every time the lead changes hands */
	float cycle_weight = 0.0f;
	uint8_t cycle_clip = 0;

	for (int i = 0; i < def->node_count; i++)
	{
		if (!animation->node_active[i]) continue;

		const Animation::Node *node = &def->node[i];
		float param_val = param[node->param_cols];

		switch (node->type)
		{
			case Animation::NODE_CLIP:
			{
				updateClip(animation, updated, node->animation[0], delta);
				break;
			}

			case Animation::NODE_SELECT:
			{
				uint8_t active = (param_val < 0.0f) ? node->animation[0] : node->animation[1];
				uint8_t inactive = (param_val < 0.0f) ? node->animation[1] : node->animation[0];

				if (animation->node_state[i] != active)
				{
					animation->node_state[i] = active;
					t3d_anim_set_time(getClip(animation, inactive), getClip(animation, active)->time);
				}

				updateClip(animation, updated, active, delta);
				break;
			}

			case Animation::NODE_SEQUENCE:
			{
				T3DAnim *clip = getClip(animation, node->animation[0]);
				if (clip->isPlaying)
				{
					float limit = t3d_anim_get_length(clip);
					if ((clip->time + delta) < limit)
						updateClip(animation, updated, node->animation[0], delta);
					else
						updateClip(animation, updated, node->animation[1], delta);
				}
				else
					updateClip(animation, updated, node->animation[1], delta);

				break;
			}

			case Animation::NODE_BLEND:
			{
				if (param_val > 0.0f) {
					T3DSkeleton *buf = (node->buffer == Animation::SLOT_MAIN) ? &animation->main : &animation->buffer[node->buffer];
					updateClip(animation, updated, node->animation[0], delta);
					buffer::addLayer(&stack, buf, param_val);
				}

				break;
			}

			case Animation::NODE_BLEND_2D:
			{
				float weight = param[node->param_weight];
				if (weight <= 0.0f) break;

				float tx, ty;
				uint8_t col = blendSegment(param_val, node->cols, &tx);
				uint8_t row = blendSegment(param[node->param_rows], node->rows, &ty);

				/* bilinear share of each corner, adds up to 1 */
				uint8_t corner[4];
				float share[4];
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

				/* every layer is diluted by the ones applied after it, so each
				   one is divided by what those leave: the main keeps 1 - weight */
				float layer[4];
				float remain = 1.0f;
				for (int m = count - 1; m >= 0; m--) {
					layer[m] = (remain > 0.0000001f) ? weight * share[m] / remain : 1.0f;
					if (layer[m] > 1.0f) layer[m] = 1.0f;
					remain *= 1.0f - layer[m];
				}

				for (uint8_t m = 0; m < count; m++) {
					updateClip(animation, updated, corner[m], delta);
					if (layer[m] > 0.0f)
						buffer::addLayer(&stack, clipBuffer(animation, corner[m]), layer[m]);

					if (weight * share[m] > cycle_weight) {
						cycle_weight = weight * share[m];
						cycle_clip = corner[m];
					}
				}

				break;
			}

			case Animation::NODE_LAYER:
			{
				float abs_val = fabsf(param_val);
				if (abs_val > 0.0f) {
					T3DSkeleton *buf = (node->buffer == Animation::SLOT_MAIN) ? &animation->main : &animation->buffer[node->buffer];
					buffer::addLayer(&stack, buf, abs_val);
				}

				break;
			}
		}
	}

	if (cycle_weight > 0.0f) {
		T3DAnim *clip = getClip(animation, cycle_clip);
		animation->cycle = clip->time / t3d_anim_get_length(clip);
	}

	buffer::blendLayers(&stack, &animation->main);
}


/* --- lifecycle -------------------------------------------------------------- */

void init(Animation *animation, const Animation::Def *def, const T3DModel *model)
{
	animation->def = def;
	animation->model = model;

	animation->main = t3d_skeleton_create_buffered(model, Viewport::FB_COUNT);

	animation->buffer = (T3DSkeleton *)malloc(def->buffer_count * sizeof(T3DSkeleton));
	assert(animation->buffer);
	for (int i = 0; i < def->buffer_count; i++)
		animation->buffer[i] = t3d_skeleton_clone(&animation->main, false);

	/* Every node starts on: setActiveNodes gates them from the params after
	   the first write. */
	animation->node_active = (bool *)malloc(def->node_count * sizeof(bool));
	assert(animation->node_active);
	animation->node_state = (uint8_t *)malloc(def->node_count * sizeof(uint8_t));
	assert(animation->node_state);
	for (int i = 0; i < def->node_count; i++) {
		animation->node_active[i] = true;
		animation->node_state[i] = 0;
	}

	animation->param = (float *)malloc(def->param_count * sizeof(float));
	assert(animation->param);
	for (int i = 0; i < def->param_count; i++) animation->param[i] = 0.0f;

	/* Read by setActiveNodes as soon as a grid comes back on, which can be
	   before any frame has had a grid with weight to write it. */
	animation->cycle = 0.0f;

	animation->clip = (T3DAnim *)malloc(def->clip_count * sizeof(T3DAnim));
	assert(animation->clip);
	for (int i = 0; i < def->clip_count; i++) {
		T3DAnim *clip = &animation->clip[i];
		const Animation::ClipDef *clip_def = &def->clip[i];

		*clip = t3d_anim_create(model, clip_def->name);
		t3d_anim_attach(clip, clipBuffer(animation, i));
		t3d_anim_set_looping(clip, clip_def->is_looping);
		t3d_anim_set_playing(clip, clip_def->is_looping);
	}
}

void destroy(Animation *animation)
{
	const Animation::Def *def = animation->def;
	if (!def) return;

	for (int i = 0; i < def->clip_count; i++)
		t3d_anim_destroy(&animation->clip[i]);
	for (int i = 0; i < def->buffer_count; i++)
		t3d_skeleton_destroy(&animation->buffer[i]);
	t3d_skeleton_destroy(&animation->main);

	free(animation->clip);
	free(animation->buffer);
	free(animation->node_state);
	free(animation->node_active);
	free(animation->param);
}

}
}
