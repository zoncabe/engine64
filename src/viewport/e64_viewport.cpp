#include <assert.h>

#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3danim.h>

#include "engine/e64_common.h"
#include "camera/e64_camera3d.h"
#include "menu/e64_settings.h"
#include "viewport/e64_viewport.h"

namespace e64 {

namespace viewport {

static Viewport v;

Viewport *get(void) { return &v; }

}


/* The camera stands in the world, so it is in metres like everything else in a
   scene, and it has to hand the view over in the units the models are drawn in.
   That is the whole of the conversion on this side. */
void Viewport::setPerspective3D(const Camera3D *camera)
{
	t3d_viewport_set_projection(
		&t3d_viewport,
		T3D_DEG_TO_RAD(camera->getFieldOfView()),
		camera->near_clipping * RENDER_SCALE,
		camera->far_clipping * RENDER_SCALE
	);

	T3DVec3 eye = {{camera->position.x * RENDER_SCALE, camera->position.y * RENDER_SCALE, camera->position.z * RENDER_SCALE}};
	T3DVec3 target = {{camera->target.x * RENDER_SCALE, camera->target.y * RENDER_SCALE, camera->target.z * RENDER_SCALE}};
	T3DVec3 up = {{camera->up.x, camera->up.y, camera->up.z}};
	t3d_viewport_look_at(&t3d_viewport, &eye, &target, &up);
}

void Viewport::setIsometric3D(const Camera3D *camera)
{
	t3d_viewport_set_ortho(
		&t3d_viewport,
		-640, 640,
		-480, 480,
		camera->near_clipping,
		camera->far_clipping
	);

	T3DVec3 eye = {{camera->position.x, camera->position.y, camera->position.z}};
	T3DVec3 target = {{camera->target.x, camera->target.y, camera->target.z}};
	T3DVec3 up = {{camera->up.x, camera->up.y, camera->up.z}};
	t3d_viewport_look_at(&t3d_viewport, &eye, &target, &up);
}

/* A width of zero is a viewport nobody gave a screen yet. Needed because
   the swap below closes the open screen first, and closing one that was
   never opened is not allowed. */
bool Viewport::hasScreen() const { return mode.width > 0; }

/* Opens the screen the game asked for with the player's choices laid over
   it. Both halves are read here and nowhere else, so a change to either one
   takes the same path.

   Closed and opened again rather than switched in place: libdragon's hot
   switch reuses the buffers it already reserved, so it can only shrink the
   picture, never grow it, and it lives behind the preview flag besides. */
void Viewport::applyMode()
{
	const ModeDef *mode = &this->mode;
	const Settings *settings = settings::get();

	/* The picture's shape and the margin for a television's overscan are
	   settings libdragon also keeps behind that flag, so the screen fills a
	   4:3 frame and nothing is trimmed. */
	resolution_t resolution = {
		.width = mode->width,
		.height = mode->height,
		.interlaced = mode->interlaced,
	};

	/* Gamma is a level from 0 to 100 and half of it is neutral: only above
	   that is the correction asked for. Below neutral there is nothing to
	   ask the video hardware, which darkens no picture. */
	gamma_t gamma = settings->gamma > 50 ? GAMMA_CORRECT_DITHER : GAMMA_NONE;

	/* Matrices, skeletons and deform buffers rotate over FB_COUNT frames:
	   more display buffers than that would let the RSP still read a frame
	   the update is already overwriting. */
	assert(mode->buffers <= Viewport::FB_COUNT);

	/* The video hardware's own smoothing pass, off. It works on the coverage
	   each draw leaves behind, and 3D edges leave partial coverage where 2D
	   leaves it whole, so turning it on makes the picture change with
	   whatever happens to be on screen. Wiring it to a setting is a separate
	   job from opening the screen. */
	display_init(
		resolution,
		mode->depth ? mode->depth : DEPTH_16_BPP,
		mode->buffers ? mode->buffers : Viewport::FB_COUNT,
		gamma,
		FILTERS_DISABLED);
}

/* Nothing of the screen and nothing of tiny3d happens here: the engine
   picks no resolution of its own, and a game with no 3D scene never pays
   for the 3D pipeline. */
void Viewport::init()
{
	mode = (ModeDef){};
	fb_index = 0;
}

Vector2 Viewport::getScale() const
{
	return (Vector2){
		mode.scale_x > 0.0f ? mode.scale_x : 1.0f,
		mode.scale_y > 0.0f ? mode.scale_y : 1.0f,
	};
}

/* Whether the 3D pipeline is up. A view with no width was never opened. */
bool Viewport::has3D() const { return t3d_viewport.size[0] > 0; }

/* Brought up when a 3D scene is loaded and taken down when it is dropped,
   so the 2D game runs without it. The view is sized against the screen in
   force, so this comes after the mode is set. */
void Viewport::open3D()
{
	if (has3D()) return;

	t3d_init((T3DInitParams){});
	t3d_viewport = t3d_viewport_create_buffered(FB_COUNT);
	t3d_viewport_set_area(&t3d_viewport, 0, 0, display_get_width(), display_get_height());
}

void Viewport::close3D()
{
	if (!has3D()) return;

	t3d_destroy();
	t3d_viewport = (T3DViewport){};
}

void Viewport::setMode(const ModeDef *mode)
{
	assert(mode && mode->width > 0 && mode->height > 0);

	if (hasScreen()
	 && mode->width == this->mode.width
	 && mode->height == this->mode.height
	 && mode->interlaced == this->mode.interlaced
	 && mode->depth == this->mode.depth
	 && mode->buffers == this->mode.buffers) return;

	/* The buffers being handed back are the ones still being drawn into:
	   nothing may be in flight when they are freed. */
	if (hasScreen()) {
		rspq_wait();
		display_close();
	}

	this->mode = *mode;
	fb_index = 0;
	applyMode();
}

void Viewport::refreshMode()
{
	if (!hasScreen()) return;

	rspq_wait();
	display_close();
	applyMode();
}

/* The frame's buffers: what is drawn into until it is shown. */
void Viewport::attach()
{
	rdpq_attach(display_get(), display_get_zbuf());
}

void Viewport::clear(color_t color)
{
	rdpq_clear(color);
}

void Viewport::detach()
{
	rdpq_detach_show();
}

}
