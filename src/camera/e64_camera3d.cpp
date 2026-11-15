#include <fmath.h>

#include "math/e64_math.h"
#include "camera/e64_camera3d.h"

namespace e64 {

/* Position and target one unit apart so the view matrix is not degenerate
   before anything places the camera. The lens starts at a mid value inside
   its bounds; the scene writes the real one. */
Camera3D::Camera3D()
	: position{ 0.0f, 1.0f, 0.0f },
	  target{ 0.0f, 0.0f, 0.0f },
	  up{ 0.0f, 0.0f, 1.0f },
	  near_clipping(1.0f),
	  far_clipping(200.0f),
	  field_of_view(60.0f)
{
}

void Camera3D::setFieldOfView(float fov)
{
	field_of_view = clampf(fov, E64_CAMERA_FOV_MIN, E64_CAMERA_FOV_MAX);
}

float Camera3D::getFieldOfView() const
{
	return field_of_view;
}

float Camera3D::getYaw() const
{
	float dx = target.x - position.x;
	float dy = target.y - position.y;

	if (dx == 0.0f && dy == 0.0f) return 0.0f;

	return rad_to_deg(fm_atan2f(dx, dy));
}

/* Positive looking up, negative looking down. */
float Camera3D::getPitch() const
{
	float dx = target.x - position.x;
	float dy = target.y - position.y;
	float dz = target.z - position.z;

	float horizontal = sqrtf(dx * dx + dy * dy);

	if (horizontal == 0.0f && dz == 0.0f) return 0.0f;

	return rad_to_deg(fm_atan2f(dz, horizontal));
}

Vector3 Camera3D::getRight() const
{
	float dx = target.x - position.x;
	float dy = target.y - position.y;

	float length = sqrtf(dx * dx + dy * dy);
	if (length < 1e-4f) return vector3::create(0.0f, -1.0f, 0.0f);

	/* Z is up, so right is forward crossed with it. */
	return vector3::create(dy / length, -dx / length, 0.0f);
}

}
