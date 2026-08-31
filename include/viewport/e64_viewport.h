#ifndef ENGINE64_VIEWPORT_H
#define ENGINE64_VIEWPORT_H

#include <t3d/t3d.h>
#include "camera/e64_camera.h"
#include "physics/math/e64_vector3.h"

#define FB_COUNT 3


typedef struct Viewport {

	T3DViewport t3d_viewport;
	Camera camera;
	int fb_index;

} Viewport;

Viewport *viewport_get(void);


void viewport_init(void);
void viewport_clear(color_t color);
void viewport_updateCamera(Vector3 *center);

/* The projection is the game's call, so it picks one and keeps it fed. */
void viewport_setPerspectiveCamera(void);
void viewport_setIsometricCamera(void);

#endif
