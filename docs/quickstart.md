# Quick Start

Get BuGL running in under 5 minutes.

## 1. Build

```bash
git clone https://github.com/akadjoker/BuGL
cd BuGL
mkdir build && cd build
cmake ..
make
```

## 2. Your First Script

Create `hello.bu`:

```bulang
Init("My Game", 800, 600);

process main() {
    var x = 100;
    var speed = 2;

    while (Running()) {
        Clear(30, 30, 30, 255);

        x += speed;
        if (x > 750) { speed = -2; }
        if (x < 50)  { speed =  2; }

        DrawCircle(x, 300, 20, 255, 100, 0, 255);
        Flip();
        frame;
    }
}

main();
```

## 3. Run It

```bash
./bugl hello.bu
```

You should see an orange circle bouncing horizontally.

## 4. Multiple Processes

BuGL shines with multiple processes running concurrently:

```bulang
Init("Processes", 800, 600);

process ball(startX, startY, r, g, b) {
    var x = startX;
    var y = startY;
    var vx = 3;
    var vy = 2;

    while (Running()) {
        x += vx;
        y += vy;
        if (x > 780 || x < 20) { vx = -vx; }
        if (y > 580 || y < 20) { vy = -vy; }
        DrawCircle(x, y, 15, r, g, b, 255);
        frame;
    }
}

process main() {
    ball(100, 100, 255, 80,  0);
    ball(400, 300,   0, 200, 255);
    ball(600, 150,  80, 255,  80);

    while (Running()) {
        Clear(20, 20, 20, 255);
        frame;
        Flip();
    }
}

main();
```

!!! info
    Each `ball(...)` call spawns an independent process.
    All processes run cooperatively — each `frame` yields control to the scheduler.

## 5. Next Steps

- [Language Reference](language/variables.md) — learn all the syntax
- [Processes](language/processes.md) — deep dive into cooperative multitasking
- [Tutorials](tutorials/01_hello_world.md) — guided examples
