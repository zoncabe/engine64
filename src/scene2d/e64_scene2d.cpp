/*
	The 2D scene, built from its definition the way the 3D one is: the def
	declares layers of placed prefabs, the load turns each placement into an
	entity of the single live scene, and from then on whoever animates
	writes there.
*/
#include <assert.h>
#include <malloc.h>

#include "scene2d/e64_scene2d.h"
#include "viewport/e64_viewport.h"

namespace e64 {
namespace scene2d {

static Scene2D scene;


Scene2D *get(void) { return &scene; }

/* A font is shared by every text that names it: loaded the first time the
   scene meets the id, freed once at unload. */
static void loadFont(uint8_t id)
{
	for (int i = 0; i < scene.font_count; i++)
		if (scene.font[i] == id) return;

	assert(scene.font_count < MAX_FONT);
	font::loadAsset(id);
	scene.font[scene.font_count++] = id;
}

void load(const Def *def)
{
	assert(def && def->layer_count <= MAX_LAYER);

	/* A scene may be loaded over another (an overlay taking the screen):
	   what the one leaving created goes first. */
	unload();

	scene.def = def;

	/* A scene with nothing but widgets declares no camera: the zeroed one is
	   of no type and moves nothing. */
	if (def->camera) camera2d::init(&scene.camera, def->camera);

	for (int i = 0; i < def->layer_count; i++) {
		const Layer *layer = &def->layer[i];

		scene.layer_start[i] = scene.entity_count;

		for (int p = 0; p < layer->entity_count; p++) {
			const Entity *placed = &layer->entity[p];
			const Prefab2D *prefab = placed->prefab;

			if (prefab->graphic.type == Graphic::TEXT)
				loadFont(prefab->graphic.text.font);

			/* Filled from the prefab and its placement, the way the 3D load
			   fills its entity3d::Def, and gone after the load. */
			entity2d::Def entity_def = {
				.position = placed->position,
				.scale = placed->scale,
				.rotation = placed->rotation,
				.graphic = &prefab->graphic,
				.sound = prefab->sound,
				.sound_count = prefab->sound_count,
			};

			assert(scene.entity_count < MAX_ENTITY);
			Entity2D *entity = entity2d::create(&entity_def);
			scene.entity[scene.entity_count++] = entity;

			if (prefab->type == prefab2d::PREFAB2D_CHARACTER) {
				assert(scene.character2d_count < MAX_CHARACTER);
				Character2D *character = character2d::create(prefab->character, entity);
				scene.character[scene.character2d_count++] = character;
			}

			/* The stage draws through its own elements; the entity only
			   says where it stands. */
			if (prefab->type == prefab2d::PREFAB2D_STAGE) {
				assert(scene.stage_count < MAX_STAGE);
				entity->graphic->is_hidden = true;
				scene.stage[scene.stage_count++] = stage2d::create(prefab->stage, entity);
			}
		}
	}

	/* The bodies collide with the last stage placed: the level, drawn over
	   whatever backdrops came before it. */
	if (scene.stage_count)
		for (int i = 0; i < scene.character2d_count; i++)
			scene.character[i]->stage = scene.stage[scene.stage_count - 1];
}

void unload(void)
{
	for (int i = 0; i < scene.character2d_count; i++)
		character2d::destroy(scene.character[i]);

	for (int i = 0; i < scene.stage_count; i++)
		stage2d::destroy(scene.stage[i]);

	for (int i = 0; i < scene.entity_count; i++)
		entity2d::destroy(scene.entity[i]);

	for (int i = 0; i < scene.font_count; i++)
		font::unloadAsset(scene.font[i]);

	scene = (Scene2D){};
}

void updateAnimations(float dt)
{
	for (int i = 0; i < scene.entity_count; i++) {
		Graphic *graphic = scene.entity[i]->graphic;
		if (graphic->type != Graphic::SPRITE || !graphic->sprite.animation) continue;

		sprite::animation::advance(graphic->sprite.animation, dt);
		graphic->sprite.asset = sprite::animation::getSprite(graphic->sprite.animation);
	}
}

void updateCharacters(float dt)
{
	for (int i = 0; i < scene.character2d_count; i++)
		character2d::update(scene.character[i], dt);
}

void updateCamera(const Character2D *character, float dt)
{
	if (!character) return;

	camera2d::update(&scene.camera, character->position,
	                character->facing_left ? -1.0f : 1.0f, dt);
}

Entity2D *getEntity(Scene2D *s, uint8_t layer, uint8_t index)
{
	assert(s->def && layer < s->def->layer_count);
	assert(index < s->def->layer[layer].entity_count);

	return s->entity[s->layer_start[layer] + index];
}

Character2D *getCharacter2D(uint8_t index)
{
	if (index >= scene.character2d_count) return NULL;
	return scene.character[index];
}

Stage2D *getStage(uint8_t index)
{
	if (index >= scene.stage_count) return NULL;
	return scene.stage[index];
}

void setRenderContext(const Scene2D *s, Render::Context *ctx)
{
	if (!s->def) return;

	/* The stages first, one section for all of them: the world the layers
	   of prefabs are drawn over. */
	if (s->stage_count) {
		assert(ctx->section_count < Render::MAX_SECTIONS);
		Render::Section *section = &ctx->section[ctx->section_count++];
		*section = (Render::Section){ .element_start = ctx->element2d_count };

		for (int i = 0; i < s->stage_count; i++)
			stage2d::setRenderContext(s->stage[i], &s->camera, ctx);

		section->element_count = ctx->element2d_count - section->element_start;
	}

	for (int i = 0; i < s->def->layer_count; i++) {
		const Layer *layer = &s->def->layer[i];

		assert(ctx->section_count < Render::MAX_SECTIONS);
		Render::Section *section = &ctx->section[ctx->section_count++];
		section->element_start = ctx->element2d_count;

		for (int p = 0; p < layer->entity_count; p++) {
			const Entity2D *entity = s->entity[s->layer_start[i] + p];

			/* A stage's entity only says where the stage stands; the stage
			   itself went above. */
			if (layer->entity[p].prefab->type == prefab2d::PREFAB2D_STAGE) continue;

			/* Where the entity stands is world; what the camera takes of it
			   is the prefab's parallax, and a widget's zero leaves the
			   position it was placed at untouched. */
			float parallax = layer->entity[p].prefab->parallax;

			/* Under a camera the zoom scales what is drawn as well as where
			   it lands; a scene with no camera draws at the placed size. On
			   top of it goes what a world pixel measures on this screen. */
			float zoom = s->camera.type == camera2d::CAMERA2D_TYPE_NONE ? 1.0f : s->camera.zoom;
			Vector2 scale = viewport::get()->getScale();

			/* Whole screen pixels, like the stage's tiles, so a sprite
			   standing on one never lands between texels. Rounded here and
			   nowhere else: the world position keeps its fractions, and a
			   second rounding before the camera would hold the body still
			   for a frame and move it two the next. */
			Vector2 screen = camera2d::toScreen(&s->camera, entity->position, parallax);

			assert(ctx->element2d_count < Render::MAX_2D_ELEMENTS);
			ctx->element2d[ctx->element2d_count++] = (Render::Element2D){
				.graphic = entity->graphic,
				.position = { floorf(screen.x), floorf(screen.y) },
				.scale = { entity->scale.x * zoom * scale.x, entity->scale.y * zoom * scale.y },
				.rotation = entity->rotation,
			};
		}

		section->element_count = ctx->element2d_count - section->element_start;
		section->has_scissor = layer->has_scissor;
		section->scissor_x = layer->scissor_x;
		section->scissor_y = layer->scissor_y;
		section->scissor_w = layer->scissor_w;
		section->scissor_h = layer->scissor_h;
	}
}

}
}
