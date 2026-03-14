# Graphics (OpenGL)

BuGL exposes raw OpenGL alongside higher-level helpers.

## Clear

```bulang
Clear(r, g, b, a);          // clear with color (0-255)
Clear(30, 30, 30, 255);     // dark grey background
```

## Shaders

```bulang
var prog = LoadShaderProgram("vert.glsl", "frag.glsl");
glUseProgram(prog);
```

## Buffers & Vertex Data

```bulang
var verts = Buffer(9 * 4, BUFFER_FLOAT32);
verts.writeFloat(-0.5); verts.writeFloat(-0.5); verts.writeFloat(0.0);
verts.writeFloat( 0.5); verts.writeFloat(-0.5); verts.writeFloat(0.0);
verts.writeFloat( 0.0); verts.writeFloat( 0.5); verts.writeFloat(0.0);

glBufferData(GL_ARRAY_BUFFER, verts, GL_STATIC_DRAW);
```

## Raw OpenGL Functions

| Function | Description |
|----------|-------------|
| `glClear(mask)` | Clear buffers |
| `glUseProgram(prog)` | Bind shader program |
| `glBufferData(target, data, usage)` | Upload buffer data |
| `glBufferSubData(target, offset, data)` | Update buffer region |
| `GLDebug(enable)` | Toggle GL debug output |
| `GLCheck(msg)` | Check for GL errors |
| `LoadShaderProgram(vert, frag)` | Compile and link shader |
| `SaveScreenshot(filename)` | Save screen to PNG |

## STB Font

```bulang
var font = STBFont();
font.load("assets/font.ttf", 24);
font.drawText("Score: 100", 10, 10, 255, 255, 255);
```
