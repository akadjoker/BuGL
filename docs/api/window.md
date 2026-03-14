# Window & Device

## Initialization

```bulang
SetGLVersion(3, 3, 1);   // call BEFORE Init
Init("My Game", 800, 600);
```

## Functions

| Function | Description |
|----------|-------------|
| `Init(title, w, h)` | Create window and GL context |
| `SetGLVersion(major, minor, profile)` | Set OpenGL version (before Init) |
| `SetGLAttribute(attr, value)` | Set SDL/GL attribute (before Init) |
| `SetTitle(title)` | Change window title |
| `SetSize(w, h)` | Resize window |
| `GetWidth()` | Current window width |
| `GetHeight()` | Current window height |
| `Running()` | True while app should run |
| `Flip()` | Swap buffers (show frame) |
| `GetDeltaTime()` | Seconds since last frame |
| `GetFPS()` | Current FPS |
| `IsReady()` | True if initialized |
| `IsResized()` | True if resized this frame |
| `Quit()` | Signal close |
| `Close()` | Release resources |

## Minimal Game Loop

```bulang
Init("BuGL Game", 800, 600);

process main() {
    while (Running()) {
        Clear(30, 30, 30, 255);

        // update & draw here

        Flip();
        frame;
    }
}

main();
```

## Delta Time

```bulang
process player() {
    var x = 400.0;
    var speed = 200.0;   // pixels per second

    while (Running()) {
        var dt = GetDeltaTime();
        if (IsKeyDown(KEY_RIGHT)) { x += speed * dt; }
        if (IsKeyDown(KEY_LEFT))  { x -= speed * dt; }
        frame;
    }
}
```
