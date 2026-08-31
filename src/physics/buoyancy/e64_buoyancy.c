/*
	Archimedes buoyancy over the bodies inside a water sensor.

	The bodies in the water come from the sensor's own contact list: the
	broadphase keeps those pairs alive and the narrowphase marks them
	COLLIDING, the island solver just never resolves them. No body scan.

	Each shape is sampled as one or more vertical columns. A column's
	submerged fraction is measured against the surface height at its own
	(x, y), so a wave lifting one end of a box and not the other produces
	the torque that rocks it; the same offset is what rights a tilted
	floating body, since the deeper samples push harder. The sphere keeps
	its closed-form spherical cap instead of a column.

	Forces are accumulated per body and applied before the islands are
	built, so they integrate in the same step. applyLinearForce takes an
	acceleration (it scales by mass), applyTorque takes a real torque.
*/
#include <math.h>

#include "physics/buoyancy/e64_buoyancy.h"
#include "physics/world/e64_physics_world.h"
#include "physics/body/e64_rigid_body.h"
#include "physics/collision/e64_contact.h"
#include "physics/math/e64_math_common.h"


/* Sample columns per box shape: the minimum that tilts on both axes. */
#define BUOYANCY_BOX_COLUMNS 4


/* Fraction of a vertical column [bottom, top] sitting under the surface. */
static float buoyancy_columnFraction(float bottom, float top, float surface)
{
	if (surface <= bottom) return 0.0f;
	if (surface >= top)    return 1.0f;
	return (surface - bottom) / (top - bottom);
}

/* Submerged fraction of a sphere: spherical cap of height h over the full
   volume, h measured from the bottom of the sphere up to the surface. */
static float buoyancy_sphereFraction(float center_z, float radius, float surface)
{
	float h = surface - (center_z - radius);
	if (h <= 0.0f)          return 0.0f;
	if (h >= 2.0f * radius) return 1.0f;
	return h * h * (3.0f * radius - h) / (4.0f * radius * radius * radius);
}


typedef struct BuoyancyAccum {
	Vector3 force;       /* real force, world space */
	Vector3 torque;      /* real torque about the center of mass */
	float   volume;      /* displaceable volume of the whole body */
	float   submerged;   /* submerged part of that volume */
} BuoyancyAccum;

/* One sample: a share of the shape's volume tested at a world point. Its
   buoyant force pushes against gravity and its arm from the center of mass
   is what turns uneven submersion into torque. */
static void buoyancy_addSample(BuoyancyAccum *accum, const BuoyancyVolume *volume,
                               const Vector3 *gravity, const Vector3 *center_of_mass,
                               const Vector3 *point, float share_volume, float fraction)
{
	accum->volume    += share_volume;
	accum->submerged += share_volume * fraction;
	if (fraction <= 0.0f) return;

	Vector3 force = vector3_scaled(gravity, -volume->density * share_volume * fraction);
	vector3_add(&accum->force, &force);

	Vector3 arm = vector3_difference(point, center_of_mass);
	Vector3 t   = vector3_cross(&arm, &force);
	vector3_add(&accum->torque, &t);
}

