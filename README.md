# Engine64

N64 game engine written in C on top of the open source SDK [Libdragon](https://github.com/DragonMinded/libdragon) and the 3D ucode & library [Tiny3D](https://github.com/HailToDodongo/tiny3d).

It works as a framework: you build your game calling the engine functions directly, declaring scenes, entities and screens in code, everything statically compiled. Asset pipelines with custom binary formats are on the todo list, for now everything is declared in source.

### Features

**Physics engine**
Rigid bodies with island solving and sleeping. Sequential impulse contact solver with warm starting, friction and restitution. Box, sphere, capsule and triangle mesh colliders with a broadphase in front, plus cloth simulation running on the same fixed step.

**Character physics**
Characters don't go through the rigid body solver, they have their own collision path: capsule against the level mesh and primitive colliders, with floor detection for grounding. Each character carries a kinematic body so the solver props still react when pushed.

**Character animation**
Skeletal state machine on top of Tiny3D skeletons. Locomotion is a parametric grid of gaits and turning blended by weights, with aiming and strafing layered on top, and basic weapon handling.

**Screen system**
2D screens (menus, HUD, cutscenes) are declared as structs of draw elements: rectangles, sprites and text, grouped in sections with optional scissor. A generic track based animation engine mutates those elements over time: lerps with easing, boolean visibility windows and lookup tracks driven by game state. The engine knows nothing about any particular screen.

**Scenes and game states**
A scene definition lists entities, their models and their collider shapes. The game state machine loads scenes and resources (sprites, fonts) per state, with enter and exit transitions handled by the screen system.

**Camera and control**
Spherical third person camera with offset, with the groundwork in place for scripted cameras. Joypad bindings mapped to game actions.

### Building

1. Install the Libdragon toolchain following their [installation guide](https://github.com/DragonMinded/libdragon/wiki/Installing-libdragon). The `N64_INST` environment variable must point to the toolchain install.
2. Build and install [Tiny3D](https://github.com/HailToDodongo/tiny3d/blob/main/README.md) against that toolchain, with `T3D_INST` pointing to its location.
3. From the project root run:

```
make
```

The build converts the assets (models, textures, fonts, collision meshes) and produces `game.z64`, ready to run on an emulator or flashcart. `make clean` removes the build directory and the ROM.

download the latest build on the [itch.io](https://zoncabe.itch.io/engine64) page<br/>

hit the [N64brew Discord](https://discord.gg/r86zSRwDDY) for more<br/>
