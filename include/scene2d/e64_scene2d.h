#ifndef ENGINE64_SCENE2D_H
#define ENGINE64_SCENE2D_H

#include "render/e64_render.h"
#include "entity/e64_entity2d.h"
#include "prefab/e64_prefab2d.h"
#include "character2d/e64_character2d.h"
#include "camera/e64_camera2d.h"
#include "stage2d/e64_stage2d.h"

namespace e64 {

namespace scene2d {

constexpr int MAX_LAYER = 8;
constexpr int MAX_ENTITY = 64;
constexpr int MAX_CHARACTER = 4;
constexpr int MAX_STAGE = 4;
constexpr int MAX_FONT = 8;


/* An entity of a layer as declared: which prefab and where. The load builds
   the live entity of the same index. A zero scale means identity. */
typedef struct Entity {

	const Prefab2D *prefab;
	Vector2 position;
	Vector2 scale;
	float rotation;

} Entity;

/* A layer groups what it draws under one order and one scissor, the
   CanvasLayer of Godot. */
typedef struct Layer {

	const Entity *entity;
	uint8_t entity_count;

	bool has_scissor;
	float scissor_x;
	float scissor_y;
	float scissor_w;
	float scissor_h;

} Layer;

typedef struct Def {

	const camera2d::Def *camera;

	const Layer *layer;
	uint8_t layer_count;

	/* What the frame is cleared to under this scene: the sky behind the
	   tiles. Left at zero alpha the render keeps its own clear. */
	color_t background;

} Def;

}


/* The live scene: the entities flat, in placement order, with where each
   layer's begin. */
typedef struct Scene2D {

	const scene2d::Def *def;

	Camera2D camera;

	Entity2D *entity[scene2d::MAX_ENTITY];
	uint8_t entity_count;

	Character2D *character[scene2d::MAX_CHARACTER];
	uint8_t character2d_count;

	Stage2D *stage[scene2d::MAX_STAGE];
	uint8_t stage_count;

	/* The font ids its texts name, each loaded once for the scene. */
	uint8_t font[scene2d::MAX_FONT];
	uint8_t font_count;

	uint8_t layer_start[scene2d::MAX_LAYER];

} Scene2D;


namespace scene2d {

Scene2D *get(void);

/* Builds the layers' entities in placement order. Nobody is seated: the game
   seats its players once the scene is up, from its onEnter. */
void load(const Def *def);
void unload(void);

/* Advances every animated sprite at the rate its owner set and hands the
   entity the frame it lands on. Runs before the characters, which pick the
   clip and the rate the next advance uses. */
void updateAnimations(float dt);

/* Moves every character, picks its clip and rate, and carries the frame to
   what draws it. */
void updateCharacters(float dt);

/* Moves the camera to where the character it follows stands. */
void updateCamera(const Character2D *character, float dt);

/* A layer's entity by its placement index in the definition. */
Entity2D *getEntity(Scene2D *scene, uint8_t layer, uint8_t index);

Character2D *getCharacter2D(uint8_t index);

Stage2D *getStage(uint8_t index);

/* Pushes one section per layer into the frame's context, with the layer's
   scissor and one Render::Element2D per entity in it. A stage entity
   contributes its own elements instead, those the camera can see. */
void setRenderContext(const Scene2D *scene, Render::Context *ctx);

}

}

#endif
