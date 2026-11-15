#ifndef ENGINE64_SPRING_ARM_H
#define ENGINE64_SPRING_ARM_H

#include "math/e64_vector3.h"
#include "camera/e64_camera3d.h"

namespace e64 {

/* Absolute zoom-in floor, below any minimum the game picks. At zero the
   camera lands on the point it is watching, the view direction comes out a
   null vector, and normalising that poisons every number downstream. */
#define E64_SPRING_ARM_MIN_LENGTH 0.1f


/* The rail: a camera held at a distance behind a pivot, on a yaw and a
   pitch around it. Writes the camera's position, target and up once per
   update. No smoothing and no input live here; the game writes the pose
   it wants and eases it beforehand if it wants a gradual change. */
class SpringArm {
public:

	/* Shoulder shift along the camera's right, and pivot lift above the
	   center it is handed. */
	float side_offset;
	float height_offset;

	/* Degrees. Positive looks down on the pivot. */
	float pitch;

	/* Zoom range the game allows; the floor above still applies under it. */
	float min_length;
	float max_length;

	SpringArm();

	/* Clamped to the zoom range. */
	void setLength(float length);
	float getLength() const;

	/* Wrapped to -180..180 so it never grows without bound. Degrees. */
	void setYaw(float yaw);
	float getYaw() const;

	void update(Camera3D *camera, const Vector3 *center) const;

private:

	float arm_length;
	float yaw;
};

}

#endif
