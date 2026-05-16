#include <string.h>

#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dskeleton.h>

#include "light/lighting.h"
#include "viewport/viewport.h"
#include "graphics/font.h"
#include "graphics/sprites.h"
#include "graphics/shapes.h"
#include "render/render.h"
#include "screen/screen.h"
#include "time/time.h"
#include "scene/scene.h"

#include "game/game.h"

#define DEBUG true


void renderTransform_init(RenderTransform *t)
{
	*t = (RenderTransform){
		.position = {0.0f, 0.0f, 0.0f},
		.rotation = {0.0f, 0.0f, 0.0f},
		.scale    = {1.0f, 1.0f, 1.0f},
	};
}

void render_initContext(RenderContext *ctx)
{
	*ctx = (RenderContext){0};
}


static RenderSection *render_beginSection(RenderContext *ctx)
{
	RenderSection *section = &ctx->section[ctx->section_count++];
	section->element_start = ctx->element_count;
	section->element_count = 0;
	section->has_scissor   = false;
	return section;
}

static void render_endSection(RenderContext *ctx, RenderSection *section)
{
	section->element_count = ctx->element_count - section->element_start;
}

static void render_pushElement(RenderContext *ctx, DrawElement element)
{
	ctx->element[ctx->element_count++] = element;
}


static void render_setSceneContext(RenderContext *ctx, const Scene *s, uint8_t fb_index)
{
	for (int i = 0; i < s->entity_count; i++) {
		Entity    *e      = s->entity[i];
		Mesh *mesh  = e->mesh;
		T3DMat4FP  *matrix = mesh->matrix_buffer ? &mesh->matrix_buffer[fb_index] : NULL;
		T3DSkeleton *skel  = e->actor ? &e->actor->animation.main : NULL;
		ctx->object[ctx->object_count++] = (T3DElement){ mesh->dl, matrix, skel };
	}
}

static void render_setDebugContext(RenderContext *ctx)
{
#if DEBUG
	static char fps_buf[8];
	snprintf(fps_buf, sizeof(fps_buf), "%d FPS", (int)time_get()->rate);
	RenderSection *section = render_beginSection(ctx);
	render_pushElement(ctx, (DrawElement){
		.type     = DRAW_TEXT,
		.position = { 272.0f, 20.0f },
		.text     = { DROID_SANS, 0, fps_buf, NULL }
	});
	render_endSection(ctx, section);
#else
	(void)ctx;
#endif
}

static void render_setScreenContext(RenderContext *ctx, const Screen *screen)
{
	for (int i = 0; i < screen->section_count; i++) {
		const ScreenSection *src = &screen->section[i];
		RenderSection *dst = render_beginSection(ctx);
		memcpy(ctx->element + ctx->element_count,
			   src->element,
			   src->element_count * sizeof(DrawElement));
		ctx->element_count += src->element_count;
		render_endSection(ctx, dst);
		dst->has_scissor = src->has_scissor;
		dst->scissor_x   = src->scissor_x;
		dst->scissor_y   = src->scissor_y;
		dst->scissor_w   = src->scissor_w;
		dst->scissor_h   = src->scissor_h;
	}
}

void render_setContext(RenderContext *ctx, const Scene *scene, uint8_t fb_index, const Screen *screen)
{
	render_initContext(ctx);
	if (scene)  render_setSceneContext(ctx, scene, fb_index);
	if (screen) render_setScreenContext(ctx, screen);
	render_setDebugContext(ctx);
}

static void render_start(int *fb_index)
{
	*fb_index = (*fb_index + 1) % FB_COUNT;
	viewport_clear();
}

static void render_end(void)
{
	rdpq_detach_show();
}

void render(RenderContext *ctx, int *fb_index)
{
	render_start(fb_index);

	if (ctx->object_count > 0) {
		light_set(light_get());
		for (int i = 0; i < ctx->object_count; i++) {
			T3DElement *obj = &ctx->object[i];
			if (obj->skeleton) t3d_skeleton_use(obj->skeleton);
			if (obj->matrix)   t3d_matrix_push(obj->matrix);
			rspq_block_run(obj->dl);
			if (obj->matrix)   t3d_matrix_pop(1);
		}
	}

	for (int s = 0; s < ctx->section_count; s++) {
		RenderSection *section = &ctx->section[s];

		if (section->has_scissor) {
			rdpq_set_scissor(
				section->scissor_x,
				section->scissor_y,
				section->scissor_x + section->scissor_w,
				section->scissor_y + section->scissor_h);
		}

		for (int i = 0; i < section->element_count; i++) {
			DrawElement *element = &ctx->element[section->element_start + i];
			if (element->is_hidden) continue;
			switch (element->type) {
				case DRAW_RECTANGLE: shape_drawRectangle(&element->rectangle, element->position, element->scale);                       break;
				case DRAW_TEXT:      text_draw(&element->text, element->position);                                                      break;
				case DRAW_SPRITE:    sprite_setMode(); sprite_draw(&element->sprite, element->position, element->scale, element->rotation); break;
			}
		}

		if (section->has_scissor)
			rdpq_set_scissor(0, 0, 320, 240);
	}

	render_end();
}
