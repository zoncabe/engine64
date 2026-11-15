#ifndef ENGINE64_VIEWPORT_H
#define ENGINE64_VIEWPORT_H

#include <libdragon.h>
#include <t3d/t3d.h>
#include "math/e64_vector2.h"

namespace e64 {

class Camera3D;

/* The screen and the 3D view over it. One instance, reached through
   viewport::get: the display is one, so the viewport is one. */
class Viewport {

public:

	static constexpr uint8_t FB_COUNT = 3;

	/* A screen the game can ask for. The engine names no mode of its own:
	   the game declares the ones it uses, the same way it declares its
	   scenes.

	   Only the part that belongs to the game is here. What the player picks
	   — gamma, the video hardware's own smoothing pass, the shape of the
	   picture and how much of it the television swallows — comes from the
	   settings and is applied on top, so changing an option never asks the
	   game to redeclare anything.

	   Height above 240 has to interlace: the console draws half the lines
	   per field, and the picture flickers on anything that moves. Doubling
	   the columns instead costs no flicker, and leaves each pixel half as
	   wide as it is tall, so whatever is drawn under it has to be twice as
	   wide to keep its shape. What it buys is a body that moves in half
	   steps of its own art instead of whole ones. */
	struct ModeDef {

		int32_t width;
		int32_t height;
		interlace_mode_t interlaced;

		bitdepth_t depth; /* zero: 16 bits per pixel */
		uint32_t buffers; /* zero: FB_COUNT */

		/* How many screen pixels one world pixel takes on each axis; zero
		   means one. The 640 column mode splits every column in two, so its
		   art goes at two across and one down to come out the same size. */
		float scale_x;
		float scale_y;

	};

	T3DViewport t3d_viewport;
	int fb_index;

	/* The mode in force, so a settings change can apply itself again over
	   the same screen. */
	ModeDef mode;

	/* Opens no screen and brings up no 3D: those are asked for. */
	void init();

	/* Changes the screen with the game running. The display is torn down and
	   built again, so this belongs at a state change and never inside a frame.
	   Called with the mode already in force, it does nothing. */
	void setMode(const ModeDef *mode);

	/* Applies the player's choices over the screen already set: for whoever
	   closes the settings menu. */
	void refreshMode();

	/* The 3D pipeline, up only while a 3D scene is loaded: a 2D game never pays
	   for it. Opening it sizes the view against the screen in force, so the
	   mode is set first. */
	bool has3D() const;
	void open3D();
	void close3D();

	/* What one world pixel measures on the screen in force, for whoever places
	   or draws in world pixels. */
	Vector2 getScale() const;

	/* The frame: the buffers drawn into, the paint that starts them, and the
	   handover that shows the result. */
	void attach();
	void clear(color_t color);
	void detach();

	/* The projection is the game's call, so it picks one and keeps it fed. The
	   camera is the scene's: the viewport only projects what it is handed. */
	void setPerspective3D(const Camera3D *camera);
	void setIsometric3D(const Camera3D *camera);

private:

	/* A width of zero is a viewport nobody gave a screen yet. */
	bool hasScreen() const;

	/* Opens the screen asked for with the player's choices laid over it. */
	void applyMode();

};


/* The pictures the console actually gives. A state points at one of these, or
   declares a mode of its own when it needs something else.

   320 by 240 is the standard one, and 424 by 240 is that same picture widened
   to 16:9 without costing a line. The 640 column mode keeps the 240 lines and
   splits every column in two, which is why its art goes at two across and one
   down. 640 by 480 is the only one that interlaces, and anything moving in it
   flickers. */
#define SCREEN_320x240 (&e64::SCREEN_320x240_DEF)
#define SCREEN_424x240 (&e64::SCREEN_424x240_DEF)
#define SCREEN_640x240 (&e64::SCREEN_640x240_DEF)
#define SCREEN_640x480 (&e64::SCREEN_640x480_DEF)

/* The objects behind the SCREEN_ macros above: one definition shared by
   every unit that includes this. */
inline const Viewport::ModeDef SCREEN_320x240_DEF = { .width = 320, .height = 240 };
inline const Viewport::ModeDef SCREEN_424x240_DEF = { .width = 424, .height = 240 };
inline const Viewport::ModeDef SCREEN_640x240_DEF = { .width = 640, .height = 240, .scale_x = 2.0f, .scale_y = 1.0f };
inline const Viewport::ModeDef SCREEN_640x480_DEF = { .width = 640, .height = 480, .interlaced = INTERLACE_HALF, .scale_x = 2.0f, .scale_y = 2.0f };


namespace viewport {

Viewport *get(void);

}

}

#endif
