# BuGL API Reference

This reference documents the language syntax, global functions, native type methods, and modules available in BuGL.

## Language Syntax

### Declarations
- `var name = value;` - Variable declaration.
- `var (a, b) = func();` - Multi-return assignment.
- `def name(args) { ... }` - Function definition.
- `process name(args) { ... }` - Process definition (coroutine-like entity).
- `class Name { ... }` - Class definition.
- `struct Name { ... }` - Struct definition.

### Control Flow
- `if (cond) { ... } elif (cond) { ... } else { ... }`
- `while (cond) { ... }`
- `do { ... } while (cond);`
- `for (var i=0; i<10; i++) { ... }`
- `foreach (item in collection) { ... }`
- `loop { ... }` - Infinite loop.
- `break;` / `continue;`
- `switch (val) { case x: ... default: ... }`
- `return val;`
- `try { ... } catch (e) { ... } finally { ... }`
- `throw value;`

### Module Management
- `import moduleName;` - Imports a script module.
- `using moduleName;` - Imports symbols into current scope (no prefix needed).
- `require "pluginName";` - Loads a native plugin (DLL/Shared Lib).
- `include "path/to/script.bu";` - Inlines another script file.

## Global Functions (Intrinsics)

Built-in functions available globally.

| Function | Description |
|----------|-------------|
| `print(arg1, ...)` | Prints values to the console. |
| `len(obj)` | Returns the length of an array, string, or buffer. |
| `type(obj)` | Returns the type name of the object (e.g., "INTEGER", "ARRAY"). |
| `free(obj)` | Forces memory release of an object (advanced use). |
| `clock()` | Returns the time elapsed in seconds since the program started. |
| `proc(obj)` | Returns the process definition associated with an instance. |
| `get_id(obj)` | Returns the unique ID of a process instance. |

### Math

| Function | Description |
|----------|-------------|
| `sin(x)`, `cos(x)`, `tan(x)` | Trigonometric functions (radians). |
| `asin(x)`, `acos(x)`, `atan(x)` | Inverse trigonometric functions. |
| `atan2(y, x)` | Arc tangent of y/x with sign. |
| `sqrt(x)` | Square root. |
| `pow(base, exp)` | Power. |
| `abs(x)` | Absolute value. |
| `floor(x)`, `ceil(x)` | Round down/up. |
| `deg(rad)` | Converts radians to degrees. |
| `rad(deg)` | Converts degrees to radians. |
| `log(x)` | Natural logarithm. |
| `exp(x)` | Exponential (e^x). |

### Process Control (Inside `process`)

| Command | Description |
|---------|-------------|
| `frame(percent)` | Pauses process execution until the next frame (or % of frame). |
| `exit(code)` | Terminates the execution of the current process. |

### Process Properties (Built-in Privates)

Variables automatically available inside a `process`.

`x`, `y`, `z`, `graph`, `angle`, `size`, `flags`, `id`, `father`, `red`, `green`, `blue`, `alpha`, `tag`, `state`, `speed`, `group`, `velx`, `vely`, `hp`, `progress`, `life`, `active`, `show`, `xold`, `yold`, `sizex`, `sizey`.

---

## Device & Window Management

Functions for window creation and context management.

| Function | Description |
|----------|-------------|
| `Init(title, w, h, [flags])` | Initializes the window and graphics context. |
| `SetGLVersion(major, minor, profile)` | Sets OpenGL version (call before Init). |
| `SetGLAttribute(attr, value)` | Sets OpenGL attributes (call before Init). |
| `SetTitle(title)` | Changes the window title. |
| `SetSize(w, h)` | Resizes the window. |
| `GetWidth()` | Returns current window width. |
| `GetHeight()` | Returns current window height. |
| `GetWindow()` | Returns the raw SDL window pointer. |
| `GetContext()` | Returns the raw GL context pointer. |
| `IsReady()` | Returns true if the device is initialized. |
| `IsResized()` | Returns true if the window was resized this frame. |
| `Running()` | Returns true if the application loop should continue. |
| `Flip()` | Swaps buffers (updates the screen). |
| `GetDeltaTime()` | Returns time in seconds since the last frame. |
| `GetFPS()` | Returns the current frames per second. |
| `Quit()` | Signals the application to close. |
| `Close()` | Closes the device resources. |

