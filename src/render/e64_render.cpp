#include <assert.h>
#include <string.h>

#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dskeleton.h>

#include "scene3d/e64_light.h"
#include "scene3d/e64_fog.h"
#include "viewport/e64_viewport.h"
#include "graphics/e64_font.h"
#include "model/e64_mesh.h"
#include "graphics/e64_sprites.h"
#include "graphics/e64_shapes.h"
#include "particles/e64_particles.h"
#include "render/e64_render.h"
#include "scene2d/e64_scene2d.h"
#include "ui/e64_ui.h"
#include "debug/e64_debug.h"
#include "time/e64_time.h"
#include "scene3d/e64_scene3d.h"

#include "game/e64_game.h"

namespace e64 {

namespace render {

/* The frame's draw list. Filled by the scenes and consumed here, every
   frame; nobody outside sees it. */
static Render::Context frame_context;


namespace transform {

void init(Render::Transform *t)
{
	*t = (Render::Transform){
		.position = {0.0f, 0.0f, 0.0f},
		.rotation = {0.0f, 0.0f, 0.0f},
		.scale = {1.0f, 1.0f, 1.0f},
	};
}

}

namespace context {

void init(Render::Context *ctx)
{
	/* Only the counts matter: entries are fully written before being read,
	   and zeroing the whole struct wipes the entire 8 KB dcache. */
	ctx->element2d_count = 0;
	ctx->section_count = 0;
	ctx->element3d_count = 0;
}

}


static void start(int *fb_index)
{
	*fb_index = (*fb_index + 1) % Viewport::FB_COUNT;

	/* Geometry fades toward the fog color, so the background must be it. A
	   2D scene that declares a sky paints that instead. */
	Fog *fog_state = fog::get();
	color_t clear = fog_state->enabled ? fog_state->color : RGBA32(0, 0, 0, 0xFF);

	const Scene2D *scene = scene2d::get();
	if (scene->def && scene->def->background.a) clear = scene->def->background;

	Viewport *viewport = viewport::get();

	viewport->attach();

	/* The 3D pipeline is only up while a 3D scene is, and it starts its own
	   frame: the render state it leaves is the one the clear and everything
	   after it run under, which is why it goes first. */
	if (viewport->has3D()) {
		t3d_frame_start();
		t3d_viewport_attach(&viewport->t3d_viewport);
	}

	viewport->clear(clear);

	/* Only the 3D pipeline writes depth. */
	if (viewport->has3D()) t3d_screen_clear_depth();
}

static void end(void)
{
	viewport::get()->detach();
}

/* No key a sprite could carry: the RDP is not on a sprite mode. */
static constexpr int SPRITE_MODE_UNSET = -1;

/* The key of a sprite mode: the transparency in the low byte, and above it
   whether it is copy mode and whether it reads a palette. */
static constexpr int SPRITE_MODE_COPY = 1 << 8;
static constexpr int SPRITE_MODE_TLUT = 1 << 9;

/* The whole RDP state a sprite draws under, in one batch: the mode API
   calls between begin and end are programmed as a single change instead of
   one apiece. Uniform fade for textured elements, the stamina wheel way:
   env alpha modulates only the alpha channel while RGB passes TEX0
   untouched, so prim color stays free for tinting.

   Copy mode moves texels to the framebuffer as they are, about four times
   faster, and only cuts by alpha compare. The TLUT goes in the same batch:
   it is part of the mode, and what reads the palette has to have it on.

   A run of sprites sharing a key sets this once: a stage's tiles are
   hundreds of elements under the very same state. */
static void setSpriteMode(uint8_t transparency, bool copy, rdpq_tlut_t tlut)
{
	rdpq_mode_begin();
		if (copy) {
			rdpq_set_mode_copy(true);
		} else {
			rdpq_set_mode_standard();
			rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
			rdpq_mode_alphacompare(1);
			if (transparency) rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,TEX0), (TEX0,0,ENV,0)));
		}
		rdpq_mode_tlut(tlut);
	rdpq_mode_end();

	/* Not a mode call, so it is issued on its own either way. */
	if (transparency) rdpq_set_env_color(RGBA32(0, 0, 0, (uint8_t)(255 - transparency)));
}

/* Whether a sprite can go in copy mode: one texel per pixel across,
   unturned, unmirrored, opaque, in a format copy mode reads, onto a 16 bit
   framebuffer (a 32 bit one hangs it). Copy mode steps 4 texels a clock and
   only renders right with dsdx 4.0: a scale across, or a mirror (-4.0), is
   wrong there, while a scale down is free. A tiled sprite fills its size at
   one texel per pixel. */
