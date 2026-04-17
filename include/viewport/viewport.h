#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <t3d/t3d.h>
#include "camera/camera.h"
#include "physics/math/vector3.h"

#define FB_COUNT 3


typedef struct ControllerActions ControllerActions;

typedef struct Viewport {

	T3DViewport t3d_viewport;
	Camera camera;
	int fb_index;

} Viewport;

Viewport *viewport_get(void);


void viewport_init(void);
void viewport_clear(void);
void viewport_updateCamera(const ControllerActions *actions, Vector3 *center);

#endif
