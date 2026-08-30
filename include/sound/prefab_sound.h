#ifndef PREFAB_SOUND_H
#define PREFAB_SOUND_H

#include <stdint.h>

#include "sound/sound.h"

struct PhysicsWorld;
struct Scene3DDef;

/* What wakes a sound of a prefab. A new trigger is a new value here, not a
   new struct shape. The splash is not one of these: it belongs to the
   surface, in WaterDef. */
typedef enum {

	PREFAB_SOUND_COLLISION,   /* the body's contact against anything solid */
	PREFAB_SOUND_AMBIENT,

} PrefabSoundTrigger;

/* One sound a prefab carries, tagged by its trigger. A prefab declares an
   array of these and each system walks it looking for its own trigger. */
typedef struct PrefabSound {

	PrefabSoundTrigger trigger;
	const SoundID *sound;      /* one is picked at random when it fires */
	uint8_t count;

} PrefabSound;

/* At scene load: starts the ambient sounds of the placed prefabs, pairing
   each declaration with the entity its placement produced. The emitters
   live in this module; stop cuts them at unload. */
void prefabSound_start(const struct Scene3DDef *def);
void prefabSound_stop(void);

/* Walks the frame's new contacts and plays the collision sound of every
   dynamic body whose prefab declared one, and the splash of every water
   surface something fell into. Call after physics_update. */
void prefabSound_update(struct PhysicsWorld *world);

#endif