static bool isCopyable(const Render::Element2D *element, bool fb16)
{
	const Graphic *graphic = element->graphic;
	const Sprite *sprite = &graphic->sprite;

	if (!fb16) return false;
	if (element->rotation != 0.0f || sprite->flip_x || graphic->transparency) return false;

	tex_format_t fmt = sprite_get_format(sprite->asset);
	if (fmt != FMT_CI4 && fmt != FMT_CI8 && fmt != FMT_RGBA16) return false;

	return sprite->tiled || element->scale.x == 1.0f;
}

/* The recorded material uploaded the textures with the file's own tile
   translate; the scroll goes on top by reissuing SET_TILE_SIZE, the one
   command that carries it, with the same extents rdpq wrote: translate to
   translate plus the texture size, in 10.2. */
static void scrollTexture(const T3DMaterial *material, const Vector2 *scroll)
{
	const T3DMaterialTexture *texture[2] = { &material->textureA, &material->textureB };

	for (int k = 0; k < 2; k++) {
		const T3DMaterialTexture *tex = texture[k];
		if (!tex->texture) continue;

		uint16_t s0 = (uint16_t)((tex->s.low + scroll[k].x) * 4.0f);
		uint16_t t0 = (uint16_t)((tex->t.low + scroll[k].y) * 4.0f);
		rdpq_set_tile_size_fx((rdpq_tile_t)(TILE0 + k), s0, t0,
		                      s0 + tex->texWidth * 4, t0 + tex->texHeight * 4);
	}
}

/* The counting sort bucket of a material id: the ids in order, and the
   objects without material last. */
static inline int materialBucket(uint8_t material)
{
	return material == Mesh::MATERIAL_NONE ? Mesh::MAX_MATERIALS : material;
}

/* Runs 3D elements in the given order: each material once, ahead of the run
   of elements that use it; skeleton and matrix only when they change,
   popping only when another matrix takes over. bound and pushed carry that
   state across calls. Returns whether any material ran with a vertex FX. */
static bool runElements(const Render::Context *ctx, const uint16_t *order, uint16_t count,
                        const T3DSkeleton **bound, const T3DMat4FP **pushed)
{
	static const Vector2 no_scroll[2] = { { 0.0f, 0.0f }, { 0.0f, 0.0f } };

	int current = -1; /* the material the RDP is set for */
	const Mesh *scrolled = NULL; /* the mesh whose scroll the tiles carry */
	bool vertex_fx = false;

	for (uint16_t i = 0; i < count; i++) {
		const Render::Element3D *element = &ctx->element3d[order[i]];

		if (element->material != current) {
			rspq_block_t *block = mesh::material::block(element->material);
			if (block) rspq_block_run(block);
			if (mesh::material::hasVertexFx(element->material)) vertex_fx = true;
			current = element->material;
			scrolled = NULL; /* the block wrote the file's own tile translate */
		}

		/* The scroll rides on top of the shared material: reissued when a
		   scrolling mesh takes the tiles over, put back to the file's when a
		   mesh without one follows under the same material. */
		if (element->object->material) {
			if (element->mesh->texture_scroll) {
				if (element->mesh != scrolled) {
					scrollTexture(element->object->material, element->mesh->texture_scroll);
					scrolled = element->mesh;
				}
			} else if (scrolled) {
				scrollTexture(element->object->material, no_scroll);
				scrolled = NULL;
			}
		}

		if (element->skeleton && element->skeleton != *bound) {
			t3d_skeleton_use(element->skeleton);
			*bound = element->skeleton;
		}

		if (element->matrix != *pushed) {
			if (*pushed) t3d_matrix_pop(1);
			if (element->matrix) t3d_matrix_push(element->matrix);
			*pushed = element->matrix;
		}

		rspq_block_run(element->object->userBlock);
	}

	return vertex_fx;
}

