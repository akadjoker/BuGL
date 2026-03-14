# Tutorial 3 — Processes

Cooperative multitasking — the heart of BuGL.

## What You'll Learn

- Spawning multiple processes
- `frame` and cooperative scheduling
- Sharing global state between processes

## The Problem Without Processes

```bulang
// BAD — sequential, blocks
movePlayer();
moveEnemy1();
moveEnemy2();   // only runs after enemy1 finishes
```

## With Processes

```bulang
Init("Processes", 800, 600);

var score = 0;

process enemy(startX, startY, col_r, col_g, col_b) {
    var x  = startX;
    var y  = startY;
    var vx = 2;

    while (Running()) {
        x += vx;
        if (x > 780 || x < 0) {
            vx = -vx;
            score++;      // shared global!
        }
        DrawCircle(x, y, 12, col_r, col_g, col_b, 255);
        frame;
    }
}

process main() {
    // Spawn 3 enemies — all run concurrently
    enemy(100, 150, 255, 80,  80);
    enemy(300, 300,  80, 255, 80);
    enemy(500, 450,  80, 80, 255);

    while (Running()) {
        if (IsKeyPressed(KEY_ESCAPE)) { Quit(); }

        Clear(20, 20, 30, 255);
        frame;   // let enemies run
        DrawText("Score: " + str(score), 10, 10, 20, 255, 255, 100, 255);
        Flip();
    }
}

main();
```

## How Scheduling Works

```
Frame 1:
  main runs → Clear → frame
  enemy0 runs → move → draw → frame
  enemy1 runs → move → draw → frame
  enemy2 runs → move → draw → frame
  main resumes → DrawText → Flip

Frame 2: (same cycle repeats)
```

!!! warning "Always put `frame` in loops"
    A process without `frame` in its loop will block all other processes.
    The VM will appear to hang.

## Built-in Process Variables

```bulang
process ship() {
    x = 400;
    y = 500;
    hp = 100;
    speed = 3;

    while (hp > 0 && Running()) {
        if (IsKeyDown(KEY_RIGHT)) { x += speed; }
        if (IsKeyDown(KEY_LEFT))  { x -= speed; }
        // draw at built-in x, y
        DrawCircle(x, y, 15, 200, 200, 255, 255);
        frame;
    }
}
```

## Next

→ [Tutorial 4 — Classes & Structs](04_classes.md)
