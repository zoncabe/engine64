
#ifndef ENGINE64_RENDER_H
#define ENGINE64_RENDER_H

#include <stdbool.h>

#include <libdragon.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>
#include "graphics/e64_shapes.h"
#include "graphics/e64_graphic.h"
#include "math/e64_vector3.h"
#include "viewport/e64_viewport.h"

namespace e64 {

class Mesh;

class Render {

public:

	/* A stage contributes one entry per visible tile: a full screen of 16 px
	   cells is 300 of them per layer. */
	static constexpr uint16_t MAX_2D_ELEMENTS = 1024;
	/* One entry per visible model object, not per entity: a prop with four
	   materials is four of them, so this has to clear the scene's object
	   count. */
	static constexpr uint16_t MAX_3D_ELEMENTS = 256;
	static constexpr uint8_t MAX_SECTIONS = 8;
	struct Element2D {

		const Graphic *graphic;

		Vector2 position;
		Vector2 scale;
		float rotation;

	};

	/* One visible model object: its geometry block sits in the object, its
	   material in the mesh module's table. The frame groups elements by
	   material and draws each material once, ahead of every element that
	   uses it. */
	struct Element3D {

		uint8_t material; /* table id, Mesh::MATERIAL_NONE for none */
		const T3DObject *object;
		const Mesh *mesh; /* for the texture scroll */
		T3DMat4FP *matrix;
		T3DSkeleton *skeleton;

	};

	struct Section {

		/* 16 bits: a stage puts hundreds of tiles in one section. */
		uint16_t element_start;
		uint16_t element_count;

		bool has_scissor;
		float scissor_x;
		float scissor_y;
		float scissor_w;
		float scissor_h;

	};

	/* The frame's draw list: filled by the scenes and the interface,
	   consumed by render::draw. */
	struct Context {

		Element2D element2d[MAX_2D_ELEMENTS];
		uint16_t element2d_count;

		Section section[MAX_SECTIONS];
		uint8_t section_count;

		Element3D element3d[MAX_3D_ELEMENTS];
		uint16_t element3d_count;

	};

};


namespace render {

namespace context {

void init(Render::Context *ctx);

}

/* Draws the frame: hands its context to each scene and to the interface to
   fill, then paints it, the interface last. What is not loaded pushes
   nothing, so whatever is up is what shows. Reaches the scenes, the
   interface and the viewport itself. */
void draw(void);

}

}

#endif
