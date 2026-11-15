#ifndef ENGINE64_ANIMATION_H
#define ENGINE64_ANIMATION_H

#include <stdbool.h>
#include <stdint.h>
#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>

namespace e64 {

/* The animation graph of a skinned model: a table of nodes walked in index
   order every frame, each one advancing its clips and pushing a layer, and
   the layer stack applied onto the main skeleton at the end. Who sets the
   params the nodes read, and what they mean, is the owner's business: a
   character drives them from its movement, anything else from its own. */
class Animation {
public:

	static constexpr uint8_t MAX_LAYERS = 24;
	static constexpr uint8_t SLOT_MAIN = 0xFF;

	typedef enum {

		NODE_CLIP,
		NODE_SELECT,
		NODE_SEQUENCE,
		NODE_BLEND,
		NODE_BLEND_2D,
		NODE_LAYER,

	} NodeType;

	struct ClipDef {

		const char *name;
		uint8_t buffer;
		bool is_looping;

	};

	/* One node of the graph. The def's node table is walked in index order
	   every frame, and that order IS the blend order: each active node pushes
	   its layer as it is met, and the layers are then applied first to last,
	   every one blended over the result of the ones before it with its own
	   weight.

	   Two consequences the table has to be built around:

	   1. A layer at full weight replaces the whole pose. Everything blended
	      before it is discarded. The full-body grids reach full weight while
	      they drive the body, so they go FIRST; the partial and action layers
	      go AFTER every grid, or the grid that happens to be at full weight
	      erases them.

	   2. A SEQUENCE or SELECT node that only advances a clip into a buffer
	      can sit anywhere before the LAYER node that reads that buffer; only
	      the LAYER's position decides where the pose lands in the stack.

	   The engine does not sort this and cannot: which grid weighs 1 changes
	   with the state, and the asset is the one that knows what its layers
	   mean. */
	struct Node {

		NodeType type;
		const uint8_t *animation;
		uint8_t cols;
		uint8_t rows;
		uint8_t buffer;
		uint8_t param_cols;
		uint8_t param_rows;
		uint8_t param_weight; /* BLEND_2D: weight the composed grid enters the main with */

	};

	struct Def {

		const ClipDef *clip;
		const Node *node;
		uint8_t clip_count;
		uint8_t node_count;
		uint8_t buffer_count;
		uint8_t param_count;

	};

	/* The layer stack one frame of the graph builds, applied onto the main in
	   push order. */
	struct Buffer {

		const T3DSkeleton *layer[MAX_LAYERS];
		float weight[MAX_LAYERS];
		uint8_t count;

	};


	const Def *def;
	const T3DModel *model;
	T3DSkeleton main;
	T3DSkeleton *buffer;
	T3DAnim *clip;
	uint8_t *node_state;
	bool *node_active;
	float *param;

	/* Phase of the heaviest grid corner of the last frame, [0,1]. */
	float cycle;

};


namespace animation {

namespace buffer {

void addLayer(Animation::Buffer *stack, const T3DSkeleton *skel, float weight);
void blendLayers(const Animation::Buffer *stack, const T3DSkeleton *main);

}

void init(Animation *animation, const Animation::Def *def, const T3DModel *model);
void destroy(Animation *animation);

/* Where a clip writes its pose: the main or one of the def's buffers. */
T3DSkeleton *clipBuffer(Animation *animation, uint8_t clip);

T3DAnim *getClip(Animation *animation, uint8_t index);

/* Splits a [0,1] weight over count clips: the base index and the share of
   the next one. */
uint8_t blendSegment(float weight, uint8_t count, float *t);

/* The 1 to 4 clips a grid blends at the given axis values: the base corner,
   then the next column, the next row, and the far corner, each only when
   its share is not zero. */
uint8_t getGridClips(const Animation::Node *node, float cols_value, float rows_value, uint8_t clip[4]);

/* Phase carry: the clips entering the grid start where the ones leaving it
   were. Reads the previous axis values from the node's params. */
void syncGridClips(Animation *animation, const Animation::Node *node, float cols_value, float rows_value);

/* Carries a clip's phase into every clip of a grid. */
void snapGridFromClip(Animation *animation, uint8_t src_clip, uint8_t dst_node);

/* Carries the phase of a grid's column closest to src_dir (first row) into
   every clip of another grid. */
void snapGridFromGrid(Animation *animation, uint8_t src_node, float src_dir, uint8_t dst_node);

/* Reads the params and turns the nodes on or off; a grid coming back on
   resumes on the live cycle. Runs after the owner wrote the params. */
void setActiveNodes(Animation *animation);

/* Advances the active nodes' clips, builds the layer stack and blends it
   onto the main. */
void evaluate(Animation *animation, float delta);

}

}

#endif
