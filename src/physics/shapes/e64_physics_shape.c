/*
	Narrowphase/body dispatchers. Each function builds the world transform
	(body_tx · shape->local) and routes to the concrete shape implementation
	based on shape->type.
*/
#include <stddef.h>

#include "physics/shapes/e64_physics_shape.h"
#include "physics/collision/e64_collision_mesh.h"


Transform shapeDef_localTransform(const Transform *tx)
{
	Transform local = *tx;

	if (vector3_squaredMagnitude(&local.rotation.ex) == 0.0f &&
	    vector3_squaredMagnitude(&local.rotation.ey) == 0.0f &&
	    vector3_squaredMagnitude(&local.rotation.ez) == 0.0f)
		local.rotation = matrix3_identity();

	return local;
}


bool physicsShape_fromDef(PhysicsShape *shape, const PhysicsShapeDef *def, Vector3 scale)
{
	const Transform *tx = NULL;

	*shape = (PhysicsShape){ .type = def->type };

	switch (def->type) {
		case SHAPE_BOX:
			shape->box = (Box){ .e = {
				def->box.e.x * scale.x,
				def->box.e.y * scale.y,
				def->box.e.z * scale.z,
			}};
			shape->friction    = def->box.friction;
			shape->restitution = def->box.restitution;
			shape->density     = def->box.density;
			shape->sensor      = def->box.sensor;
			tx = &def->box.tx;
			break;

		/* A sphere cannot be squashed, so a non-uniform scale takes X. */
		case SHAPE_SPHERE:
			shape->sphere      = (Sphere){ .radius = def->sphere.radius * scale.x };
			shape->friction    = def->sphere.friction;
			shape->restitution = def->sphere.restitution;
			shape->density     = def->sphere.density;
			shape->sensor      = def->sphere.sensor;
			tx = &def->sphere.tx;
			break;

		/* The capsule runs along local Z: radius takes X, height takes Z. */
		case SHAPE_CAPSULE:
			shape->capsule = (Capsule){
				.radius      = def->capsule.radius * scale.x,
				.half_height = def->capsule.half_height * scale.z,
			};
			shape->friction    = def->capsule.friction;
			shape->restitution = def->capsule.restitution;
			shape->density     = def->capsule.density;
			shape->sensor      = def->capsule.sensor;
			tx = &def->capsule.tx;
			break;

		/* The mesh comes from an asset, so the def names a file instead of
		   describing a size. Scale is ignored: the triangles are already at
		   the size they were authored. */
		case SHAPE_MESH:
			shape->mesh = collisionMesh_load(def->mesh.path);
			if (shape->mesh == NULL) return false;

			shape->friction    = def->mesh.friction;
			shape->restitution = def->mesh.restitution;
			shape->density     = 0.0f;
			tx = &def->mesh.tx;
			break;
	}

	shape->local = shapeDef_localTransform(tx);
	shape->local.position = (Vector3){
		shape->local.position.x * scale.x,
		shape->local.position.y * scale.y,
		shape->local.position.z * scale.z,
	};

	return true;
}


/* Counterpart of physicsShape_fromDef: only the mesh case allocates, and the
   asset it loaded dies with the shape that asked for it. */
void physicsShape_release(PhysicsShape *shape)
{
	if (shape->type != SHAPE_MESH || shape->mesh == NULL) return;

	collisionMesh_delete(shape->mesh);
	shape->mesh = NULL;
}


int physicsShape_testPoint(const PhysicsShape *shape, const Transform *body_tx, const Vector3 *p)
{
	Transform world = transform_product(body_tx, &shape->local);

	switch (shape->type) {
		case SHAPE_BOX:     return box_testPoint    (&shape->box,     &world, p);
		case SHAPE_SPHERE:  return sphere_testPoint (&shape->sphere,  &world, p);
		case SHAPE_CAPSULE: return capsule_testPoint(&shape->capsule, &world, p);
		case SHAPE_MESH:    break;   /* static-only, never on a rigid body */
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
		case SHAPE_MESH:    return collisionMesh_raycast(shape->mesh, &world, raycast);
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

		/* The tree's root already bounds every triangle, in mesh-local space:
		   shifting it by the shape's own transform puts it in the world. */
		case SHAPE_MESH: {
			*aabb = (AABB){ vector3_zero(), vector3_zero() };
			if (shape->mesh == NULL || shape->mesh->tree.root < 0) break;

			*aabb = dynamicAABBTree_getFatAABB(&shape->mesh->tree, shape->mesh->tree.root);
			aabb->min = vector3_sum(&aabb->min, &world.position);
			aabb->max = vector3_sum(&aabb->max, &world.position);
			break;
		}
	}
}


void physicsShape_computeMass(const PhysicsShape *shape, MassData *md)
{
	switch (shape->type) {
		case SHAPE_BOX:     box_computeMass    (&shape->box,     &shape->local, shape->density, md); break;
		case SHAPE_SPHERE:  sphere_computeMass (&shape->sphere,  &shape->local, shape->density, md); break;
		case SHAPE_CAPSULE: capsule_computeMass(&shape->capsule, &shape->local, shape->density, md); break;
		case SHAPE_MESH:    break;   /* static-only, never on a rigid body */
	}
}
