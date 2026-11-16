/*
	Where something drawn stands in the 3D scene: the one form everything
	placed there declares and carries. Position in metres, rotation as the
	quaternion the matrix of a model is written from and a rigid body already
	keeps, and scale.

	Nothing here belongs to an entity: a mesh, whatever is placed, has its
	own.

	The physics has no scale and does not use this: it places its bodies with
	its own Transform (math/e64_transform.h, position and a 3x3 matrix).
	toPhysics and fromBody cross between the two.
*/
#ifndef ENGINE64_RENDER_TRANSFORM3D_H
#define ENGINE64_RENDER_TRANSFORM3D_H

#include <t3d/t3dmath.h>

#include "math/e64_vector3.h"
#include "math/e64_quaternion.h"
#include "math/e64_transform.h"

namespace e64 {

class RigidBody;


typedef struct Transform3D {

	Vector3 position; /* metres */
	Quaternion rotation;
	Vector3 scale;

} Transform3D;


namespace transform3d {

/* At the origin, unrotated, at its own size. */
Transform3D identity(void);

/* What a declaration means: a rotation or a scale left at zero is one
   nobody wrote, and stands for none. Whoever opens a def reads its
   transform through this. */
Transform3D fromDeclaration(const Transform3D *declared);

/* From angles in degrees, in the x, y, z order of quaternion::fromEuler,
   for a rotation written by hand. */
Transform3D fromEuler(const Vector3 *position, const Vector3 *degrees, const Vector3 *scale);

/* The child placed inside the parent: scales multiply per axis and
   rotations compose. Equal to the product of the two matrices while the
   parent's scale is uniform or the child carries no rotation. */
Transform3D product(const Transform3D *parent, const Transform3D *child);

/* The transform that undoes this one. Exact while the scale is uniform;
   for a point, mulVectorInverse is exact always. */
Transform3D inverse(const Transform3D *t);

/* A point of the transform's own space, in the scene's. */
Vector3 mulVector(const Transform3D *t, const Vector3 *v);

/* A point of the scene, in the transform's own space. */
Vector3 mulVectorInverse(const Transform3D *t, const Vector3 *v);

/* The RSP matrix of the placement, straight in fixed point. The one place
   where metres become render units. */
void toMatrix(const Transform3D *t, T3DMat4FP *out);

/* Position and rotation in the solver's form; the scale stays behind. */
Transform toPhysics(const Transform3D *t);

/* Where a body stands, at the given scale: a body has none of its own. */
Transform3D fromBody(const RigidBody *body, const Vector3 *scale);

}

}

#endif
