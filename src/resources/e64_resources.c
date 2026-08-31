#include "resources/e64_resources.h"


void resources_load(const ResourceSet *set)
{
	if (!set) return;
	for (int i = 0; i < set->sprite_count; i++)
		sprite_loadAsset(set->sprite[i]);
	for (int i = 0; i < set->font_count; i++)
		font_loadAsset(set->font[i]);
}

void resources_unload(const ResourceSet *set)
{
	if (!set) return;
	for (int i = 0; i < set->sprite_count; i++)
		sprite_unloadAsset(set->sprite[i]);
	for (int i = 0; i < set->font_count; i++)
		font_unloadAsset(set->font[i]);
}
