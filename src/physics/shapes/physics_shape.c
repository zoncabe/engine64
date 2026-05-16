/*
	physics_shape.c — narrowphase/body dispatchers. Each function builds the
	world transform (body_tx · shape->local) and routes to the concrete shape
	implementation based on shape->type.
*/
#include "physics/shapes/physics_shape.h"


int physicsShape_testPoint(const PhysicsShape *shape, const Transform *body_tx, const Vector3 *p)
{
	Transform world = transform_product(body_tx, &shape->local);

	switch (shape->type) {
		case SHAPE_BOX:     return box_testPoint    (&shape->box,     &world, p);
		case SHAPE_SPHERE:  return sphere_testPoint (&shape->sphere,  &world, p);
		case SHAPE_CAPSULE: return capsule_testPoint(&shape->capsule, &world, p);
	}
	return 0;
}


int physicsShape_raycast(const PhysicsShape *shape, const Transform *body_tx, RaycastData *raycast)
{
	Transform world = transform_product(body_tx, &shape->local);

	switch (shape->type) {
		case SHAPE_BOX:     return box_raycast    (&shape->box,     &world, raycast);
		case SHAPE_SPHERE:  return sphere_raycast (&shape->sphere,  &world, raycast);
		case SHAPE_CAPSULE: return capsule_raycast(&shape->capsule, &world, raycast);
	}
	return 0;
}


void physicsShape_computeAABB(const PhysicsShape *shape, const Transform *body_tx, AABB *aabb)
{
	Transform world = transform_product(body_tx, &shape->local);

	switch (shape->type) {
		case SHAPE_BOX:     box_computeAABB    (&shape->box,     &world, aabb); break;
		case SHAPE_SPHERE:  sphere_computeAABB (&shape->sphere,  &world, aabb); break;
		case SHAPE_CAPSULE: capsule_computeAABB(&shape->capsule, &world, aabb); break;
	}
}


void physicsShape_computeMass(const PhysicsShape *shape, MassData *md)
{
	switch (shape->type) {
		case SHAPE_BOX:     box_computeMass    (&shape->box,     &shape->local, shape->density, md); break;
		case SHAPE_SPHERE:  sphere_computeMass (&shape->sphere,  &shape->local, shape->density, md); break;
		case SHAPE_CAPSULE: capsule_computeMass(&shape->capsule, &shape->local, shape->density, md); break;
	}
}
