
#ifndef ENGINE64_RENDER_H
#define ENGINE64_RENDER_H

#include <stdbool.h>

#include <libdragon.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>
#include "graphics/e64_shapes.h"
#include "graphics/e64_sprites.h"
#include "graphics/e64_font.h"
#include "physics/math/e64_vector3.h"

#define RENDER_MAX_2D_ELEMENTS   64
/* One entry per visible mesh part, not per entity: a skinned character alone
   contributes several, so this has to clear the scene's entity budget. */
#define RENDER_MAX_3D_ELEMENTS    32
#define RENDER_MAX_SECTIONS        8

typedef struct Scene3D          Scene3D;
typedef struct Scene2D         Scene2D;

typedef struct RenderTransform {
	Vector3 position;
	Vector3 rotation;
	Vector3 scale;
} RenderTransform;

typedef enum {

	ELEMENT2D_RECTANGLE,
	ELEMENT2D_SPRITE,
	ELEMENT2D_TILED_SPRITE,   /* repeated sprite: position = origin, scale = size in px */
	ELEMENT2D_TEXT,

} Element2DType;

typedef struct {

	Element2DType type;

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

} Element2D;

typedef struct {

	rspq_block_t *dl;      /* NULL: draw model's visible objects instead */
	T3DModel     *model;
	T3DMat4FP    *matrix;
	T3DSkeleton  *skeleton;
	T3DModelDrawConf *conf; /* optional, object path only: per-frame tile/texture hooks */

} Element3D;

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

	Element2D     element[RENDER_MAX_2D_ELEMENTS];
	uint8_t       element_count;

	RenderSection section[RENDER_MAX_SECTIONS];
	uint8_t       section_count;

	Element3D     object[RENDER_MAX_3D_ELEMENTS];
	uint8_t       object_count;

} RenderContext;


void renderTransform_init(RenderTransform *t);

void render_initContext(RenderContext *ctx);
void render_setContext(RenderContext *ctx, const Scene3D *scene3d, uint8_t fb_index, const Scene2D *scene2d);
void render(RenderContext *ctx, int *fb_index);


#endif
