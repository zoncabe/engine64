/*
	One lamp over the middle of the room, plus enough ambient to keep the
	corners out of pitch black. The size is its reach: past 4000 units it
	lights nothing, which covers the room end to end.

	Seven slots, shared between kinds. Declared in order and cut at the first
	empty one, so the six left over cost nothing.
*/
#include "scene3d/e64_lighting.h"


const LightDef light = {

	.ambient_color = { 60, 60, 70, 0xFF },

	.source = {
		{ .type  = LIGHT_POINT,
		  .color = { 255, 245, 220, 0xFF },
		  .point = { .position = {{ 0.0f, 0.0f, 1200.0f }}, .size = 4000.0f } },
	},
};
