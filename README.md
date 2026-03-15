
# BuGL

> Build your own engine in script.  
> A C++-oriented runtime for programmers who want to build systems, not just use them.

BuGL is not a closed game engine.

BuGL is a **programmable runtime** built around **BuLang**, a simple but powerful scripting language designed to stay close to C++ while enabling fast iteration.

The goal is not to hide systems behind a monolithic editor.  
The goal is to give programmers a space to experiment with **rendering**, **physics**, **AI/navigation**, **tooling**, **gameplay systems**, and **engine architecture** directly from code.

With the current release, you can download BuGL, open the included `.bu` scripts, edit them, and run them immediately.

---

## Why BuGL Exists

BuGL was created out of frustration with traditional scripting approaches.

I wanted a language that felt much closer to **C++** than the usual embedded scripting solutions, while still being simple enough for rapid iteration.

With BuGL:

- native **C++ systems** can be exposed directly to script
- script classes can **inherit from C++ classes**
- native C++ code can call back into **script-defined behavior**

That makes BuGL less like a traditional engine and more like a programmable environment for building rendering systems, gameplay logic, tools and engine workflows from code.

BuGL is for **curious programmers** who enjoy writing code, understanding systems, and building things from the ground up.

---

## Download and Run

The easiest way to start is with the latest release.

The release package already includes:

- `bugl` / `bugl.exe`
- tutorial and demo scripts
- required runtime and plugin files

Extract the release and run a script directly.

### Linux

