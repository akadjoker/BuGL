# Tutorial 1 — Hello World

Your first BuGL program.

## What You'll Learn

- How to create a window
- The main game loop
- Drawing text on screen

## The Code

```bulang
Init("Hello BuGL", 800, 600);

process main() {
    while (Running()) {
        Clear(30, 30, 40, 255);

        DrawText("Hello, World!", 300, 280, 24, 255, 255, 255, 255);

        Flip();
        frame;
    }
}

main();
```

## Step by Step

### 1. `Init(title, width, height)`

Creates the window and OpenGL context. Always call this first.

### 2. `process main()`

The entry process. BuGL uses processes instead of a single main function.

### 3. `while (Running())`

`Running()` returns `true` until the user closes the window or `Quit()` is called.

### 4. `Clear(r, g, b, a)`

Clears the screen with the given color (0-255 per channel).

### 5. `Flip()`

Swaps front and back buffers — this actually shows what you drew.

### 6. `frame`

Yields control back to the scheduler. **Required** in every process loop.

## Adding Interactivity

```bulang
Init("Hello BuGL", 800, 600);

process main() {
    var color_r = 255;

    while (Running()) {
        Clear(20, 20, 30, 255);

        if (IsKeyDown(KEY_SPACE)) {
            color_r = 100;
        } else {
            color_r = 255;
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            Quit();
        }

        DrawText("Press SPACE!", 280, 280, 24, color_r, 200, 100, 255);

        Flip();
        frame;
    }
}

main();
```

## Next

→ [Tutorial 2 — Game Loop](02_game_loop.md)
