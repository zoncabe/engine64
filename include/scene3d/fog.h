#ifndef FOG_H
#define FOG_H

#include <stdbool.h>
#include <libdragon.h>

/* Distance fog: computed per vertex on the RSP and blended by the RDP.
   Range is in world units along the view axis, inside the camera planes. */

typedef struct {

	color_t color;
	float near;
	float far;
	bool enabled;

} FogDef;

typedef struct {

	color_t color;
	float near;
	float far;
	bool enabled;

} Fog;


Fog* fog_get(void);

void fog_init(const FogDef* def);
void fog_set(Fog* fog);

#endif
