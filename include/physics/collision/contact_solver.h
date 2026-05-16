/*
	contact_solver.h — sequential impulse constraint solver.
	Ported from qu3e q3ContactSolver.
*/
#ifndef PHYSICS_COLLISION_CONTACT_SOLVER_H
#define PHYSICS_COLLISION_CONTACT_SOLVER_H

#include <stdint.h>

#include "physics/math/vector3.h"
#include "physics/math/matrix3.h"
#include "physics/physics_settings.h"


struct PhysicsIsland;
struct VelocityState;


typedef struct ContactState {
	Vector3 ra;
	Vector3 rb;
	float   penetration;
	float   normal_impulse;
	float   tangent_impulse[2];
	float   bias;
	float   normal_mass;
	float   tangent_mass[2];
} ContactState;


typedef struct ContactConstraintState {
	ContactState contacts[8];
	int32_t      contact_count;
	Vector3      tangent_vectors[2];
	Vector3      normal;
	Vector3      center_a;
	Vector3      center_b;
	Matrix3      iA;
	Matrix3      iB;
	float        mA;
	float        mB;
	float        restitution;
	float        friction;
	int32_t      index_a;
	int32_t      index_b;
} ContactConstraintState;


typedef struct ContactSolver {
	struct PhysicsIsland     *island;
	ContactConstraintState   *contacts;
	int32_t                   contact_count;
	struct VelocityState     *velocities;
	int                       enable_friction;
} ContactSolver;


void contactSolver_initialize(ContactSolver *s, struct PhysicsIsland *island);
void contactSolver_shutdown  (ContactSolver *s);
void contactSolver_preSolve  (ContactSolver *s, float dt);
void contactSolver_solve     (ContactSolver *s);


#endif