---

## Input System

Functions to handle Mouse, Keyboard, and Gamepads.

### Mouse

| Function | Description |
|----------|-------------|
| `IsMousePressed(button)` | True if button was pressed this frame. |
| `IsMouseDown(button)` | True if button is currently held down. |
| `IsMouseReleased(button)` | True if button was released this frame. |
| `IsMouseUp(button)` | True if button is not pressed. |
| `GetMousePosition()` | Returns `(x, y)` tuple of mouse coordinates. |
| `GetMouseX()` | Returns mouse X coordinate. |
| `GetMouseY()` | Returns mouse Y coordinate. |
| `GetMouseDelta()` | Returns `(dx, dy)` movement since last frame. |
| `GetMouseWheelMove()` | Returns `(x, y)` scroll wheel delta. |
| `GetMouseWheelMoveV()` | Returns vertical scroll delta. |
| `SetMousePosition(x, y)` | Warps the mouse cursor. |
| `SetMouseOffset(x, y)` | Sets an offset for mouse coordinates. |
| `SetMouseScale(x, y)` | Sets a scale factor for mouse coordinates. |
| `SetMouseCursor(cursor)` | Changes the system cursor icon. |

### Keyboard

| Function | Description |
|----------|-------------|
| `IsKeyPressed(key)` | True if key was pressed this frame. |
| `IsKeyDown(key)` | True if key is currently held down. |
| `IsKeyReleased(key)` | True if key was released this frame. |
| `IsKeyUp(key)` | True if key is not pressed. |
| `GetKeyPressed()` | Returns the keycode of the last key pressed. |
| `GetCharPressed()` | Returns the unicode character of the last input. |

### Gamepad

| Function | Description |
|----------|-------------|
| `IsGamepadAvailable(id)` | True if gamepad `id` is connected. |
| `GetGamepadName(id)` | Returns the name of the gamepad. |
| `IsGamepadButtonPressed(id, btn)` | True if button was pressed this frame. |
| `IsGamepadButtonDown(id, btn)` | True if button is held down. |
| `IsGamepadButtonReleased(id, btn)` | True if button was released. |
| `IsGamepadButtonUp(id, btn)` | True if button is up. |
| `GetGamepadButtonPressed()` | Returns the last gamepad button pressed. |
| `GetGamepadAxisCount(id)` | Returns number of axes. |
| `GetGamepadAxisMovement(id, axis)` | Returns axis value (-1.0 to 1.0). |

### Input Constants

**Mouse Buttons:** `MOUSE_LEFT`, `MOUSE_RIGHT`, `MOUSE_MIDDLE`.

**Cursors:** `MOUSE_CURSOR_DEFAULT`, `MOUSE_CURSOR_ARROW`, `MOUSE_CURSOR_IBEAM`, `MOUSE_CURSOR_CROSSHAIR`, `MOUSE_CURSOR_POINTING_HAND`, `MOUSE_CURSOR_RESIZE_EW`, `MOUSE_CURSOR_RESIZE_NS`, `MOUSE_CURSOR_RESIZE_NWSE`, `MOUSE_CURSOR_RESIZE_NESW`, `MOUSE_CURSOR_RESIZE_ALL`, `MOUSE_CURSOR_NOT_ALLOWED`.

**Keys (Common):**
`KEY_SPACE`, `KEY_ESCAPE`, `KEY_ENTER`, `KEY_TAB`, `KEY_BACKSPACE`, `KEY_INSERT`, `KEY_DELETE`, `KEY_RIGHT`, `KEY_LEFT`, `KEY_DOWN`, `KEY_UP`, `KEY_F1`...`KEY_F12`, `KEY_0`...`KEY_9`, `KEY_A`...`KEY_Z`, `KEY_LEFT_SHIFT`, `KEY_LEFT_CONTROL`, `KEY_LEFT_ALT`.

