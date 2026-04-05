#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <t3d/t3d.h>
#include "../camera/camera.h"
#include "../physics/math/vector3.h"

typedef struct ControllerActions ControllerActions;


#define FB_COUNT 3

typedef struct Viewport {

	T3DViewport t3d_viewport;
	Camera camera;
	int fb_index;

} Viewport;

Viewport *viewport_get(void);

void viewport_init(void);
void viewport_clear(void);
void viewport_setOrbitalCamera(const ControllerActions *actions, Vector3 *target);

#endif
