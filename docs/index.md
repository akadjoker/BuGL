# BuGL — Scripting Language for Games

**BuGL** is a fast, lightweight scripting language embedded in C++, designed for game development.
It runs on the BuLang VM and exposes SDL, OpenGL, Box2D, ODE and audio via a clean scripting API.

!!! tip "Inspired by DIV Games Studio"
    BuGL uses a cooperative multitasking model via `process` and `frame`,
    just like DIV Games Studio — but with modern syntax, classes, closures, and exception handling.

## Features

- **Processes** — cooperative multitasking with `process` + `frame`
- **Classes** — with inheritance (`extends`) and `self`/`super`
- **Structs** — lightweight value types
- **Closures** — nested functions that capture outer variables
- **Try/Catch/Finally** — full exception handling
- **Built-in Math** — `sin`, `cos`, `sqrt`, `pow`, etc. as direct opcodes
- **Native Bindings** — SDL, OpenGL, Box2D, ODE, Audio
- **Garbage Collected** — with optional manual `free`
- **Modules** — `import`, `using`, `include`, `require`

## Hello World

```bulang
Init("Hello BuGL", 800, 600);

process main() {
    while (Running()) {
        Clear(0, 0, 0, 255);
        DrawText("Hello World!", 300, 280, 24, 255, 255, 255, 255);
        Flip();
        frame;
    }
}

main();
```

## Installation & Build

```bash
git clone https://github.com/akadjoker/BuGL
cd BuGL
mkdir build && cd build
cmake ..
make
```

## Running a Script

```bash
./bugl scripts/main.bu
```

## Quick Navigation

- [Quick Start](quickstart.md) — up and running in 5 minutes
- [Language Reference](language/variables.md) — full syntax guide
- [BuGL API](api/window.md) — window, input, graphics
- [Tutorials](tutorials/01_hello_world.md) — step-by-step guides
