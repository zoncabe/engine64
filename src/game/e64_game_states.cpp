/*
	State machinery only: the table itself is the game's, handed over at
	game::state::start. The engine loads, unloads and switches whatever it is given,
	and plays the frame of whatever it loaded.
*/
#include <assert.h>
#include <libdragon.h>

#include "time/e64_time.h"
#include "scene3d/e64_scene3d.h"
#include "scene2d/e64_scene2d.h"
#include "model/e64_mesh_deform.h"
#include "player/e64_player.h"
#include "viewport/e64_viewport.h"
#include "physics3d/e64_physics.h"
#include "shaders/e64_water.h"
#include "particles/e64_particles.h"
#include "game/e64_game.h"
#include "game/e64_game_states.h"

namespace e64 {

namespace game {

namespace state {

const Game::State::Def *get(Game::State::ID id)
{
	Game::State *s = &game::get()->state;

	assert(id < s->count);
	return &s->table[id];
}

static void load(Game::State::ID id)
{
	const Game::State::Def *def = get(id);

	/* The screen first: the scenes below size their views against it. */
	if (def->viewport) viewport::get()->setMode(def->viewport);

	/* The scenes build their entities and seat nobody: the state's onEnter
	   below seats the players and places the camera, with the scenes up. */
	if (def->scene3d) scene3d::get()->load(def->scene3d);
	if (def->scene2d) scene2d::load(def->scene2d);
	if (def->ui) ui::load(def->ui);

	if (def->onEnter) def->onEnter();
}

static void unload(Game::State::ID id)
{
	const Game::State::Def *def = get(id);

	if (def->onExit) def->onExit();

	/* The seats point into the scenes: emptied before the bodies go. */
	if (def->scene2d || def->scene3d) player::init();
	if (def->ui) ui::unload();
	if (def->scene2d) scene2d::unload();
	if (def->scene3d) scene3d::get()->unload();
}

static bool isOverlay(const Game::State::Def *def)
{
	return def->overlay_count > 0;
}

/* Whether the state declares this base among the ones it overlays. */
static bool isOverlayOf(const Game::State::Def *def, Game::State::ID base)
{
	for (int i = 0; i < def->overlay_count; i++)
		if (def->overlay_of[i] == get(base)) return true;
	return false;
}

/* Only the interface changes hands between an overlay and its base: the
   world underneath stays as it is, and coming back does not re-enter it. */
static void switchInterface(Game::State *s, Game::State::ID new_state)
{
	const Game::State::Def *def = get(new_state);

	s->current = new_state;
	if (def->ui) ui::load(def->ui);
	else ui::unload();
}

static void settle(Game::State *s)
{
	if (s->next == s->current) return;

	const Game::State::Def *leaving = get(s->current);
	if (leaving->canLeave && !leaving->canLeave()) return;

	Game::State::ID prev = s->current;
	Game::State::ID new_state = s->next;

	/* Up onto an overlay from one of its bases: the base is remembered for
	   the way back down. */
	if (isOverlayOf(get(new_state), prev)) {
		s->base = prev;
		switchInterface(s, new_state);
		if (get(new_state)->onEnter) get(new_state)->onEnter();
		return;
	}

	/* Back down to the base it came from. */
	if (isOverlay(leaving) && new_state == s->base) {
		switchInterface(s, new_state);
		return;
	}

	rspq_wait();
	unload(prev);
	/* An abandoned overlay takes its base state down with it. */
	if (isOverlay(leaving)) unload(s->base);
	s->current = new_state;
	load(new_state);

	/* The load blocked for as long as it took: none of it counts as a
	   played frame, or the enter animations would swallow it as one. */
	time::reset();
}


/* Asking to leave is not leaving: the state names where it goes, and the
   switch happens as soon as it lets go. */
void set(Game::State::ID new_state)
{
	Game::State *s = &game::get()->state;

	assert(new_state < s->count);
	s->next = new_state;
}

void start(const Game::State::Def *table, uint8_t count, Game::State::ID initial)
{
	assert(table && initial < count);

	Game::State *s = &game::get()->state;
	s->table = table;
	s->count = count;
	s->current = initial;
	s->next = initial;
	s->base = initial;

	load(initial);
	time::reset();
}

/* The frame of the scenes the state declares: the seats drive their bodies
   on the commands the state's control wrote, the world advances, and the
   camera the state posed is handed to the projection. A state that declares
   none plays nothing here. */
static void step(const Game::State::Def *def)
{
	Viewport *viewport = viewport::get();
	Scene3D *scene = scene3d::get();
	float delta = time::get()->delta;
	uint8_t fb_index = viewport->fb_index;

	if (def->scene3d) {
		player::update();

		physics::update(&scene->physics, delta);
		water::update(delta);
		mesh::deform::update(scene->entity, scene->entity_count, fb_index);

		scene->updateCharacters(fb_index);
		scene->updateEntities(fb_index);

		particles::update(fb_index);

		viewport->setPerspective3D(&scene->camera);
	}

	if (def->scene2d) {
		if (!def->scene3d) player::update();

		scene2d::updateAnimations(delta);
		scene2d::updateCharacters(delta);
		scene2d::updateCamera(scene2d::getCharacter2D(0), delta);
	}
}

/* What the game declared as the state's own runs first, the controller
   read and then the update, then the frame. */
void update(void)
{
	Game::State *s = &game::get()->state;
	const Game::State::Def *def = get(s->current);

	if (def->control) def->control();
	if (def->update) def->update();
	step(def);
	settle(s);
}

}

}

}
