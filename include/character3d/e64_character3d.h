#ifndef ENGINE64_CHARACTER3D_H
#define ENGINE64_CHARACTER3D_H

#include <stdbool.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>

#include "physics3d/e64_physics.h"
#include "model/e64_mesh.h"
#include "model/e64_skeleton.h"
#include "model/e64_spring_bone.h"
#include "character3d/e64_character3d_collision.h"
#include "character3d/e64_character3d_movement.h"

namespace e64 {

class Entity;

/* An aggregate on purpose: no constructor, public data. character3d::create
   builds it with a designated initializer over one allocation that also
   carries the spring bones behind it. */
class Character3D {
public:

	Entity *entity;
	character3d::Collider collider;
	character3d::Movement movement;
	skeleton::Modifiers skeleton_modifiers;

	void updateMovement(character3d::MovementCommand *cmd, float dt);
};


namespace character3d {

typedef struct Def {

	const MovementSettings *movement_settings;
	const ColliderSettings *collider_settings;
	const SpringBonesDef *spring_bones; /* optional: array of sets, one tuning each, count 0 terminates */

} Def;


Character3D *create(const Def *def, Entity *entity);
void destroy(Character3D *character);

}


}

#endif
