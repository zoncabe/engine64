#include <fmath.h>

#include "math/e64_math.h"
#include "camera/e64_spring_arm.h"

namespace e64 {

SpringArm::SpringArm()
	: side_offset(0.0f),
	  height_offset(0.0f),
	  pitch(0.0f),
	  min_length(E64_SPRING_ARM_MIN_LENGTH),
	  max_length(100.0f),
	  arm_length(5.0f),
	  yaw(0.0f)
{
}

void SpringArm::setLength(float length)
{
	float floor = min_length > E64_SPRING_ARM_MIN_LENGTH
	            ? min_length : E64_SPRING_ARM_MIN_LENGTH;

	arm_length = clampf(length, floor, max_length);
}

float SpringArm::getLength() const
{
	return arm_length;
}

void SpringArm::setYaw(float angle)
{
	yaw = angle_wrap(angle);
}

float SpringArm::getYaw() const
{
	return yaw;
}

void SpringArm::update(Camera3D *camera, const Vector3 *center) const
{
	float yaw_rad = deg_to_rad(yaw);
	float pitch_rad = deg_to_rad(pitch);

	float sin_yaw, cos_yaw, sin_pitch, cos_pitch;
	fm_sincosf(yaw_rad, &sin_yaw, &cos_yaw);
	fm_sincosf(pitch_rad, &sin_pitch, &cos_pitch);

	/* forward points from the camera toward the pivot; right is its horizontal perpendicular */
	Vector3 forward = { cos_pitch * sin_yaw, cos_pitch * cos_yaw, -sin_pitch };
	Vector3 right = { cos_yaw, -sin_yaw, 0.0f };

	Vector3 pivot = { center->x, center->y, center->z + height_offset };

	camera->position.x = pivot.x - forward.x * arm_length + right.x * side_offset;
	camera->position.y = pivot.y - forward.y * arm_length + right.y * side_offset;
	camera->position.z = pivot.z - forward.z * arm_length;

	camera->target.x = pivot.x + right.x * side_offset;
	camera->target.y = pivot.y + right.y * side_offset;
	camera->target.z = pivot.z;

	camera->up = vector3::create(0.0f, 0.0f, 1.0f);
}

}
