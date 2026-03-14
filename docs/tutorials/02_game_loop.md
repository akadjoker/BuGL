# Tutorial 2 — Game Loop

Understanding delta time and smooth movement.

## What You'll Learn

- Delta time for frame-rate independent movement
- Moving objects smoothly
- Bouncing off screen edges

## Frame-Rate Independent Movement

Without delta time, movement speed depends on FPS.
With delta time, it's always in **pixels per second**:

```bulang
Init("Game Loop", 800, 600);

process main() {
    var x     = 400.0;
    var y     = 300.0;
    var speed = 200.0;   // pixels per second

    while (Running()) {
        var dt = GetDeltaTime();

        Clear(20, 20, 30, 255);

        if (IsKeyDown(KEY_RIGHT)) { x += speed * dt; }
        if (IsKeyDown(KEY_LEFT))  { x -= speed * dt; }
        if (IsKeyDown(KEY_DOWN))  { y += speed * dt; }
        if (IsKeyDown(KEY_UP))    { y -= speed * dt; }

        // clamp to screen
        if (x < 0)   { x = 0; }
        if (x > 780) { x = 780; }
        if (y < 0)   { y = 0; }
        if (y > 580) { y = 580; }

        DrawCircle(x, y, 20, 255, 150, 50, 255);
        DrawText("Arrow keys to move", 10, 10, 16, 200, 200, 200, 255);

        Flip();
        frame;
    }
}

main();
```

## Bouncing Ball

```bulang
Init("Bouncing Ball", 800, 600);

process ball() {
    var x  = 400.0;
    var y  = 300.0;
    var vx = 250.0;   // pixels/sec
    var vy = 180.0;

    while (Running()) {
        var dt = GetDeltaTime();

        x += vx * dt;
        y += vy * dt;

        if (x > 780 || x < 20) { vx = -vx; }
        if (y > 580 || y < 20) { vy = -vy; }

        DrawCircle(x, y, 20, 255, 80, 40, 255);
        frame;
    }
}

process main() {
    ball();
    while (Running()) {
        Clear(15, 15, 25, 255);
        frame;
        Flip();
    }
}

main();
```

!!! tip
    Notice that `ball()` spawns an independent process.
    The `main` process handles clearing and flipping;
    `ball` just handles its own movement and drawing.

## Next

→ [Tutorial 3 — Processes](03_processes.md)
