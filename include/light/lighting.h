#ifndef LIGHT_H
#define LIGHT_H

#define DIRECTIONAL_LIGHT_COUNT 1
#define POINT_LIGHT_COUNT 0

typedef struct{

	color_t color;
	T3DVec3 direction;
	T3DVec3 size;
	
} DirectionalLight;

typedef struct {

	T3DVec3 position;
	color_t color;
	float  size;

} PointLight;

typedef struct{

	color_t ambient_color;
	DirectionalLight directional[DIRECTIONAL_LIGHT_COUNT];
	PointLight point[POINT_LIGHT_COUNT];

} Light;

Light* light_get(void);

void light_initDirectional(DirectionalLight* light, T3DVec3 direction, color_t color);
void light_initAmbient(color_t color);
void light_initPoint(PointLight* light, T3DVec3 position, color_t color, float strength);
void light_updatePoint(PointLight* light, T3DVec3 position, color_t color, float strength);
void light_setDirectional(DirectionalLight* light);
void light_setPoint(PointLight* light);
void light_set(Light* light);

#endif
