/*
	Distance haze. Every material in this example has fog turned on, so the
	scene only says from where and until where: nothing fades before 1500
	units and everything is background colour past 4500.
*/
#include "scene3d/e64_fog.h"


const FogDef fog = {

	.color   = { 70, 80, 100, 0xFF },
	.near    = 1500.0f,
	.far     = 4500.0f,
	.enabled = true,
};
