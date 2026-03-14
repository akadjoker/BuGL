# Audio

## Setup

```bulang
AudioInit();
```

Call once before using any audio functions.

## Loading & Playing

```bulang
var sfx_jump  = AudioLoadSfx("assets/jump.wav");
var sfx_shoot = AudioLoadSfx("assets/shoot.wav");

// Play: id, volume (0.0-1.0), pitch (1.0 = normal), pan (-1.0 to 1.0)
AudioPlaySfx(sfx_jump,  1.0, 1.0,  0.0);
AudioPlaySfx(sfx_shoot, 0.8, 1.2, -0.3);
```

## Update

Call every frame inside the game loop:

```bulang
process main() {
    while (Running()) {
        // ...
        AudioUpdate();
        Flip();
        frame;
    }
}
```

## Cleanup

```bulang
AudioClose();
```

## Functions

| Function | Description |
|----------|-------------|
| `AudioInit()` | Initialize audio system |
| `AudioLoadSfx(path)` | Load sound file, returns ID |
| `AudioPlaySfx(id, vol, pitch, pan)` | Play sound |
| `AudioUpdate()` | Process audio (call each frame) |
| `AudioClose()` | Release audio resources |
