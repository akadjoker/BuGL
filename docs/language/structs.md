# Structs

Structs are lightweight value containers, ideal for data like positions, colors, or rectangles.

## Two Syntaxes

### Positional (no `var`)

Fields are set via a positional constructor:

```bulang
struct Vec2 { x, y }

var v = Vec2(10.0, 20.0);
print(v.x);   // 10.0
print(v.y);   // 20.0
```

### With `var` (no constructor args)

Fields start as `nil` and are set manually:

```bulang
struct Color {
    var r;
    var g;
    var b;
    var a;
}

var c = Color();
c.r = 255;
c.g = 128;
c.b = 0;
c.a = 255;
```

### Mixed

```bulang
struct Rect {
    var x;
    var y;
    width, height
}

var r = Rect();
r.x = 10;
r.y = 20;
r.width  = 100;
r.height = 50;
```

## Nested Structs

```bulang
struct Vec2  { x, y }
struct Rect2 { pos, size }

var rect = Rect2(Vec2(0.0, 0.0), Vec2(200.0, 100.0));
rect.pos.x  = 10.0;
rect.size.x = 300.0;
```

## Structs vs Classes

| | Struct | Class |
|---|---|---|
| Methods | ❌ | ✅ |
| Inheritance | ❌ | ✅ |
| `self` | ❌ | ✅ |
| Lightweight | ✅ | ✅ |
| Best for | Data (Vec2, Color, Rect) | Behaviour (Enemy, Player) |
