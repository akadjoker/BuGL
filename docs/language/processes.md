# Processes

Processes are BuGL's cooperative multitasking primitive, inspired by DIV Games Studio.
Each process has its own execution stack and runs concurrently with others.

## Basic Process

```bulang
process ball(x, y) {
    var vx = 3;
    var vy = 2;

    while (Running()) {
        x += vx;
        y += vy;
        if (x > 780 || x < 0) { vx = -vx; }
        if (y > 580 || y < 0) { vy = -vy; }
        DrawCircle(x, y, 10, 255, 100, 0, 255);
        frame;   // yield to scheduler
    }
}

ball(100, 100);
```

## `frame` — Yielding

`frame` pauses the current process and lets all other processes run one step.
Without `frame` inside a loop, the process blocks everything.

```bulang
process timer(seconds) {
    var elapsed = 0.0;
    while (elapsed < seconds) {
        elapsed += GetDeltaTime();
        frame;   // REQUIRED — otherwise infinite loop blocks VM
    }
    print("Timer done!");
}
```

## Multiple Processes

```bulang
process enemy(x, y) {
    while (Running()) {
        // AI logic
        frame;
    }
}

process main() {
    enemy(100, 200);
    enemy(400, 300);
    enemy(600, 100);

    while (Running()) {
        Clear(0, 0, 0, 255);
        frame;
        Flip();
    }
}

main();
```

## Built-in Process Variables

Every `process` has these variables automatically available:

`x`, `y`, `z`, `angle`, `speed`, `graph`, `size`,
`flags`, `id`, `father`, `state`, `group`, `tag`,
`red`, `green`, `blue`, `alpha`,
`velx`, `vely`, `hp`, `life`, `active`, `show`,
`xold`, `yold`, `sizex`, `sizey`, `progress`.

```bulang
process player() {
    x = 400;
    y = 300;
    hp = 100;

    while (hp > 0 && Running()) {
        if (IsKeyDown(KEY_RIGHT)) { x += speed; }
        if (IsKeyDown(KEY_LEFT))  { x -= speed; }
        frame;
    }
}
```

## Process Control

| Function | Description |
|----------|-------------|
| `frame` | Yield control for one frame |
| `frame(percent)` | Yield for a fraction of a frame |
| `exit` | Terminate current process |
| `get_id(proc)` | Get unique process ID |
| `proc(instance)` | Get process definition |

!!! warning
    Fibers (`fiber`/`yield`) have been removed. Use `process` + `frame` for all concurrency.
