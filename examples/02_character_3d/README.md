# 02 character 3d

A capsule walking around a room: the stick moves it, A jumps, the camera follows it.

Scene, light, fog and camera work exactly as in example 01. What this one adds is a character, the collision it stands on, and the player seat that ties a controller to it.

### Character

A character is declared entirely as settings, and the engine ships no movement values of its own: what this body does is the `Character3DDef` in `main.c` and nothing else. Animation, weapons, aiming, sound and stats are separate blocks of settings, all optional, all left out here.

Its body is kinematic. Nothing in the solver moves it: it moves itself, then resolves whatever it ended up inside and writes its own transform. That whole step is one call, `scene3d_updateCharacters`.

One gait is declared, which is the proportional locomotion case: speed follows stick displacement. Declaring a second gait switches locomotion to target speeds, where the stick selects a gait by crossing its threshold.

The body leaves the floor on the press, and while A stays down the rise pays less gravity, so a tap and a held press reach different heights.

### Collision

Drawn geometry and collision geometry are separate assets built from the same `.glb`. Room geometry is imported twice, once as a model and once as a triangle mesh, and the `Makefile` asks for the second one under `assets_collision`. A character falls through anything with no collision shape.

### Player seat

Binding a controller to a body happens in the state's `bindCharacter`, which the engine calls right after the scene is built. Camera binding names that same player, which is what makes the camera follow the body.

### Controls

| Action | Button |
| --- | --- |
| Walk | Stick |
| Jump | A |
| Turn and tilt the camera | C buttons |
| Pull the camera in and out | L and R |
| Narrow and widen the lens | D-Up and D-Down |

### Building

`ENGINE_DIR` at the top of the `Makefile` points at the engine checkout, two levels up from here. With the Libdragon toolchain and Tiny3D installed:

```
make
```
