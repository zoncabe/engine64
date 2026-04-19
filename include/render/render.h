
#ifndef RENDER_H
#define RENDER_H

#include <stdbool.h>

#include <libdragon.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dskeleton.h>
#include "graphics/shapes.h"
#include "graphics/sprites.h"
#include "graphics/font.h"

#define RENDER_MAX_DRAW_ELEMENT  128
#define RENDER_MAX_T3D_OBJECT     16
#define RENDER_MAX_SECTION         8

typedef struct Scene          Scene;
typedef struct GameTransition GameTransition;
typedef struct Screen         Screen;

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

	Vector2 scale;
	Vector2 position;
	float rotation;

	bool is_hidden;

} DrawElement;

typedef struct {

	rspq_block_t *dl;
	T3DMat4FP    *matrix;
	T3DSkeleton  *skeleton;

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


// function prototypes

void render_initContext(RenderContext *ctx);
void render_setContext(RenderContext *ctx, const Scene *scene, uint8_t fb_index, const GameTransition *transition, const Screen *screen);
void render(RenderContext *ctx, int *fb_index);


#endif