static void buoyancy_sampleShape(BuoyancyAccum *accum, const BuoyancyVolume *volume,
                                 const Vector3 *gravity, const RigidBody *body,
                                 const PhysicsShape *shape)
{
	Transform tx = transform_product(&body->tx, &shape->local);

	switch (shape->type) {
		case SHAPE_SPHERE: {
			float r = shape->sphere.radius;
			float v = (4.0f / 3.0f) * PI * r * r * r;

			float surface  = volume->surface_height(volume->surface, tx.position.x, tx.position.y);
			float fraction = buoyancy_sphereFraction(tx.position.z, r, surface);
			buoyancy_addSample(accum, volume, gravity, &body->world_center, &tx.position, v, fraction);
			break;
		}

		case SHAPE_BOX: {
			const Vector3 *e = &shape->box.e;
			float v = 8.0f * e->x * e->y * e->z;

			/* World-z half-extent of the OBB: its slab along the vertical. */
			float hz = fabsf(tx.rotation.ex.z) * e->x
			         + fabsf(tx.rotation.ey.z) * e->y
			         + fabsf(tx.rotation.ez.z) * e->z;

			/* Four columns halfway to the corners, a quarter volume each. */
			static const float offset[BUOYANCY_BOX_COLUMNS][2] = {
				{ -0.5f, -0.5f }, { 0.5f, -0.5f }, { -0.5f, 0.5f }, { 0.5f, 0.5f },
			};
			for (int i = 0; i < BUOYANCY_BOX_COLUMNS; i++) {
				Vector3 local = { e->x * offset[i][0], e->y * offset[i][1], 0.0f };
				Vector3 point = transform_mulVector(&tx, &local);

				float surface  = volume->surface_height(volume->surface, point.x, point.y);
				float fraction = buoyancy_columnFraction(point.z - hz, point.z + hz, surface);
				buoyancy_addSample(accum, volume, gravity, &body->world_center, &point,
				                   v / BUOYANCY_BOX_COLUMNS, fraction);
			}
			break;
		}

		case SHAPE_CAPSULE: {
			float r  = shape->capsule.radius;
			float hh = shape->capsule.half_height;
			float v  = PI * r * r * (2.0f * hh) + (4.0f / 3.0f) * PI * r * r * r;

			/* One column of radius r at each end of the segment: exact when
			   the capsule floats on its side, approximate upright. */
			Vector3 a, b;
			capsule_getSegment(&shape->capsule, &tx, &a, &b);

			const Vector3 *end[2] = { &a, &b };
			for (int i = 0; i < 2; i++) {
				float surface  = volume->surface_height(volume->surface, end[i]->x, end[i]->y);
				float fraction = buoyancy_columnFraction(end[i]->z - r, end[i]->z + r, surface);
				buoyancy_addSample(accum, volume, gravity, &body->world_center, end[i], v * 0.5f, fraction);
			}
			break;
		}

		case SHAPE_MESH:   /* static-only, never on a dynamic body */
			break;
	}
}

static void buoyancy_applyToBody(const PhysicsWorld *world, const BuoyancyVolume *volume, RigidBody *body)
{
	BuoyancyAccum accum = {0};

	for (const PhysicsShape *shape = body->shapes; shape; shape = shape->next)
		buoyancy_sampleShape(&accum, volume, &world->gravity, body, shape);

	if (accum.volume <= 0.0f || accum.submerged <= 0.0f) return;

	float fraction = accum.submerged / accum.volume;

	/* applyLinearForce multiplies by mass, so it is handed F/m. It also
	   wakes the body: floating under waves never settles, on purpose. */
	Vector3 acceleration = vector3_scaled(&accum.force, body->inv_mass);
	rigidBody_applyLinearForce(body, acceleration);
	rigidBody_applyTorque(body, accum.torque);

	/* Water drag, gated by how submerged the body is. The angular term uses
	   the mass as a stand-in for the inertia, so both coefficients stay
	   per-second rates independent of the body's size. */
	Vector3 linear_drag = vector3_scaled(&body->linear_velocity,
	                                     -volume->linear_drag * fraction);
	rigidBody_applyLinearForce(body, linear_drag);

	Vector3 angular_drag = vector3_scaled(&body->angular_velocity,
	                                      -volume->angular_drag * fraction * body->mass);
	rigidBody_applyTorque(body, angular_drag);
}

void buoyancy_apply(struct PhysicsWorld *world, const BuoyancyVolume *volume)
{
	if (volume->body == NULL || volume->surface_height == NULL) return;

	for (const ContactEdge *edge = volume->body->contact_list; edge; edge = edge->next) {
		if (!(edge->constraint->flags & CONSTRAINT_COLLIDING)) continue;
		if (!(edge->other->flags & BODY_FLAG_DYNAMIC)) continue;

		buoyancy_applyToBody(world, volume, edge->other);
	}
}
