#include <assert.h>
#include <malloc.h>

#include <t3d/t3dmath.h>

#include "model/e64_mesh_deform.h"
#include "viewport/e64_viewport.h"
#include "scene3d/e64_light.h"
#include "scene3d/e64_fog.h"
#include "scene3d/e64_scene3d.h"
#include "character3d/e64_character3d_collision.h"
#include "physics3d/e64_physics.h"
#include "engine/e64_common.h"

namespace e64 {

static Scene3D scene;

namespace scene3d {

Scene3D *get(void) { return &scene; }

}


void Scene3D::load(const Def *def)
{
	/* A scene with no light def is left pitch black rather than lit by
	   something the game never asked for. */
	static const light::Def unlit = {};
	static const fog::Def no_fog = {};

	/* The 3D pipeline comes up with the scene that needs it and goes down
	   with it, so a game that never loads one never pays for it. */
	viewport::get()->open3D();

	const light::Def *light_def = def->light ? def->light : &unlit;
	const fog::Def *fog_def = def->fog ? def->fog : &no_fog;

	light::init(light_def);
	fog::init(fog_def);

	/* The camera goes back to its defaults with the rest: the game places it
	   from its onEnter, once the scene is up. */
	assert(entity_count == 0);
	*this = (Scene3D){};

	Vector3 gravity = { 0.0f, 0.0f, -9.8f };
	physics::world::init(&physics, physics::TIMESTEP, gravity, physics::SOLVER_ITERATIONS);
	physics::world::setWind(&physics, def->wind);

	for (int i = 0; i < def->instance_count; i++) {
		const Instance *instance = &def->instance[i];
		const Prefab *prefab = instance->prefab;

		/* The entity opens the prefab's content; the instance says where it
		   stands, and what depends on the place follows the transform. */
		Entity *e = entity::create(prefab);
		e->transform.position = instance->position;
		e->transform.rotation = instance->rotation;
		if (instance->scale.x != 0.0f || instance->scale.y != 0.0f || instance->scale.z != 0.0f)
			e->transform.scale = instance->scale;

		e->startLoopSounds();
		e->attachPhysics(&physics);

		/* Cloth and water deform the mesh of the same index. Both run in
		   metres, the vertex buffer in render units. */
		for (uint8_t c = 0; c < prefab->cloth_count && c < e->mesh_count; c++) {
			Cloth *cloth = physics::world::createCloth(&physics, &prefab->cloth[c]);
			if (!cloth) continue;

			cloth->culled = &e->mesh[c]->culled;
			mesh::deform::set(e->mesh[c], cloth->render_position, cloth->normal,
			                  NULL, cloth->particle_count, RENDER_SCALE);
		}

		for (uint8_t w = 0; w < prefab->water_count && w < e->mesh_count; w++) {
			Water *water = water::create(&prefab->water[w]);
			if (!water) continue;

			/* The offsets are what scroll the texture layers; render reads
			   them after the recorded material. */
			water->culled = &e->mesh[w]->culled;
			mesh::deform::set(e->mesh[w], water->position, water->normal,
			                  water->rgba, water->count, RENDER_SCALE);
			e->mesh[w]->texture_scroll = water->offset;

			/* The entity's body is the water's sensor volume: bind them and
			   the bodies inside it start floating. */
			if (e->body)
				water::bindPhysics(water, e->body, &physics);
		}

		for (uint8_t c = 0; c < prefab->character3d_count; c++) {
			assert(character3d_count < MAX_CHARACTERS);
			Character3D *ch = character3d::create(&prefab->character3d[c], e);
			character[character3d_count++] = ch;

			character3d::collision::createBody(ch, &physics);
		}

		/* A character is placed every frame by updateCharacters; everything
		   else gets its matrices once, here. */
		if (prefab->character3d_count == 0) {
			for (int fb = 0; fb < Viewport::FB_COUNT; fb++)
				e->setMatrix(fb);
		}

		addEntity(e);
	}
}

void Scene3D::clear(void)
{
	*this = (Scene3D){};
}

void Scene3D::unload(void)
{
	for (int i = 0; i < character3d_count; i++)
		character3d::destroy(character[i]);
	for (int i = 0; i < entity_count; i++)
		entity::destroy(entity[i]);
	water::clear();
	clear();
	physics::world::shutdown(&physics);

	viewport::get()->close3D();
}

/* The characters' half of the frame after physics::update: each one collides
   against the world, hands the outcome to its body, and from there to what
   draws it. Always these steps, always in this order, so no game writes them
   out. The list is the scene's, which is why it lives here. */
void Scene3D::updateCharacters(uint8_t fb_index)
{
	for (int i = 0; i < character3d_count; i++) {
		Character3D *ch = character[i];
		const character3d::Collider *collider = &ch->collider;

		character3d::collision::collide(ch, &physics);

		/* The collider's matrix as a quaternion, once per frame: the body and
		   every mesh take the same one. */
		Quaternion rotation = quaternion::fromMatrix3(&collider->transform.rotation);
		character3d::collision::syncBody(ch, &rotation);

		/* The model is drawn foot_offset above the capsule's bottom. */
		Vector3 position = collider->transform.position;
		position.z += collider->foot_offset - collider->shape.radius - collider->shape.half_height;

		Entity *e = ch->entity;
		for (uint8_t m = 0; m < e->mesh_count; m++)
			mesh::setMatrixFromBody(e->mesh[m], &position, &rotation, &e->transform.scale, fb_index);
	}
}

/* The same for everything else in the world. What the solver moved is
   somewhere new by the time the frame gets here, and the matrix it is drawn
   with has to say so. Anything still was placed when the scene loaded and this
   passes over it. The list is the scene's, which is why it lives here. */
void Scene3D::updateEntities(uint8_t fb_index)
{
	for (int i = 0; i < entity_count; i++)
		entity[i]->setMatrixFromBody(fb_index);
}

void Scene3D::addEntity(Entity *e)
{
	assert(entity_count < MAX_ENTITIES);
	entity[entity_count++] = e;
}

Character3D *Scene3D::getCharacter3D(uint8_t index)
{
	if (index >= character3d_count) return NULL;
	return character[index];
}

void Scene3D::setRenderContext(Render::Context *ctx, const Viewport *viewport) const
{
	uint8_t fb_index = viewport->fb_index;

	for (int i = 0; i < entity_count; i++) {
		const Entity *e = entity[i];

		for (uint8_t m = 0; m < e->mesh_count; m++) {
			Mesh *mesh = e->mesh[m];
			T3DSkeleton *skel = mesh->skeleton;

			/* The mesh culls itself and writes its own flags; here they are
			   only consumed. */
			if (e->cull) {
				mesh::cull(mesh, &viewport->t3d_viewport);
				if (mesh->culled) continue;
			}

			/* One element per object still on: its part turned on, and not
			   cut by the frustum when the entity culls. A part displaced from
			   the rest of the model carries a matrix of its own; every other
			   object draws with the entity's. */
			for (uint16_t i = 0; T3DObject *object = mesh::object(mesh, i); i++) {
				uint8_t part = mesh->object_part[i];
				if (!(mesh->visible & (1u << part))) continue;
				if (e->cull && !object->isVisible) continue;

				assert(ctx->element3d_count < Render::MAX_3D_ELEMENTS);
				ctx->element3d[ctx->element3d_count++] = (Render::Element3D){
					.material = object->userValue0,
					.object = object,
					.mesh = mesh,
					.matrix = (T3DMat4FP *)mesh::part::getMatrix(mesh, part, fb_index),
					.skeleton = skel,
				};
			}
		}
	}
}

}
