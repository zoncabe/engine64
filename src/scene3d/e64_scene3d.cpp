#include <assert.h>
#include <malloc.h>

#include <t3d/t3dmath.h>

#include "viewport/e64_viewport.h"
#include "scene3d/e64_light.h"
#include "scene3d/e64_fog.h"
#include "scene3d/e64_scene3d.h"
#include "physics3d/e64_physics.h"

namespace e64 {

static Scene3D scene;

namespace scene3d {

Scene3D *get(void) { return &scene; }

}


void Scene3D::load(const Def *def)
{
	static const fog::Def no_fog = {};

	/* The 3D pipeline comes up with the scene that needs it and goes down
	   with it, so a game that never loads one never pays for it. */
	viewport::get()->open3D();

	/* No light is open yet: the entities bring theirs. Until then the scene
	   is lit by its ambient colour alone, and black if the def gave none. */
	light::init(def->ambient_color);
	fog::init(def->fog ? def->fog : &no_fog);

	/* The camera goes back to its defaults with the rest: the game places it
	   from its onEnter, once the scene is up. */
	assert(entity_count == 0);
	*this = (Scene3D){};

	Vector3 gravity = { 0.0f, 0.0f, -9.8f };
	physics::world::init(&physics, physics::TIMESTEP, gravity, physics::SOLVER_ITERATIONS);
	physics::world::setWind(&physics, def->wind);
}

void Scene3D::clear(void)
{
	*this = (Scene3D){};
}

void Scene3D::unload(void)
{
	/* The game takes down what it built first, with the entities and the
	   physics world still up. */
	for (int i = 0; i < entity_count; i++)
		if (entity[i]->destroy) entity[i]->destroy(entity[i]);

	for (int i = 0; i < entity_count; i++)
		entity3d::destroy(entity[i]);
	free(entity);
	clear();
	physics::world::shutdown(&physics);

	viewport::get()->close3D();
}

/* Nothing is placed here: each model and each light is already where its
   def says when entity3d::create returns, and the body goes where its own
   def says. */
Entity3D *Scene3D::createEntity(const Prefab3D *prefab)
{
	Entity3D *e = entity3d::create(prefab);

	/* The body first: a sound plays from where the body is. */
	e->attachPhysics(&physics, prefab->body);
	e->startLoopSounds();

	addEntity(e);

	/* The game's part of the entity, over one that is whole and already in
	   the scene's list. */
	if (e->create) e->create(e);

	return e;
}

/* The game's half of the frame, entity by entity, in the scene's order. */
void Scene3D::updateCallbacks(void)
{
	for (int i = 0; i < entity_count; i++)
		if (entity[i]->update) entity[i]->update(entity[i]);
}

/* What the solver moved is somewhere new by the time the frame gets here,
   and the matrix it is drawn with has to say so. Anything still was placed
   when it was created and this passes over it. The list is the scene's,
   which is why it lives here. */
void Scene3D::updateEntities(uint8_t fb_index)
{
	for (int i = 0; i < entity_count; i++)
		entity[i]->setMatrixFromBody(fb_index);
}

/* The array grows by one pointer per entity. */
void Scene3D::addEntity(Entity3D *e)
{
	entity = (Entity3D **)realloc(entity, (entity_count + 1) * sizeof(Entity3D *));
	assert(entity);
	entity[entity_count++] = e;
}

void Scene3D::setRenderContext(Render::Context *ctx, const Viewport *viewport) const
{
	uint8_t fb_index = viewport->fb_index;

	for (int i = 0; i < entity_count; i++) {
		const Entity3D *e = entity[i];

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
			   object draws with the model's. */
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
