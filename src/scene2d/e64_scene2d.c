/*
	The 2D scene, built from its definition the way the 3D one is: the def
	declares layers and elements, the load copies them into the single live
	scene, and from then on whoever animates writes there.
*/
#include <assert.h>
#include <string.h>

#include "scene2d/e64_scene2d.h"


static Scene2D scene2d;


Scene2D *scene2d_get(void) { return &scene2d; }

void scene2d_load(const Scene2DDef *def)
{
	assert(def && def->layer_count <= SCENE2D_MAX_LAYER);

	scene2d = (Scene2D){ .def = def };

	for (int i = 0; i < def->layer_count; i++) {
		const Scene2DLayer *layer = &def->layer[i];

		assert(scene2d.element_count + layer->element_count <= SCENE2D_MAX_ELEMENT);

		scene2d.layer_start[i] = scene2d.element_count;
		memcpy(scene2d.element + scene2d.element_count,
		       layer->element,
		       layer->element_count * sizeof(Element2D));
		scene2d.element_count += layer->element_count;
	}
}

void scene2d_unload(void)
{
	scene2d = (Scene2D){0};
}

Element2D *scene2d_getElement(Scene2D *scene, uint8_t layer, uint8_t element)
{
	assert(scene->def && layer < scene->def->layer_count);
	assert(element < scene->def->layer[layer].element_count);

	return &scene->element[scene->layer_start[layer] + element];
}