**Gamepad Buttons:**
`GAMEPAD_BUTTON_LEFT_FACE_UP`, `GAMEPAD_BUTTON_LEFT_FACE_RIGHT`, `GAMEPAD_BUTTON_LEFT_FACE_DOWN`, `GAMEPAD_BUTTON_LEFT_FACE_LEFT`, `GAMEPAD_BUTTON_LEFT_TRIGGER_1`, `GAMEPAD_BUTTON_LEFT_TRIGGER_2`, `GAMEPAD_BUTTON_RIGHT_TRIGGER_1`, `GAMEPAD_BUTTON_RIGHT_TRIGGER_2`, `GAMEPAD_BUTTON_MIDDLE_LEFT`, `GAMEPAD_BUTTON_MIDDLE`, `GAMEPAD_BUTTON_MIDDLE_RIGHT`, `GAMEPAD_BUTTON_LEFT_THUMB`, `GAMEPAD_BUTTON_RIGHT_THUMB`.

**Gamepad Axes:**
`GAMEPAD_AXIS_LEFT_X`, `GAMEPAD_AXIS_LEFT_Y`, `GAMEPAD_AXIS_RIGHT_X`, `GAMEPAD_AXIS_RIGHT_Y`, `GAMEPAD_AXIS_LEFT_TRIGGER`, `GAMEPAD_AXIS_RIGHT_TRIGGER`.

---

## Shapes Module

CPU mesh generation via `par_shapes` exposed by ID (`int`).

```javascript
import Shapes;
using Shapes;

var shape = ShapeCreateBox(1.0, 1.0, 1.0);
var positions = ShapeGetPositions(shape);   // float buffer (x,y,z,...)
var indices = ShapeGetIndices(shape);       // uint16 buffer (i,j,k,...)
ShapeDestroy(shape);
```

### Creation

| Function | Description |
|----------|-------------|
| `ShapeCreateBox(w, h, d)` | Creates a centered box mesh. Returns `shapeId` (`0` on failure). |
| `ShapeCreateSphere(radius, slices, stacks)` | Creates a centered sphere mesh. |
| `ShapeCreateCylinder(radius, height, slices, stacks)` | Creates a centered cylinder mesh (height on Z). |
| `ShapeCreateCone(radius, height, slices, stacks)` | Creates a centered cone mesh (height on Z). |
| `ShapeCreatePlane(width, depth, slices, stacks)` | Creates a centered plane mesh on XY (`z=0`). |
| `ShapeCreateDisk(radius, slices, stacks)` | Creates a centered disk mesh. |
| `ShapeCreateHemisphere(radius, slices, stacks)` | Creates a hemisphere mesh. |
| `ShapeCreateTorus(majorRadius, minorRadius, slices, stacks)` | Creates a torus mesh. |
| `ShapeCreateIcoSphere(radius, subdivisions)` | Creates a subdivided icosphere mesh. |
| `ShapeCreateIcosahedron(radius)` | Creates an icosahedron mesh. |
| `ShapeCreateDodecahedron(radius)` | Creates a dodecahedron mesh. |
| `ShapeCreateOctahedron(radius)` | Creates an octahedron mesh. |
| `ShapeCreateTetrahedron(radius)` | Creates a tetrahedron mesh. |

### Query / Data

