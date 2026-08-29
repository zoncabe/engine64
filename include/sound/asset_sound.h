#ifndef ASSET_SOUND_H
#define ASSET_SOUND_H

#include <stdint.h>

#include "sound/sound.h"

struct PhysicsWorld;
struct Scene3DDef;

/* What wakes a sound of an asset. A new trigger is a new value here, not a
   new struct shape. */
typedef enum {

	ASSET_SOUND_COLLISION,   /* the body's contact against anything solid */
	ASSET_SOUND_WATER_ENTRY, /* the body's first frame inside a water volume */
	ASSET_SOUND_AMBIENT,

} AssetSoundTrigger;

/* One sound an asset carries, tagged by its trigger. An asset declares an
   array of these and each system walks it looking for its own trigger. */
typedef struct AssetSound {

	AssetSoundTrigger trigger;
	const SoundID *sound;      /* one is picked at random when it fires */
	uint8_t count;

} AssetSound;

/* At scene load: starts the ambient sounds of the placed assets, pairing
   each declaration with the entity its placement produced. The emitters
   live in this module; stop cuts them at unload. */
void assetSound_start(const struct Scene3DDef *def);
void assetSound_stop(void);

/* Walks the frame's new contacts and plays the collision sound of every
   dynamic body whose asset declared one. Call after physics_update. */
void assetSound_update(struct PhysicsWorld *world);

#endif
