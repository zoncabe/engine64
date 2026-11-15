/*
	Content declaration: everything an entity can be made of, each as an
	array with its count. Count zero means the entity has none of that. No
	tag: what the prefab holds is what the entity is, in 2D or 3D alike.
	Position, rotation and scale are not here, they belong to the scene
	entry, so one prefab can be placed any number of times.
*/
#ifndef ENGINE64_PREFAB_H
#define ENGINE64_PREFAB_H

#include <stdint.h>

#include "model/e64_mesh.h"
#include "graphics/e64_graphic.h"
#include "character3d/e64_character3d.h"
#include "physics3d/e64_rigid_body.h"
#include "physics3d/collision/e64_collider.h"
#include "physics3d/e64_cloth.h"
#include "shaders/e64_water.h"
#include "sound/e64_sound.h"

namespace e64 {

/* Their headers include the entity's, which includes this one. */
namespace character2d { struct Def; }
namespace stage2d { struct Def; }


struct Prefab {

	/* What is drawn: models in 3D, graphics in 2D. The entity copies each
	   graphic and loads its sprite. */
	const Mesh::Def *mesh;
	uint8_t mesh_count;

	const Graphic *graphic;
	uint8_t graphic_count;

	const character3d::Def *character3d;
	uint8_t character3d_count;

	const character2d::Def *character2d;
	uint8_t character2d_count;

	const stage2d::Def *stage;
	uint8_t stage_count;
	
	const RigidBody::Def *body;
	const collider::Def *collider;

	const Cloth::Def *cloth;
	uint8_t cloth_count;

	const Water::Def *water;
	uint8_t water_count;

	/* Opened in the entity from create to delete. The looping ones play on
	   their own; the rest wait for whoever fires them. */
	const Sound::Def *const *sound;
	uint8_t sound_count;

	/* 3D: test the model's bounding box against the view frustum before
	   drawing it. */
	bool cull;

};

}

#endif
