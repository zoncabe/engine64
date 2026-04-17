
#ifndef RENDER_H
#define RENDER_H

#include <libdragon.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dskeleton.h>
#include "graphics/shapes.h"
#include "graphics/sprites.h"
#include "graphics/font.h"

#define RENDER_MAX_DRAW_ELEMENT  32
#define RENDER_MAX_T3D_OBJECT    16

typedef struct Scene          Scene;
typedef struct GameTransition GameTransition;

typedef enum {

	DRAW_RECTANGLE,
	DRAW_SPRITE,
	DRAW_TEXT,

} DrawElementType;

typedef struct {

	DrawElementType type;

	union {
		Rectangle rectangle;
		Sprite    sprite;
		Text      text;
	};

} DrawElement;

typedef struct {

	rspq_block_t *dl;
	T3DMat4FP    *matrix;
	T3DSkeleton  *skeleton;

} T3DElement;

typedef struct RenderContext {

	DrawElement element[RENDER_MAX_DRAW_ELEMENT];
	uint8_t     element_count;

	T3DElement  object[RENDER_MAX_T3D_OBJECT];
	uint8_t     object_count;

} RenderContext;


// function prototypes

void render_initContext(RenderContext *ctx);
void render_getScreenContext(RenderContext *ctx, const RenderContext *screen);
void render_setContext(RenderContext *ctx, const Scene *scene, uint8_t fb_index, const GameTransition *transition, const RenderContext *screen);
void render(RenderContext *ctx, int *fb_index);


#endif
