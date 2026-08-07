# Engine 64

Framework type game engine written in C on top of the open source SDK [Libdragon](https://github.com/DragonMinded/libdragon) and the 3D ucode & library [Tiny3D](https://github.com/HailToDodongo/tiny3d).

### Features

**Physics engine**
A port of [qu3e](https://github.com/RandyGaul/qu3e) by Randy Gaul, expanded to use all basic primitives plus triangle meshes. Rigid bodies with island solving and sleeping, sequential impulse contact solver with warm starting, friction and restitution.<br/>
Cloth simulation implemented using the [Advanced Character Physics](https://www.cs.cmu.edu/afs/cs/academic/class/15462-s13/www/lec_slides/Jakobsen.pdf) papers, a Verlet method developed by Thomas Jakobsen at IO Interactive for the Hitman games.

**Character physics**
Collide and slide based character collision, resolved against boxes, spheres, capsules and triangle meshes. Floor detection and snapping.

**Character movement**
Gait based movement system: each gait defines its target speed, acceleration and rotation response. Movement accelerates towards a target velocity, with per asset settings for jump force, air control and roll timings.

**Character animation**
Blend tree based animation system: poses are sampled from clips and combined by weighted bone blending. Supports clip playback, selection, sequencing, 1D and 2D blend spaces and layering.

**Screen system**
2D screen system with a track based animation engine. Handles rectangles, sprites and text grouped in scissor sections, with easing curves, timed visibility windows and state driven lookups.

**Scenes**
Scene definitions hold the level content: entities with model, transform and collider shapes, characters, and the physics world.

**Camera**
Camera system with a spherical third person camera implemented, and the structure in place to add new camera types.

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