```bash
./bugl scripts/tutor_1.bu
````

### Windows

```bash
bugl.exe scripts/tutor_1.bu
```

Then try:

1. `scripts/tutor_3.bu`
2. `scripts/tutor_6.bu`
3. `scripts/demo_box2d_stack_chaos.bu`
4. `scripts/demo_jolt_tank_csm_particles.bu`

The intended workflow is simple:

* download the release
* open a `.bu` script
* change it
* run it again

---

## What BuGL Is

* **BuLang** = the language
* **BuGL** = the runtime and native systems exposed to script
* **Plugins** = larger systems loaded on demand

Typical script flow:

```text
Init -> while (Running) -> update -> draw -> Flip
```

BuGL lets you prototype rendering and gameplay logic quickly, without writing C++ boilerplate for every experiment.

---

## Featured Demos

These demos are a good starting point to understand what BuGL can do.

### Box2D Stack Chaos

Interactive 2D physics sandbox with spawning, impulses, debug draw and GIF capture.

* left click: spawn box
* right click: spawn ball
* space: blast impulse
* `R`: reset

Script:

* `scripts/demo_box2d_stack_chaos.bu`

### Jolt Tank + Cascaded Shadows + Sparks

A more advanced 3D showcase combining:

* tracked vehicle physics
* turret and barrel control
* shell firing and explosion force
* cascaded shadow maps
* procedural normal mapping
* batch-based spark effects
* shader reload and screenshot capture

Script:

* `scripts/demo_jolt_tank_csm_particles.bu`

### Other demo groups

* **Rendering**: bloom, HDR, ray tracing, ray marching, terrain, portals
* **Physics**: Jolt, ODE, Box2D, Chipmunk
* **AI / Navigation**: Recast, Detour, OpenSteer, MicroPather
* **Tooling / UI**: ImGui, fonts, Batch drawing, widgets

---

## Plugin-Based Runtime

BuGL uses plugins for larger feature groups.

At runtime, scripts load what they need:

```bu
require "Jolt";
import Jolt;
using Jolt;
```

Main plugin modules:

| Plugin          | Module in script | Typical usage                          |
| --------------- | ---------------- | -------------------------------------- |
| `BuJolt`        | `Jolt`           | 3D vehicles, rigid bodies, constraints |
| `BuOde`         | `ODE`            | 3D rigid body simulation               |
| `BuBox2d`       | `Box2D`          | 2D physics                             |
| `BuChipmunk`    | `Chipmunk`       | Alternative 2D physics                 |
| `BuRecast`      | `Recast`         | Navmesh + pathfinding + crowd          |
| `BuOpenSteer`   | `OpenSteer`      | Steering behaviors                     |
| `BuMicroPather` | `MicroPather`    | Grid/pathfinding                       |
| `BuAssimp`      | `Assimp`         | 3D model import                        |
| `BuImgui`       | `ImGui`          | Tooling and UI                         |

Plugin binaries live in:

```text
bin/plugins/
```

---

## AI and Navigation Stack

BuGL is especially strong as a playground for real-time agent behavior and movement systems.

### Recast / Detour

Used for:

* navmesh generation
* path queries
* crowd movement
* large-scene navigation

### OpenSteer

Used for:

* flocking
* pursuit / evasion
* avoidance
* steering-based local motion

### MicroPather

Used for:

* fast grid-based pathfinding
* RTS / tactics style routing
* deterministic fallback logic

A useful combined workflow is:

* **Detour** for high-level path
* **OpenSteer** for local movement
* **MicroPather** for deterministic grid logic

Relevant demos:

* `demo_recast_*`
* `demo_opensteer_*`
* `demo_steertest.bu`
* `demo_micropather.bu`

---

## Learning Path

A suggested learning order:

### Tutorials

1. `scripts/tutor_1.bu` — window, loop and basic drawing
2. `scripts/tutor_2.bu` — arrays and buffers
3. `scripts/tutor_3.bu` — shader and VBO flow
4. `scripts/tutor_4.bu` — camera and matrix usage
5. `scripts/tutor_5.bu` — FPS controls and capture workflow
6. `scripts/tutor_6.bu` — text and font flow

### Then move to demos by topic

**Rendering**

* `demo_phong.bu`
* `demo_texture_quad.bu`
* `demo_bloom_hdr.bu`
* `demo_shadowmap_csm_v3.bu`
* `demo_raymarching.bu`
* `demo_raytrace.bu`

**Physics**

* `demo_jolt_*`
* `demo_ode_*`
* `demo_box2d_*`
* `demo_chipmunk_*`

**AI / Navigation**

* `demo_opensteer_*`
* `demo_steertest.bu`
* `demo_recast_*`
* `demo_micropather.bu`

**UI / Tools**

* `demo_imgui.bu`
* `test_imgui_widgets_smoke.bu`
* `test_font.bu`

---

## Build from Source

If you prefer to build from source:

```bash
git clone https://github.com/akadjoker/BuGL
cd BuGL
cmake -S . -B build
cmake --build build -j
./bin/bugl scripts/tutor_1.bu
```

Run scripts:

```bash
./bin/bugl scripts/tutor_1.bu
./bin/bugl scripts/main.bu
```

---

## Common Commands

```bash
# run the first tutorial
./bin/bugl scripts/tutor_1.bu

# run a larger release-style showcase
./bin/bugl scripts/demo_jolt_tank_csm_particles.bu

# run the Box2D chaos demo
./bin/bugl scripts/demo_box2d_stack_chaos.bu

