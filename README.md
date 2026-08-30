# Engine 64

Framework type game engine. Written in C on top of the open source SDK [Libdragon](https://github.com/DragonMinded/libdragon) and the 3D ucode & library [Tiny3D](https://github.com/HailToDodongo/tiny3d).

### Features

**Physics engine**<br/>
A port of [qu3e](https://github.com/RandyGaul/qu3e) by Randy Gaul, expanded to use all basic primitives plus triangle meshes. Rigid bodies with island solving and sleeping, sequential impulse contact solver with warm starting, friction and restitution.<br/>

**Cloth**<br/>
Cloth simulation implemented using the [Advanced Character Physics](https://www.cs.cmu.edu/afs/cs/academic/class/15462-s13/www/lec_slides/Jakobsen.pdf) papers, a Verlet method developed by Thomas Jakobsen at IO Interactive for the Hitman games.

**Water**<br/>
Procedural water surfaces: sum of directional sine waves over a subdivided plane through the mesh deform path, analytic normals, height shaded vertex colors and dual scrolling texture layers. Analytic surface height query at any point, shared with the physics.<br/>
Buoyancy: water registered as sensor volumes in the physics world, overlap tracked by the broadphase and skipped by the solver. Archimedes force on dynamic bodies, per shape column sampling of the submerged volume against the wave surface, closed form spherical cap, force applied at the submerged centroid. Linear and angular drag scaled by submersion, floating depth set by density.

**Character physics**<br/>
A port of the character collision from Godot Engine, the recovery step of [GodotSpace3D::test_body_motion](https://github.com/godotengine/godot/blob/master/modules/godot_physics_3d/godot_space_3d.cpp) and the contact classification of [CharacterBody3D::_set_collision_direction](https://github.com/godotengine/godot/blob/master/scene/3d/physics/character_body_3d.cpp): kinematic capsule against boxes, spheres, capsules and triangle meshes, moved the full step and recovered out of penetration, with floor detection and snapping. Contact normals on internal mesh edges corrected with a port of [ActiveEdges::FixNormal](https://github.com/jrouwe/JoltPhysics/blob/master/Jolt/Physics/Collision/ActiveEdges.h) from Jolt Physics. Registered in the physics world for two way interaction with rigid bodies.<br/>
Fake buoyancy; equilibrium depth spring, submersion scaled drag and sink limit.

**Character movement**<br/>
Gait based movement system: target speed, acceleration and rotation response per gait, with exponential velocity convergence. Per asset settings for charged jump, air control and roll phases.

**Character animation**<br/>
Blend tree based animation system: clip playback, selection, sequencing, 1D and 2D blend spaces, and weighted layering over skeleton buffers.

**Spring bones**<br/>
A port of [SpringBoneSimulator3D](https://github.com/godotengine/godot/blob/master/scene/3d/spring_bone_simulator_3d.cpp) from Godot Engine as a skeleton modifier: one Verlet tail per joint, chains resolved root to tip, simulated in character space with per set stiffness, drag and gravity. Sphere, capsule and plane colliders hung from bones, with per joint radii.<br/>
World motion damping and teleport thresholds from [AnimNode_KawaiiPhysicsSimulation](https://github.com/pafuhana1213/KawaiiPhysics/blob/master/Plugins/KawaiiPhysics/Source/KawaiiPhysics/Private/AnimNode_KawaiiPhysicsSimulation.cpp), part of Kawaii Physics by pafuhana1213.

**3D scenes**<br/>
Data driven scene definitions: lighting, fog, camera setup, wind, and sound emitters. Loading resolves a definition into the physics world, character instances, cloth bindings and active emitters.

**2D scenes**<br/>
Rectangles, sprites and text grouped in layers, each under its own scissor. Definition and instance as on the 3D side: loading resolves a definition into the live scene drawn over the world.

**UI**<br/>
Track based animation engine over the loaded 2D scene: easing curves, staggered delays, timed visibility windows and state driven lookups, with tracks addressing an element field by index instead of by pointer and reverse playback mirroring the curves.

**Sound**<br/>
Positional sound system on top of the Libdragon mixer. Per sample volume, min and max distance with inverse falloff, priority based channel allocation and out of range culling. Constant power panning from listener position and camera orientation, damped at close range. Distance and panning applied on independent mixer paths with RSP driven ramps. Playback rate scaling to fit a sample to a requested duration.<br/>
Character sounds driven by animation and movement state: footsteps at configurable points of the running locomotion clip, roll, jump and landing at their movement phases, volume scaled by speed.

**Camera**<br/>
Spring arm third person camera: exponential convergence on yaw and pitch with velocity clamping, per state arm length, shoulder offset and field of view, and view target blending on target changes. Camera types resolve through a handler table.

### Building

1. Install the Libdragon toolchain following their [installation guide](https://github.com/DragonMinded/libdragon/wiki/Installing-libdragon).
2. Build and install [Tiny3D](https://github.com/HailToDodongo/tiny3d/blob/main/README.md#usage).
3. From the project root run:

```
./build.sh
```

That rebuilds every example. To build a single one, run `make` inside its folder.

Download the latest build on the [itch.io](https://zoncabe.itch.io/engine64) page.<br/>

Hit the [N64brew Discord](https://discord.gg/r86zSRwDDY) for more.<br/>
