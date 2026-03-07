# BuGL

BuGL is a graphics-oriented runtime that combines:

- **BuLang** scripting
- **SDL2** window/input/context management
- **OpenGL** bindings (legacy + modern)
- **STB utilities** (image I/O, resize, fonts, packing, noise)

The goal is simple: **learn and prototype computer graphics fast**, then scale to more advanced OpenGL workflows without leaving the same scripting environment.

## Why BuGL

- Fast iteration with scripts (no C++ rebuild for every visual tweak)
- Legacy OpenGL path for teaching and rapid experiments
- Modern OpenGL path for shaders, VBO/VAO/EBO, textures, uniforms
- Low-copy upload path via `Buffer` and built-in typed arrays
- Built-in helpers for GIF capture, font baking, atlas packing, triangulation

## Current Status

BuGL is actively evolving. Core rendering workflows are working and already useful for tutorials and demos.

## Project Docs

- [Contributing](CONTRIBUTING.md)
- [Sponsorship](SPONSORS.md)
- [License](LICENSE)

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

Binary:

- `bin/main`

## Run

Run a script directly:

```bash
./bin/main scripts/tutor_6.bu
```

Or use the launcher script:

```bash
./bin/main scripts/main.bu
```

## Learning Path (Recommended)

Start with the tutorial sequence:

- `scripts/tutor_1.bu` - legacy triangle and client arrays
- `scripts/tutor_2.bu` - transition concepts
- `scripts/tutor_3.bu` - modern pipeline minimal shader + VBO/VAO
- `scripts/tutor_4.bu` - camera + MVP + grid
- `scripts/tutor_5.bu` - FPS controls + GIF capture
- `scripts/tutor_6.bu` - 2D text rendering with `Font` + texture atlas

Extra demos:

- `scripts/demo_texture_quad.bu`
- `scripts/demo_phong.bu`
- `scripts/demo_shader_hotreload.bu`
- `scripts/demo_box2d_stack.bu`
- `scripts/demo_box2d_mouse_joint.bu`

Many demos support GIF capture with `F12` toggle.

## GIF Gallery

Drop captured GIFs under `assets/gifs/` and wire them here:

| Demo | Preview |
|---|---|
| `demo_box2d_mouse_joint.bu` | ![box2d mouse joint](assets/gifs/box2d_mouse_joint.gif) |
| `demo_box2d_stack.bu` | ![box2d stack](assets/gifs/box2d_stack.gif) |
| `demo_bloom_hdr.bu` | ![bloom hdr](assets/gifs/bloom_hdr.gif) |
| `demo_particles.bu` | ![particles](assets/gifs/particles.gif) |
| `demo_raymarching.bu` | ![raymarching](assets/gifs/raymarching.gif) |
| `demo_raytrace.bu` | ![raytrace](assets/gifs/raytrace.gif) |

## Module Overview

### `SDL`

Window creation, event/input access, and GL context attributes.

Typical usage:

```bu
SetGLVersion(3, 3, SDL_GL_CONTEXT_PROFILE_CORE);
SetGLAttribute(SDL_GL_DOUBLEBUFFER, 1);
Init("Demo", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
```

### `OpenGl`

Includes:

- Core state functions (`glEnable`, `glViewport`, `glClear`, ...)
- Legacy fixed pipeline + client arrays
- Vertex buffer API (`glBufferData`, `glBufferSubData`, ...)
- Texture API (`glTexImage2D`, params, pixel store)
- Shader/program API (compile/link/uniforms/attribs)
- Modern extras split by file (`instancing`, `query`, `ubo`, `rendertarget`)
- Productivity helpers:
  - `LoadShaderProgram(vertexPath, fragmentPath[, watch])` (auto hot reload on file changes when used with `glUseProgram`)
  - `ReloadShaderProgram(program)` (force reload now)
  - `GLDebug(enabled)` / `GLCheck([label])`
  - `SaveScreenshot(path[, flipY])`

Quick example:

```bu
GLDebug(true);
var prog = LoadShaderProgram("assets/shaders/basic.vert", "assets/shaders/basic.frag"); // watch = true by default

while (Running())
{
    glUseProgram(prog); // auto reload if files changed
    // draw...

    if (IsKeyPressed(KEY_F12)) SaveScreenshot("frame.png");
    Flip();
}
```

### `STB`

Includes:

- `stbi_*` image loading/writing
- resize helpers
- perlin helpers
- native classes:
  - `Font` (truetype load, bake, quads)
  - `RectPack`
  - `Gif`
  - `CDT` (poly2tri)

## Data Types for Uploads

BuGL supports three useful data paths for graphics uploads:

1. **Bu arrays**: easiest, may require conversion
2. **`Buffer` (`@(count, type)`)**: fixed-size native memory
3. **Typed arrays** (`Uint8Array`, `Float32Array`, ...): dynamic container with `add/reserve/pack`

Use `Buffer` or typed arrays for hot paths (meshes, textures, dynamic updates).

### Typed Array API

Available classes:

- `Uint8Array`
- `Int16Array`
- `Uint16Array`
- `Int32Array`
- `Uint32Array`
- `Float32Array`
- `Float64Array`

Shared methods:

- constructor: `size | array | buffer | typedarray`
- `add(...)`
- `clear()`
- `reserve(n)`
- `pack()`
- `get(i)` / `set(i, v)`
- `toBuffer()`
- `length()`, `capacity()`, `byteLength()`, `byteCapacity()`, `ptr()`

## Architecture / Source Layout

Main binding files:

- `main/src/imopengl.cpp` - OpenGL core + immediate mode + matrix ops
- `main/src/opengl_legacy.cpp` - legacy arrays and fixed-function helpers
- `main/src/opengl_vertexbuffer.cpp`
- `main/src/opengl_texture.cpp`
- `main/src/opengl_shader.cpp`
- `main/src/opengl_instancing.cpp`
- `main/src/opengl_query.cpp`
- `main/src/opengl_ubo.cpp`
- `main/src/opengl_rendertarget.cpp`
- `main/src/stb_bindings.cpp`
- `main/src/stb_truetype.cpp`
- `main/src/stb_rect_pack.cpp`
- `main/src/msf_gif.cpp`
- `main/src/poly2tri_binding.cpp`
- `main/src/device_input_bindings.cpp`
- `main/src/raymath.cpp`

Module registration entry point:

- `main/src/bindings.cpp`

## Known Notes

- Include path resolution currently tries multiple fallbacks; some "file not found" probes are expected before a successful include.
- Typed arrays use capacity-growth semantics; constructor with numeric arg creates capacity, not pre-filled logical length.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

BuGL is distributed under the [MIT License](LICENSE).
