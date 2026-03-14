# Physics 3D (ODE)

BuGL includes ODE (Open Dynamics Engine) bindings for 3D physics.

## Setup

```bulang
dInitODE2(0);
var world = dWorldCreate();
var space = dSimpleSpaceCreate(nil);
dWorldSetGravity(world, 0.0, -9.8, 0.0);
```

## Creating Bodies

```bulang
var body = dBodyCreate(world);
var geom = dCreateBox(space, 1.0, 1.0, 1.0);
dGeomSetBody(geom, body);
```

## Simulation

```bulang
process sim() {
    var contacts = dJointGroupCreate(0);
    while (Running()) {
        dSpaceCollide(space, contacts);
        dWorldStep(world, GetDeltaTime());
        dJointGroupEmpty(contacts);
        frame;
    }
}
```

## Key Functions

| Function | Description |
|----------|-------------|
| `dInitODE2(flags)` | Initialize ODE |
| `dWorldCreate()` | Create physics world |
| `dSimpleSpaceCreate(space)` | Create collision space |
| `dWorldSetGravity(world, x, y, z)` | Set gravity |
| `dBodyCreate(world)` | Create rigid body |
| `dCreateBox(space, x, y, z)` | Box geometry |
| `dCreateSphere(space, radius)` | Sphere geometry |
| `dGeomSetBody(geom, body)` | Link geom to body |
| `dBodyGetPosition(body)` | Returns `(x, y, z)` |
| `dBodyGetRotation(body)` | Returns rotation matrix |
| `dWorldStep(world, dt)` | Step simulation |
