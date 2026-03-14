# Tutorial 4 — Classes & Structs

Organize your game entities with classes and structs.

## Structs for Data

```bulang
struct Vec2  { x, y }
struct Color { r, g, b, a }

var pos   = Vec2(100.0, 200.0);
var white = Color(255, 255, 255, 255);
```

## Classes for Behaviour

```bulang
class Bullet {
    def init(x, y, vx, vy) {
        self.x  = x;
        self.y  = y;
        self.vx = vx;
        self.vy = vy;
        self.alive = true;
    }

    def update(dt) {
        self.x += self.vx * dt;
        self.y += self.vy * dt;
        if (self.x < 0 || self.x > 800 ||
            self.y < 0 || self.y > 600) {
            self.alive = false;
        }
    }

    def draw() {
        DrawCircle(self.x, self.y, 4, 255, 255, 100, 255);
    }
}
```

## Managing a List of Objects

```bulang
Init("Bullets", 800, 600);

var bullets = [];

process main() {
    var px = 400.0;

    while (Running()) {
        var dt = GetDeltaTime();
        Clear(15, 15, 25, 255);

        // Move player
        if (IsKeyDown(KEY_RIGHT)) { px += 200.0 * dt; }
        if (IsKeyDown(KEY_LEFT))  { px -= 200.0 * dt; }

        // Shoot
        if (IsKeyPressed(KEY_SPACE)) {
            bullets.push(Bullet(px, 550.0, 0.0, -400.0));
        }

        // Update & draw bullets
        var i = 0;
        while (i < len(bullets)) {
            bullets[i].update(dt);
            if (bullets[i].alive) {
                bullets[i].draw();
                i++;
            } else {
                bullets.remove(i);
            }
        }

        // Draw player
        DrawCircle(px, 550.0, 15, 100, 200, 255, 255);

        Flip();
        frame;
    }
}

main();
```

## Inheritance

```bulang
class Enemy {
    def init(x, y, hp) {
        self.x  = x;
        self.y  = y;
        self.hp = hp;
    }
    def isAlive() { return self.hp > 0; }
    def takeDamage(d) { self.hp -= d; }
    def draw() { DrawCircle(self.x, self.y, 10, 255, 0, 0, 255); }
}

class FastEnemy extends Enemy {
    def init(x, y) {
        super.init(x, y, 30);    // less HP
        self.speed = 5;
    }
    def update() {
        self.x += self.speed;
        if (self.x > 800) { self.x = 0; }
    }
}
```

## Next

→ [Tutorial 5 — Input](05_input.md)
