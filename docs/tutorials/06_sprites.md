# Tutorial 6 — Sprites & Drawing

Loading textures and drawing sprites.

## Drawing Primitives

```bulang
Init("Drawing", 800, 600);

process main() {
    while (Running()) {
        Clear(20, 20, 30, 255);

        // Shapes
        DrawCircle(100, 100, 30, 255, 100,  50, 255);
        DrawRect(200, 80, 60, 60, 100, 200, 255, 255);
        DrawLine(300, 80, 400, 140, 255, 255, 100, 255);

        // Text
        DrawText("BuGL!", 500, 90, 24, 255, 255, 255, 255);

        Flip();
        frame;
    }
}

main();
```

## Loading a Texture

```bulang
Init("Sprites", 800, 600);

var tex = LoadTexture("assets/player.png");

process main() {
    var x = 100.0;
    var y = 100.0;

    while (Running()) {
        var dt = GetDeltaTime();
        Clear(20, 20, 30, 255);

        if (IsKeyDown(KEY_RIGHT)) { x += 150.0 * dt; }
        if (IsKeyDown(KEY_LEFT))  { x -= 150.0 * dt; }
        if (IsKeyDown(KEY_DOWN))  { y += 150.0 * dt; }
        if (IsKeyDown(KEY_UP))    { y -= 150.0 * dt; }

        DrawTexture(tex, x, y, 255, 255, 255, 255);

        Flip();
        frame;
    }
}

main();
```

## Sprite Sheet / Animation

```bulang
class Animator {
    def init(texture, frameW, frameH, fps) {
        self.tex    = texture;
        self.frameW = frameW;
        self.frameH = frameH;
        self.fps    = fps;
        self.frame  = 0;
        self.timer  = 0.0;
    }

    def update(dt) {
        self.timer += dt;
        if (self.timer >= 1.0 / self.fps) {
            self.timer = 0.0;
            self.frame++;
        }
    }

    def draw(x, y) {
        var srcX = self.frame * self.frameW;
        DrawTextureRec(self.tex, srcX, 0,
                       self.frameW, self.frameH,
                       x, y, 255, 255, 255, 255);
    }
}
```

## Font Rendering

```bulang
var font = STBFont();
font.load("assets/font.ttf", 20);

// In game loop:
font.drawText("Score: " + str(score), 10, 10, 255, 255, 100);
```