void draw(void)
{
	Render::Context *ctx = &frame_context;
	Viewport *viewport = viewport::get();

	context::init(ctx);
	scene3d::get()->setRenderContext(ctx, viewport);
	scene2d::setRenderContext(scene2d::get(), ctx);
	/* Last in the list, so it draws over the scene. */
	ui::setRenderContext(ui::get(), ctx);

	start(&viewport->fb_index);

	if (ctx->element3d_count > 0) {
		light::set(light::get());
		fog::set(fog::get());

		/* The elements grouped by material: a counting sort over the table
		   ids, stable, so the elements of one entity stay together inside a
		   material and the matrix changes as little as it can. A material
		   that reads what is already drawn (blending, decal) keeps its scene
		   order instead, after everything opaque. */
		uint16_t start[Mesh::MAX_MATERIALS + 2] = {0};
		uint16_t order[Render::MAX_3D_ELEMENTS];
		uint16_t deferred[Render::MAX_3D_ELEMENTS];
		uint16_t sorted_count = 0, deferred_count = 0;

		for (uint16_t i = 0; i < ctx->element3d_count; i++) {
			uint8_t m = ctx->element3d[i].material;
			if (mesh::material::isDeferred(m)) deferred[deferred_count++] = i;
			else start[materialBucket(m) + 1]++;
		}
		for (int b = 1; b <= Mesh::MAX_MATERIALS + 1; b++) start[b] += start[b - 1];
		for (uint16_t i = 0; i < ctx->element3d_count; i++) {
			uint8_t m = ctx->element3d[i].material;
			if (mesh::material::isDeferred(m)) continue;
			order[start[materialBucket(m)]++] = i;
			sorted_count++;
		}

		const T3DSkeleton *bound = NULL;
		const T3DMat4FP *pushed = NULL;
		bool vertex_fx = false;

		vertex_fx |= runElements(ctx, order, sorted_count, &bound, &pushed);
		vertex_fx |= runElements(ctx, deferred, deferred_count, &bound, &pushed);

		if (pushed) t3d_matrix_pop(1);

		/* A recorded material leaves its vertex FX set; whatever draws next
		   starts from a fresh state and would inherit it. */
		if (vertex_fx) t3d_state_set_vertex_fx(T3D_VERTEX_FX_NONE, 0, 0);
	}

	particles::draw();

	sprite::copySharedPalettes(viewport->fb_index);

	/* The texture the RDP has on, for the whole frame: a sprite in a
	   section finds what the one before it left, scissor or not. Whatever
	   draws its own way says what it overwrote. */
	sprite::Texture texture = {};

	/* Copy mode needs a 16 bit framebuffer: the same for every sprite. */
	const bool fb16 = display_get_bitdepth() == 2;

	for (int s = 0; s < ctx->section_count; s++) {
		Render::Section *section = &ctx->section[s];

		/* Nothing on screen, nothing on the wire: a fully hidden section
		   must not emit a single command, scissor setup included. */
		bool any_visible = false;
		for (int i = 0; i < section->element_count; i++) {
			if (!ctx->element2d[section->element_start + i].graphic->is_hidden) {
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

		/* The sprite state the RDP is already programmed with: the key it
		   was set for, or SPRITE_MODE_UNSET when a shape or a text left the
		   mode on something else. Only a change pays. */
		int sprite_mode = SPRITE_MODE_UNSET;

		for (int i = 0; i < section->element_count; i++) {
			Render::Element2D *element = &ctx->element2d[section->element_start + i];
			const Graphic *graphic = element->graphic;
			if (graphic->is_hidden) continue;
			switch (graphic->type) {
				case Graphic::RECTANGLE:
					rectangle::draw(&graphic->rectangle, element->position, element->scale);
					sprite_mode = SPRITE_MODE_UNSET;
					sprite::forgetTexture(&texture);
					break;
				case Graphic::TEXT:
					text::draw(&graphic->text, element->position);
					sprite_mode = SPRITE_MODE_UNSET;
					sprite::forgetPalettes(&texture);
					break;
				case Graphic::SPRITE: {
					bool copy = isCopyable(element, fb16);
					rdpq_tlut_t tlut = rdpq_tlut_from_format(sprite_get_format(graphic->sprite.asset));
					int mode = graphic->transparency
					         | (copy ? SPRITE_MODE_COPY : 0)
					         | (tlut != TLUT_NONE ? SPRITE_MODE_TLUT : 0);

					if (sprite_mode != mode) {
						setSpriteMode(graphic->transparency, copy, tlut);
						sprite_mode = mode;
					}

					if (graphic->sprite.tiled) sprite::drawTiled(&graphic->sprite, &texture, element->position, element->scale);
					else sprite::draw(&graphic->sprite, &texture, element->position, element->scale, element->rotation);
					break;
				}
			}
		}

		/* Back to the whole screen, whatever the display mode is now. */
		if (section->has_scissor)
			rdpq_set_scissor(0, 0, display_get_width(), display_get_height());
	}

	debug::ui::draw();

	end();
}

}

}