# run ImGui smoke test
./bin/bugl scripts/test_imgui_widgets_smoke.bu
```

On Windows:

```bash
bugl.exe scripts/tutor_1.bu
bugl.exe scripts/demo_jolt_tank_csm_particles.bu
```

---

## Gallery

|             Ray Tracer             |           Bloom + HDR           |            Particles            |
| :--------------------------------: | :-----------------------------: | :-----------------------------: |
|   ![ray tracer](gif/raytrace.gif)  | ![bloom hdr](gif/bloom_hdr.gif) | ![particles](gif/particles.gif) |
| Reflections · refraction · Fresnel |      Multi-pass HDR + bloom     |  High-count particle rendering  |

|             Ray Marching            |           Terrain           |                      Box2D                      |
| :---------------------------------: | :-------------------------: | :---------------------------------------------: |
| ![raymarching](gif/raymarching.gif) | ![terrain](gif/terrain.gif) | ![box2d mouse joint](gif/box2d_mouse_joint.gif) |
|            SDF + lighting           |      Procedural terrain     |              2D physics interaction             |

|           ODE Car           |             ODE Fall 3D            |             Box2D Stack             |
| :-------------------------: | :--------------------------------: | :---------------------------------: |
| ![ode car](gif/ode_car.gif) | ![ode fall 3d](gif/ode_fall3d.gif) | ![box2d stack](gif/box2d_stack.gif) |
|         Car physics         |      3D rigid body collisions      |           Stack stability           |

|              Jolt Vehicle             |               Jolt Motorcycle               |            Jolt Tank            |
| :-----------------------------------: | :-----------------------------------------: | :-----------------------------: |
| ![jolt vehicle](gif/jolt_vehicle.gif) | ![jolt motorcycle](gif/jolt_motorcycle.gif) | ![jolt tank](gif/jolt_tank.gif) |
|       Wheeled vehicle controller      |               Bike controller               |      Tracked tank + turret      |

|               Portal Showcase               |               ImGui Components               |             Batch Lines             |
| :-----------------------------------------: | :------------------------------------------: | :---------------------------------: |
| ![showcase portal](gif/showcase_portal.gif) | ![imgui components](gif/imgui_componnts.gif) | ![batch lines](gif/batch_lines.gif) |
|           Render-to-texture portal          |            ImGui widgets/bindings            |         2D Batch primitives         |

|                 Batch 3D Primitives                 |          Tank CSM + Sparks          |              Tutor 5 Capture              |
| :-------------------------------------------------: | :---------------------------------: | :---------------------------------------: |
| ![batch 3d primitives](gif/batch_3d_primitives.gif) | ![tank jolt csm](gif/tank_jolt.gif) | ![tutor5 capture](gif/capture_tutor5.gif) |
|               3D primitives with Batch              | Jolt + cascaded shadows + particles |            FPS camera tutorial            |

|               OpenSteer Boids               |               OpenSteer Path              |          Steering Test          |
| :-----------------------------------------: | :---------------------------------------: | :-----------------------------: |
| ![opensteer boids](gif/opensteer_boids.gif) | ![opensteer path](gif/opensteer_path.gif) | ![steertest](gif/steertest.gif) |
|              Flocking behaviors             |               Path following              |         Steering sandbox        |

|            MicroPather Pathfinding           |        OpenSteer Showcase       |       Assimp Import       |
| :------------------------------------------: | :-----------------------------: | :-----------------------: |
| ![micropather pathfinding](gif/pathfind.gif) | ![opensteer](gif/opensteer.gif) | ![assimp](gif/assimp.gif) |
|               Grid pathfinding               |        Steering behaviors       |   Model import pipeline   |

---

## Current Public Release

Current public release highlights:

* plugin-based runtime architecture
* 3D physics with `Jolt` and `ODE`
* 2D physics with `Chipmunk` and `Box2D`
* navigation and agents with `Recast/Detour`, `OpenSteer`, and `MicroPather`
* tooling UI with `ImGui`
* 3D model import with `Assimp`

Next target in the physics stack:

* `Bullet` integration

---

## API Orientation

Start here:

1. [API.md](API.md) — central API reference
2. [`scripts/`](scripts) — runnable examples for each subsystem
3. [`scripts/demo_jolt_tank_csm_particles.bu`](scripts/demo_jolt_tank_csm_particles.bu) — full release-style scene

---

## Notes

* GIF capture hotkey in runtime is `F10`
* many demo scripts include their controls in header comments
* the release is intended for direct experimentation: edit scripts and run again

---

## Links

* BuLang VM: [https://github.com/akadjoker/BuLangVM](https://github.com/akadjoker/BuLangVM)
* API reference: [API.md](API.md)
* Contributing: [CONTRIBUTING.md](CONTRIBUTING.md)
* License: MIT

