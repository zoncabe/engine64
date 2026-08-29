
#ifndef RENDER_H
#define RENDER_H

#include <stdbool.h>

#include <libdragon.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>
#include "graphics/shapes.h"
#include "graphics/sprites.h"
#include "graphics/font.h"
#include "physics/math/vector3.h"

#define RENDER_MAX_DRAW_ELEMENT  128
/* One entry per visible mesh part, not per entity: a skinned character alone
   contributes several, so this has to clear the scene's entity budget. */
#define RENDER_MAX_T3D_OBJECT    128
#define RENDER_MAX_SECTION         8

typedef struct Scene3D          Scene3D;
typedef struct Scene2D         Scene2D;

typedef struct RenderTransform {
	Vector3 position;
	Vector3 rotation;
	Vector3 scale;
} RenderTransform;

typedef enum {

	DRAW_RECTANGLE,
	DRAW_SPRITE,
	DRAW_TILED_SPRITE,   /* repeated sprite: position = origin, scale = size in px */
	DRAW_TEXT,

} DrawElementType;

typedef struct {

	DrawElementType type;

	union {
		Rectangle rectangle;
		Sprite    sprite;
		Text      text;
	};

	Vector2 scale;
	Vector2 position;
	float rotation;
	uint8_t transparency;

	bool    is_hidden;

} DrawElement;

typedef struct {

	rspq_block_t *dl;      /* NULL: draw model's visible objects instead */
	T3DModel     *model;
	T3DMat4FP    *matrix;
	T3DSkeleton  *skeleton;
	T3DModelDrawConf *conf; /* optional, object path only: per-frame tile/texture hooks */

} T3DElement;

typedef struct {

	uint8_t element_start;
	uint8_t element_count;

	bool    has_scissor;
	float   scissor_x;
	float   scissor_y;
	float   scissor_w;
	float   scissor_h;

} RenderSection;

typedef struct RenderContext {

	DrawElement   element[RENDER_MAX_DRAW_ELEMENT];
	uint8_t       element_count;

	RenderSection section[RENDER_MAX_SECTION];
	uint8_t       section_count;

	T3DElement    object[RENDER_MAX_T3D_OBJECT];
	uint8_t       object_count;

} RenderContext;


void renderTransform_init(RenderTransform *t);

void render_initContext(RenderContext *ctx);
void render_setContext(RenderContext *ctx, const Scene3D *scene3d, uint8_t fb_index, const Scene2D *scene2d);
void render(RenderContext *ctx, int *fb_index);


#endif
