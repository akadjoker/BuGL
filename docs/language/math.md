# Math

Math functions in BuGL are **built-in opcodes** — they compile directly to VM instructions, not function calls. This makes them extremely fast.

## Trigonometry

```bulang
var s = sin(angle);           // sine (radians)
var c = cos(angle);           // cosine (radians)
var t = tan(angle);           // tangent (radians)
var a = atan(x);              // arc tangent
var a2 = atan2(y, x);         // arc tangent of y/x (correct quadrant)
```

## Powers & Roots

```bulang
var sq = sqrt(16.0);           // 4.0
var pw = pow(2.0, 8.0);        // 256.0
var lg = log(2.718);           // ~1.0
var ex = exp(1.0);             // ~2.718
```

## Rounding

```bulang
var f = floor(3.9);            // 3.0
var c = ceil(3.1);             // 4.0
var a = abs(-42);              // 42
```

## Conversion

```bulang
var d = deg(3.14159);          // ~180.0  (radians -> degrees)
var r = rad(180.0);            // ~3.14159 (degrees -> radians)
```

## Time

```bulang
var t = clock();               // milliseconds since program start
```

## Common Patterns

```bulang
// Distance between two points
def distance(x1, y1, x2, y2) {
    var dx = x2 - x1;
    var dy = y2 - y1;
    return sqrt(dx*dx + dy*dy);
}

// Angle between two points
def angleTo(x1, y1, x2, y2) {
    return atan2(y2 - y1, x2 - x1);
}

// Move towards angle
def moveAngle(x, y, angle, speed) {
    return [x + cos(angle) * speed,
            y + sin(angle) * speed];
}

// Clamp
def clamp(v, mn, mx) {
    if (v < mn) return mn;
    if (v > mx) return mx;
    return v;
}
```
