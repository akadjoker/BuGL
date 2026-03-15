# BuGL

> Build your own engine in script.

BuGL is not a closed game engine. BuGL is an engine-building runtime.
You write `.bu` scripts in BuLang and assemble your own stack: rendering, physics, AI navigation, UI, and tooling.

BuLang is easy to iterate with (Python-like workflow), but keeps explicit engine structure with braces, `class`, `struct`, `process`, and generics (`<>`).

## First Public Release (v2.0.1)

This is the first public release focused on practical, programmable game-tech blocks:

- Plugin-based runtime architecture.
- 3D physics with `Jolt`.
- 2D physics with `Chipmunk` and `Box2D`.
- Navigation and agents with `Recast/Detour`, `OpenSteer`, and `MicroPather`.
- Tooling UI with `ImGui`.
- 3D model import pipeline with `Assimp`.
- Generics in BuLang (`<>`) for reusable gameplay/framework code.

BuGL is designed for people who want control: no editor lock-in, no monolithic workflow, no hidden pipeline.

## Quick Start (5 Minutes)

Start here:

```bash
git clone https://github.com/akadjoker/BuGL
cd BuGL
cmake -S . -B build
cmake --build build -j
./bin/bugl scripts/tutor_1.bu
```

If that runs, continue with:

1. `./bin/bugl scripts/tutor_3.bu`
2. `./bin/bugl scripts/tutor_6.bu`
3. `./bin/bugl scripts/demo_jolt_tank_csm_particles.bu`

## What BuGL Is

- `BuLang` = language (syntax, flow control, arrays, classes).
- `BuGL` = runtime modules exposed to BuLang.
- Typical loop in scripts: `Init -> while (Running) -> update -> draw -> Flip`.

You can prototype rendering and gameplay logic fast, without C++ boilerplate in each demo.

## Plugin-Based Runtime

BuGL uses plugins for bigger feature groups.  
At runtime, scripts load what they need (`require "Jolt"`, etc.).

Main plugin modules:

| Plugin | Module in script | Typical usage |
|------|------|------|
| `BuJolt` | `Jolt` | 3D vehicles, rigid bodies, constraints |
| `BuOde` | `ODE` | 3D rigid body simulation |
| `BuBox2d` | `Box2D` | 2D physics |
| `BuChipmunk` | `Chipmunk` | Alternative 2D physics |
| `BuRecast` | `Recast` | Navmesh + Detour pathfinding/crowd |
| `BuOpenSteer` | `OpenSteer` | Steering behaviors |
| `BuMicroPather` | `MicroPather` | Grid/pathfinding |
| `BuAssimp` | `Assimp` | 3D model loading |

Plugin binaries are in `bin/plugins/`.

## AI and Navigation Stack

BuGL is strong for real-time agent behavior:

- `Recast/Detour`: navmesh generation, path queries, and crowd movement for large scenes.
- `OpenSteer`: flocking, pursuit, avoidance, and steering behaviors for responsive agents.
- `MicroPather`: fast grid-based pathfinding for tactics, RTS logic, and fallback routing.
- Combined workflow: high-level path (`Detour`) + local motion (`OpenSteer`) + deterministic grid logic (`MicroPather`).

Relevant demos: `demo_recast_*`, `demo_opensteer_*`, `demo_steertest.bu`, `demo_micropather.bu`.

## Gallery

| Ray Tracer | Bloom + HDR | Particles |
|:-:|:-:|:-:|
| ![ray tracer](gif/raytrace.gif) | ![bloom hdr](gif/bloom_hdr.gif) | ![particles](gif/particles.gif) |
| Reflections · refraction · Fresnel | Multi-pass HDR + bloom | High-count particle rendering |

| Ray Marching | Terrain | Box2D |
|:-:|:-:|:-:|
| ![raymarching](gif/raymarching.gif) | ![terrain](gif/terrain.gif) | ![box2d mouse joint](gif/box2d_mouse_joint.gif) |
| SDF + lighting | Procedural terrain | 2D physics interaction |

