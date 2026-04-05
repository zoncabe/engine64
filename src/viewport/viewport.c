#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3danim.h>

#include "../../include/time/time.h"
#include "../../include/physics/physics.h"
#include "../../include/control/controller.h"
#include "../../include/actor/actor.h"
#include "../../include/light/lighting.h"
#include "../../include/camera/camera.h"
#include "../../include/camera/camera_motion.h"
#include "../../include/control/camera_control.h"
#include "../../include/viewport/viewport.h"
#include "../../include/player/player.h"

const resolution_t WIDESCREEN = {.width = 424, .height = 240, .interlaced = INTERLACE_OFF};

static Viewport viewport;
Viewport* viewport_get(void) { return &viewport; }

void viewport_init()
{
	display_init(RESOLUTION_320x240, DEPTH_16_BPP, FB_COUNT, GAMMA_NONE, FILTERS_DISABLED);
	viewport.t3d_viewport = t3d_viewport_create_buffered(FB_COUNT);
	t3d_init((T3DInitParams){});
    camera_init(&viewport.camera);
    viewport.fb_index = 0;
}

void viewport_clear()
{
	rdpq_attach(display_get(), display_get_zbuf());
	t3d_frame_start();
	t3d_viewport_attach(&viewport.t3d_viewport);

	t3d_screen_clear_color(RGBA32(0, 0, 0, 0xFF));
	t3d_screen_clear_depth();
}

void viewport_setPerspectiveCamera()
{
    t3d_viewport_set_projection(
        &viewport.t3d_viewport, 
        T3D_DEG_TO_RAD(viewport.camera.field_of_view), 
        viewport.camera.near_clipping,
		viewport.camera.far_clipping
    );

    t3d_viewport_look_at(
        &viewport.t3d_viewport, 
        &(T3DVec3){{viewport.camera.position.x, viewport.camera.position.y, viewport.camera.position.z}}, 
        &(T3DVec3){{viewport.camera.target.x, viewport.camera.target.y, viewport.camera.target.z}}, 
        &(T3DVec3){{0, 0, 1}}
    );
}

void viewport_setIsometricCamera()
{
    t3d_viewport_set_ortho(
        &viewport.t3d_viewport,
        -640, 640,
        -480, 480,
        viewport.camera.near_clipping,
        viewport.camera.far_clipping
    );

    t3d_viewport_look_at(
        &viewport.t3d_viewport, 
        &(T3DVec3){{viewport.camera.position.x, viewport.camera.position.y, viewport.camera.position.z}}, 
        &(T3DVec3){{viewport.camera.target.x, viewport.camera.target.y, viewport.camera.target.z}}, 
        &(T3DVec3){{0, 0, 1}}
    );
}

void viewport_setOrbitalCamera(const ControllerActions *actions, Vector3 *target)
{
    cameraControl_setOrbitalInput(&viewport.camera, actions);
    camera_setOrbitalMotion(&viewport.camera, target);
    viewport_setPerspectiveCamera();
    //viewport_setIsometricCamera();
}
