#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <t3d/t3d.h>
#include "../camera/camera.h"
#include "../control/control.h"
#include "../physics/math/vector3.h"


#define FB_COUNT 3

typedef struct Viewport {

	T3DViewport t3d_viewport;
	Camera camera;
	int fb_index;

} Viewport;

Viewport* viewport_get(void);

void viewport_init();
void viewport_clear();
void viewport_setOrbitalCamera(ControllerData *control, Vector3 *target);

#endif