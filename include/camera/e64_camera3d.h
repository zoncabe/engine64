#ifndef ENGINE64_CAMERA3D_H
#define ENGINE64_CAMERA3D_H

#include "math/e64_vector3.h"

namespace e64 {

/* Sane lens bounds, in degrees. Below the minimum the projection's
   cotangent blows past what the RSP's 16.16 matrices can hold (it crashed
   the fixed-point cast); above the maximum the image is unusable anyway. */
#define E64_CAMERA_FOV_MIN 10.0f
#define E64_CAMERA_FOV_MAX 120.0f


/* The view and the lens, nothing else: what the viewport projects. Whoever
   places it (a spring arm, the game) writes position, target and up directly.
   No smoothing, no input, no transitions live here. */
class Camera3D {
public:

	/* The three vectors of the view matrix: where the camera is, what it looks
	   at, and which way is up on screen. */
	Vector3 position;
	Vector3 target;
	Vector3 up;

	float near_clipping; // unused since t3d breaking changes now offer no near, it always clamp at 0
	float far_clipping;

	Camera3D();

	/* Clamped to the lens bounds above: the one lens value a direct write
	   could push past what the hardware tolerates. */
	void setFieldOfView(float field_of_view);
	float getFieldOfView() const;

	/* Angles of the view direction (position -> target), in degrees and in
	   the yaw convention of the engine. Lateral offsets like a shoulder
	   cancel out instead of skewing them. */
	float getYaw() const;
	float getPitch() const;

	/* The camera's right hand side on the ground plane. */
	Vector3 getRight() const;

private:

	float field_of_view;
};

}

#endif
