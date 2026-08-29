#include <assert.h>
#include <string.h>

#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dskeleton.h>

#include "scene3d/lighting.h"
#include "scene3d/fog.h"
#include "viewport/viewport.h"
#include "graphics/font.h"
#include "graphics/sprites.h"
#include "graphics/shapes.h"
#include "particles/particles.h"
#include "render/render.h"
#include "scene2d/scene2d.h"
#include "time/time.h"
#include "scene3d/scene3d.h"

#include "game/game.h"


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
	/* Only the counts matter: entries are fully written before being read,
	   and zeroing the whole struct wipes the entire 8 KB dcache. */
	ctx->element_count = 0;
	ctx->section_count = 0;
	ctx->object_count  = 0;
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



/* Bound and frustum are both in world space and both already built: the mesh
   placed its boxes with the matrix, the viewport its frustum with the camera,
   both before the context. */
static bool render_isBoundVisible(const MeshBound *bound, const T3DViewport *viewport)
{
	return t3d_frustum_vs_aabb(&viewport->viewFrustum, &bound->min, &bound->max);
}

static void render_setScene3DContext(RenderContext *ctx, const Scene3D *s, uint8_t fb_index)
{
	const T3DViewport *viewport = &viewport_get()->t3d_viewport;

	for (int i = 0; i < s->entity_count; i++) {
		Entity    *e      = s->entity[i];
		Mesh *mesh  = e->mesh;
		T3DMat4FP  *matrix = mesh->matrix_buffer ? &mesh->matrix_buffer[fb_index] : NULL;
		T3DSkeleton *skel  = mesh->skeleton;

		if (e->cull) {
			mesh->culled = !render_isBoundVisible(&mesh->bound[0], viewport);
			if (mesh->culled) continue;
		}

		/* Whatever drives this mesh has already moved: fold the new positions
		   into this frame's vertex buffer, then point the segment its recorded
		   display list reads from at that same copy. */
		mesh_updateDeform(mesh, fb_index);
		mesh_bindDeformFrame(mesh, fb_index);

		if (mesh->dl_count == 0) {
			uint8_t b = 1;
			T3DModelIter it = t3d_model_iter_create(mesh->model, T3D_CHUNK_TYPE_OBJECT);
			while (t3d_model_iter_next(&it) && b < mesh->bound_count) {
				it.object->isVisible = !e->cull || render_isBoundVisible(&mesh->bound[b], viewport);
				b++;
			}

			assert(ctx->object_count < RENDER_MAX_T3D_OBJECT);
			ctx->object[ctx->object_count++] = (T3DElement){ NULL, mesh->model, matrix, skel, mesh->draw_conf };
			continue;
		}

		for (int part = 0; part < mesh->dl_count; part++) {
			if (!(mesh->visible & (1u << part))) continue;

			assert(ctx->object_count < RENDER_MAX_T3D_OBJECT);
			ctx->object[ctx->object_count++] = (T3DElement){ mesh->dl[part], NULL, matrix, skel };
		}
	}
}

/* One section per layer: the elements come from the live scene, the scissor
   from the definition that built it. */
static void render_setScene2DContext(RenderContext *ctx, const Scene2D *scene2d)
{
	if (!scene2d->def) return;

	for (int i = 0; i < scene2d->def->layer_count; i++) {
		const Scene2DLayer *src = &scene2d->def->layer[i];

		RenderSection *dst = render_beginSection(ctx);
		memcpy(ctx->element + ctx->element_count,
			   scene2d->element + scene2d->layer_start[i],
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

void render_setContext(RenderContext *ctx, const Scene3D *scene3d, uint8_t fb_index, const Scene2D *scene2d)
{
	render_initContext(ctx);
	if (scene3d) render_setScene3DContext(ctx, scene3d, fb_index);
	if (scene2d) render_setScene2DContext(ctx, scene2d);
}

static void render_start(int *fb_index)
{
	*fb_index = (*fb_index + 1) % FB_COUNT;

	/* Geometry fades toward the fog color, so the background must be it. */
	Fog *fog = fog_get();
	viewport_clear(fog->enabled ? fog->color : RGBA32(0, 0, 0, 0xFF));
}

static void render_end(void)
{
	rdpq_detach_show();
}

/* Uniform fade for textured elements, the stamina wheel way: env alpha
   modulates only the alpha channel while RGB passes TEX0 untouched, so
   prim color stays free for tinting. */
static void render_setTransparency(const DrawElement *element)
{
	if (element->transparency == 0) return;

	rdpq_set_env_color(RGBA32(0, 0, 0, 255 - element->transparency));
	rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,TEX0), (TEX0,0,ENV,0)));
}

void render(RenderContext *ctx, int *fb_index)
{
	render_start(fb_index);

	if (ctx->object_count > 0) {
		light_set(light_get());
		fog_set(fog_get());

		/* A mesh contributes one element per visible part, and every part of
		   the same mesh shares its skeleton and its matrix: a character with
		   three weapons is four elements with identical state. Binding and
		   pushing once per run of equal state, and popping only when it
		   changes, cuts that setup without altering a single draw. */
		const T3DSkeleton *bound  = NULL;
		const T3DMat4FP   *pushed = NULL;

		for (int i = 0; i < ctx->object_count; i++) {
			T3DElement *obj = &ctx->object[i];

			if (obj->skeleton && obj->skeleton != bound) {
				t3d_skeleton_use(obj->skeleton);
				bound = obj->skeleton;
			}

			if (obj->matrix != pushed) {
				if (pushed) t3d_matrix_pop(1);
				if (obj->matrix) t3d_matrix_push(obj->matrix);
				pushed = obj->matrix;
			}

			if (obj->dl) {
				rspq_block_run(obj->dl);
				continue;
			}

			T3DModelState state = t3d_model_state_create();
			state.drawConf = obj->conf;
			T3DModelIter it = t3d_model_iter_create(obj->model, T3D_CHUNK_TYPE_OBJECT);
			while (t3d_model_iter_next(&it)) {
				if (!it.object->isVisible) continue;
				t3d_model_draw_material(it.object->material, &state);
				rspq_block_run(it.object->userBlock);
			}
		}

		if (pushed) t3d_matrix_pop(1);
	}

	particles_draw();

	for (int s = 0; s < ctx->section_count; s++) {
		RenderSection *section = &ctx->section[s];

		/* Nothing on screen, nothing on the wire: a fully hidden section
		   must not emit a single command, scissor setup included. */
		bool any_visible = false;
		for (int i = 0; i < section->element_count; i++) {
			if (!ctx->element[section->element_start + i].is_hidden) {
				any_visible = true;
				break;
			}
		}
		if (!any_visible) continue;

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
				case DRAW_RECTANGLE:    shape_drawRectangle(&element->rectangle, element->position, element->scale);                          break;
				case DRAW_TEXT:         text_draw(&element->text, element->position);                                                         break;
				case DRAW_SPRITE:       sprite_setMode(); render_setTransparency(element); sprite_draw(&element->sprite, element->position, element->scale, element->rotation); break;
				case DRAW_TILED_SPRITE: sprite_setMode(); render_setTransparency(element); sprite_drawTiled(&element->sprite, element->position, element->scale);               break;
			}
		}

		if (section->has_scissor)
			rdpq_set_scissor(0, 0, 320, 240);
	}

	render_end();
}