| ODE Car | ODE Fall 3D | Box2D Stack |
|:-:|:-:|:-:|
| ![ode car](gif/ode_car.gif) | ![ode fall 3d](gif/ode_fall3d.gif) | ![box2d stack](gif/box2d_stack.gif) |
| Car physics | 3D rigid body collisions | Stack stability |

| Jolt Vehicle | Jolt Motorcycle | Jolt Tank |
|:-:|:-:|:-:|
| ![jolt vehicle](gif/jolt_vehicle.gif) | ![jolt motorcycle](gif/jolt_motorcycle.gif) | ![jolt tank](gif/jolt_tank.gif) |
| Wheeled vehicle controller | Bike controller | Tracked tank + turret |

| Portal Showcase | ImGui Components | Batch Lines |
|:-:|:-:|:-:|
| ![showcase portal](gif/showcase_portal.gif) | ![imgui components](gif/imgui_componnts.gif) | ![batch lines](gif/batch_lines.gif) |
| Render-to-texture portal | ImGui widgets/bindings | 2D Batch primitives |

| Batch 3D Primitives | Tank CSM + Sparks | Tutor 5 Capture |
|:-:|:-:|:-:|
| ![batch 3d primitives](gif/batch_3d_primitives.gif) | ![tank jolt csm](gif/tank_jolt.gif) | ![tutor5 capture](gif/capture_tutor5.gif) |
| 3D primitives with Batch | Jolt + cascaded shadows + particles | FPS camera tutorial |

| OpenSteer Boids | OpenSteer Path | Steering Test |
|:-:|:-:|:-:|
| ![opensteer boids](gif/opensteer_boids.gif) | ![opensteer path](gif/opensteer_path.gif) | ![steertest](gif/steertest.gif) |
| Flocking behaviors | Path following | Steering sandbox |

| MicroPather Pathfinding | OpenSteer Showcase | Assimp Import |
|:-:|:-:|:-:|
| ![micropather pathfinding](gif/pathfind.gif) | ![opensteer](gif/opensteer.gif) | ![assimp](gif/assimp.gif) |
| Grid pathfinding | Steering behaviors | Model import pipeline |

## Learning Path

Tutorials:

1. `scripts/tutor_1.bu` (window + loop + basic draw)
2. `scripts/tutor_2.bu` (arrays/buffers basics)
3. `scripts/tutor_3.bu` (shader + VBO flow)
4. `scripts/tutor_4.bu` (camera + matrix usage)
5. `scripts/tutor_5.bu` (FPS controls + capture workflow)
6. `scripts/tutor_6.bu` (text/font flow)

Then demos by topic:

- Rendering: `demo_phong.bu`, `demo_texture_quad.bu`, `demo_bloom_hdr.bu`
- Advanced visuals: `demo_raymarching.bu`, `demo_raytrace.bu`, `demo_shadowmap_csm_v3.bu`
- Physics: `demo_jolt_*`, `demo_ode_*`, `demo_box2d_*`
- AI/Navigation: `demo_opensteer_*`, `demo_steertest.bu`, `demo_recast_*`, `demo_micropather.bu`
- UI/tools: `demo_imgui.bu`, `test_imgui_widgets_smoke.bu`, `test_font.bu`

## API Orientation

Start with these files:

1. [API.md](API.md): central API reference.
2. [`scripts/`](scripts): runnable examples for each subsystem.
3. [`scripts/demo_jolt_tank_csm_particles.bu`](scripts/demo_jolt_tank_csm_particles.bu): full release-style scene.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

Run scripts:

```bash
./bin/bugl scripts/tutor_1.bu
./bin/bugl scripts/main.bu
```

## Common Commands

```bash
# run main release demo
./bin/bugl scripts/demo_jolt_tank_csm_particles.bu

# run ImGui widget smoke
./bin/bugl scripts/test_imgui_widgets_smoke.bu
```

## Notes

- GIF capture hotkey in runtime is `F10`.
- Most demo scripts also include their own controls in header comments.

## Links

- BuLang VM: https://github.com/akadjoker/BuLangVM
- API reference: [API.md](API.md)
- Contributing: [CONTRIBUTING.md](CONTRIBUTING.md)
- License: MIT