| Function | Description |
|----------|-------------|
| `ShapeExists(shapeId)` | Returns true if ID is valid. |
| `ShapeGetVertexCount(shapeId)` | Number of vertices. |
| `ShapeGetTriangleCount(shapeId)` | Number of triangles. |
| `ShapeGetIndexCount(shapeId)` | Number of indices (`triangles * 3`). |
| `ShapeGetPositions(shapeId)` | Returns float buffer with XYZ triplets. |
| `ShapeGetNormals(shapeId)` | Returns float buffer with normals (computed on demand if missing). |
| `ShapeComputeNormals(shapeId)` | Recomputes normals for this mesh. |
| `ShapeGetTexCoords(shapeId)` | Returns float buffer with UV pairs, or `nil` when absent. |
| `ShapeGetIndices(shapeId)` | Returns uint16 index buffer. |
| `ShapeGetAABB(shapeId)` | Returns `minX, minY, minZ, maxX, maxY, maxZ`. |

### Transforms (CPU-side)

| Function | Description |
|----------|-------------|
| `ShapeScale(shapeId, x, y, z)` | Scales mesh vertices. |
| `ShapeTranslate(shapeId, x, y, z)` | Translates mesh vertices. |
| `ShapeRotate(shapeId, radians, axisX, axisY, axisZ)` | Rotates mesh vertices around axis. |

### Lifetime

| Function | Description |
|----------|-------------|
| `ShapeDestroy(shapeId)` | Frees one mesh by ID. |
| `ShapeClear()` | Frees all stored meshes. |

---

## Native Type Methods

### Array / List

```javascript
var arr = [1, 2, 3];
arr.push(4);
```

| Method | Description |
|--------|-----------|
| `.push(val)` | Adds an element to the end. |
| `.pop()` | Removes and returns the last element. |
| `.back()` | Returns the last element without removing it. |
| `.length()` | Returns the number of elements. |
| `.clear()` | Removes all elements. |
| `.insert(index, val)` | Inserts an element at the specified position. |
| `.remove(index)` | Removes the element at the specified position. |
| `.find(val)` | Searches for a value and returns the index (or -1). |
| `.contains(val)` | Returns `true` if the value exists. |
| `.reverse()` | Reverses the order of elements. |
| `.join(sep)` | Joins elements into a string separated by `sep`. |
| `.slice(start, end)` | Returns a sub-array. |
| `.concat(other)` | Concatenates with another array. |
| `.fill(val)` | Fills the array with the value. |
| `.first()` | Returns the first element. |
| `.last()` | Returns the last element. |
| `.count(val)` | Counts occurrences of a value. |

### String

```javascript
var s = "Hello";
s = s.upper();
```

| Method | Description |
|--------|-----------|
| `.length()` | Returns the string length. |
| `.upper()` | Converts to uppercase. |
| `.lower()` | Converts to lowercase. |
| `.concat(str)` | Concatenates strings. |
| `.sub(start, len)` | Returns a substring. |
| `.replace(old, new)` | Replaces occurrences. |
| `.at(index)` | Returns the character at the index. |
| `.contains(str)` | Checks if it contains the substring. |
| `.trim()` | Removes whitespace from ends. |
| `.startswith(str)` | Checks if it starts with the string. |
| `.endswith(str)` | Checks if it ends with the string. |
| `.indexof(str)` | Returns the position of the substring. |
| `.repeat(n)` | Repeats the string N times. |
| `.split(sep)` | Splits the string into an array using the separator. |

### Map

```javascript
var m = { "key": "value" };
if (m.has("key")) ...
```

| Method | Description |
|--------|-----------|
| `.has(key)` | Checks if the key exists. |
| `.remove(key)` | Removes the entry associated with the key. |
| `.keys()` | Returns an array with the keys. |
| `.values()` | Returns an array with the values. |
| `.length()` | Returns the number of entries. |
| `.clear()` | Clears the map. |

### Buffer

Binary buffers for low-level data manipulation.

