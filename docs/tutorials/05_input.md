# Tutorial 5 — Input

Handle keyboard, mouse and gamepad.

## Keyboard

```bulang
Init("Input Demo", 800, 600);

process main() {
    var x = 400.0;
    var y = 300.0;

    while (Running()) {
        var dt = GetDeltaTime();
        Clear(20, 20, 30, 255);

        // Held — smooth movement
        if (IsKeyDown(KEY_RIGHT)) { x += 200.0 * dt; }
        if (IsKeyDown(KEY_LEFT))  { x -= 200.0 * dt; }
        if (IsKeyDown(KEY_DOWN))  { y += 200.0 * dt; }
        if (IsKeyDown(KEY_UP))    { y -= 200.0 * dt; }

        // Just pressed — single action
        if (IsKeyPressed(KEY_ESCAPE)) { Quit(); }
        if (IsKeyPressed(KEY_F11))    { ToggleFullscreen(); }

        DrawCircle(x, y, 20, 100, 200, 255, 255);
        Flip();
        frame;
    }
}

main();
```

## Mouse

```bulang
process main() {
    while (Running()) {
        Clear(20, 20, 30, 255);

        var mx = GetMouseX();
        var my = GetMouseY();

        // Draw at mouse position
        DrawCircle(mx, my, 10, 255, 255, 100, 255);

        if (IsMousePressed(MOUSE_LEFT)) {
            print("Click at " + str(mx) + "," + str(my));
        }

        if (IsMouseDown(MOUSE_RIGHT)) {
            DrawCircle(mx, my, 20, 255, 100, 100, 128);
        }

        Flip();
        frame;
    }
}
```

## Gamepad

```bulang
process main() {
    var x = 400.0;
    var y = 300.0;

    while (Running()) {
        Clear(20, 20, 30, 255);
        var dt = GetDeltaTime();

        if (IsGamepadAvailable(0)) {
            var ax = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
            var ay = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);

            // Deadzone
            if (abs(ax) > 0.1) { x += ax * 200.0 * dt; }
            if (abs(ay) > 0.1) { y += ay * 200.0 * dt; }

            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                print("A pressed!");
            }
        }

        DrawCircle(x, y, 20, 255, 150, 50, 255);
        Flip();
        frame;
    }
}
```

## Next

→ [Tutorial 6 — Sprites & Drawing](06_sprites.md)
