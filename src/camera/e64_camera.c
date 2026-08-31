#include <fmath.h>

#include "physics/math/e64_math_common.h"
#include "camera/e64_camera.h"
#include "camera/e64_spring_arm.h"


static void (*camera_handler[CAMERA_TYPE_COUNT])(Camera *, Vector3 *, float) = {
	[CAMERA_TYPE_SPRING_ARM] = cameraSpringArm_update,
};

void camera_init(Camera *camera)
{
	/* Lens and placement come from the scene's CameraDef; position and target
	   are one unit apart so the view matrix is not degenerate before the first
	   update places them. */
	*camera = (Camera){
		.position = { 0.0f, 1.0f, 0.0f },
		.target   = { 0.0f, 0.0f, 0.0f },
		.type     = CAMERA_TYPE_NONE,
	};
}

void camera_reset(Camera *camera)
{
	camera_init(camera);
}

/* horizontal yaw of the view direction, in the yaw convention of the engine;
   measured from the view (position -> target) so lateral offsets like the
   shoulder cancel out instead of skewing the angle */
float camera_getAngleAround(const Camera *camera, const Vector3 *point)
{
	(void)point;

	float dx = camera->target.x - camera->position.x;
	float dy = camera->target.y - camera->position.y;

	if (dx == 0.0f && dy == 0.0f) return 0.0f;

	return rad_to_deg(fm_atan2f(dx, dy));
}

/* The camera's right hand side on the ground plane. */
Vector3 camera_getRight(const Camera *camera)
{
	float dx = camera->target.x - camera->position.x;
	float dy = camera->target.y - camera->position.y;

	float length = sqrtf(dx * dx + dy * dy);
	if (length < 1e-4f) return vector3_create(0.0f, -1.0f, 0.0f);

	/* Z is up, so right is forward crossed with it. */
	return vector3_create(dy / length, -dx / length, 0.0f);
}


/* Starts a transition to whatever center the camera is fed next, gliding out of
   the given point. Passing a duration of zero cuts straight to the new target. */
void camera_setViewTarget(Camera *camera, const Vector3 *from, float duration)
{
	camera->blend_from     = *from;
	camera->blend_elapsed  = 0.0f;
	camera->blend_duration = duration;
}

void camera_update(Camera *camera, Vector3 *center, float dt)
{
	if (camera->type == CAMERA_TYPE_NONE) return;

	Vector3 blended;

	if (camera->blend_elapsed < camera->blend_duration) {
		camera->blend_elapsed += dt;

		float t = camera->blend_elapsed / camera->blend_duration;
		if (t > 1.0f) t = 1.0f;

		float alpha = ease_cubic_in_out(t);

		blended.x = lerpf(camera->blend_from.x, center->x, alpha);
		blended.y = lerpf(camera->blend_from.y, center->y, alpha);
		blended.z = lerpf(camera->blend_from.z, center->z, alpha);

		center = &blended;
	}

	camera_handler[camera->type](camera, center, dt);
}