| Method | Description |
|--------|-----------|
| `.writeByte(b)`, `.readByte()` | Reads/Writes Uint8. |
| `.writeShort(s)`, `.readShort()` | Reads/Writes Int16. |
| `.writeUShort(s)`, `.readUShort()` | Reads/Writes Uint16. |
| `.writeInt(i)`, `.readInt()` | Reads/Writes Int32. |
| `.writeUInt(i)`, `.readUInt()` | Reads/Writes Uint32. |
| `.writeFloat(f)`, `.readFloat()` | Reads/Writes Float32. |
| `.writeDouble(d)`, `.readDouble()` | Reads/Writes Float64. |
| `.writeString(s)`, `.readString()` | Reads/Writes String (null-terminated). |
| `.seek(pos)` | Moves the cursor to the position. |
| `.tell()` | Returns the current cursor position. |
| `.rewind()` | Returns to the beginning (seek 0). |
| `.skip(n)` | Advances N bytes. |
| `.remaining()` | Bytes remaining until the end. |
| `.fill(val)` | Fills the buffer with a byte. |
| `.copy(src, dest, len)` | Copies data. |
| `.save(filename)` | Saves the buffer to a file. |

---

## Module: OS

Operating system interaction.

```javascript
import os;
```

| Function | Description |
|--------|-----------|
| `os.platform` | Constant string with platform name (windows, linux, macos, etc). |
| `os.spawn(cmd, args...)` | Starts a process. Returns PID. |
| `os.spawn_shell(cmd)` | Starts a command via shell. Returns PID. |
| `os.spawn_capture(cmd)` | Executes and captures stdout. Returns Map `{output, code}`. |
| `os.wait(pid, [timeout])` | Waits for a process. Returns exit code or nil (timeout). |
| `os.poll(pid)` | Checks process status. Returns exit code or nil (running). |
| `os.is_alive(pid)` | Returns true if the process (PID) is running. |
| `os.kill(pid, [sig])` | Terminates a process. |
| `os.execute(cmd)` | Executes command (system). Returns exit code. |
| `os.getenv(name)` | Gets environment variable. |
| `os.setenv(name, val)` | Sets environment variable. |
| `os.getcwd()` | Returns current directory. |
| `os.chdir(path)` | Changes current directory. |
| `os.quit(code)` | Terminates the BuGL application. |

---

## Graphics and Physics Modules (Summary)

> Note: These modules are loaded via `import` or available globally depending on configuration.

### SDL (Window and Input)
- `Init(title, w, h, flags)`
- `SetGLVersion(major, minor, profile)`
- `SetGLAttribute(attr, value)`
- `Running()`
- `Flip()`
- `Quit()`
- `GetDeltaTime()`
- `GetMouseDelta()`
- `IsKeyDown(key)`, `IsKeyPressed(key)`
- `SaveScreenshot(filename)`

### OpenGL
- `glClear(mask)`
- `glUseProgram(prog)`
- `glBufferData(target, size, data, usage)`
- `glBufferSubData(target, offset, size, data)`
- `GLDebug(enable)`
- `GLCheck(msg)`
- `LoadShaderProgram(vert, frag)`

### STB (Assets)
- **Font**
  - `.load(path, size)`
  - `.drawText(text, x, y, r, g, b)`
- **Gif**
  - `.begin(w, h)`
  - `.addFrame(pixels, delay)`
  - `.save(filename)`

### Audio
- `AudioInit()`
- `AudioLoadSfx(path)`
- `AudioPlaySfx(id, vol, pitch, pan)`
- `AudioUpdate()`
- `AudioClose()`

### Box2D (2D Physics)
- `b2CreateWorld(gx, gy)`
- `b2CreateDynamicBody(world, x, y)`
- `b2AddCircle(body, radius, ...)`
- `b2WorldStep(world, dt, velIter, posIter)`
- `b2GetPosition(body)`

### ODE (3D Physics)
- `dInitODE2(flags)`
- `dWorldCreate()`
- `dSimpleSpaceCreate(space)`
- `dWorldSetGravity(world, x, y, z)`
- `dBodyCreate(world)`
- `dGeomSetBody(geom, body)`
- `dCreateBox(space, x, y, z)`

---

## Binary Data Types

For efficient GPU upload (OpenGL):
- `Uint8Array`, `Int16Array`, `Uint16Array`
- `Int32Array`, `Uint32Array`
- `Float32Array`, `Float64Array`
