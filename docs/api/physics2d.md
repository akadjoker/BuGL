# Physics 2D (Box2D)

BuGL includes Box2D bindings for 2D physics simulation.

## Setup

```bulang
var world = b2CreateWorld(0.0, -9.8);   // gravity
```

## Bodies

```bulang
var body = b2CreateDynamicBody(world, x, y);
b2AddCircle(body, radius, density, friction, restitution);
// or
b2AddBox(body, halfW, halfH, density, friction, restitution);
```

## Simulation

```bulang
process physics_loop() {
    while (Running()) {
        b2WorldStep(world, GetDeltaTime(), 8, 3);
        frame;
    }
}
```

## Reading State

```bulang
var (px, py) = b2GetPosition(body);
var angle    = b2GetAngle(body);
```

## Key Functions

| Function | Description |
|----------|-------------|
| `b2CreateWorld(gx, gy)` | Create physics world |
| `b2CreateDynamicBody(world, x, y)` | Dynamic rigid body |
| `b2CreateStaticBody(world, x, y)` | Static body (floor, walls) |
| `b2AddCircle(body, r, density, friction, rest)` | Circle collider |
| `b2AddBox(body, hw, hh, density, friction, rest)` | Box collider |
| `b2WorldStep(world, dt, velIter, posIter)` | Step simulation |
| `b2GetPosition(body)` | Returns `(x, y)` |
| `b2GetAngle(body)` | Returns rotation in radians |
| `b2SetLinearVelocity(body, vx, vy)` | Set velocity |
| `b2ApplyForce(body, fx, fy)` | Apply force |
