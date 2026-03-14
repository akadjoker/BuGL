# Shapes Module

CPU-side mesh generation via `par_shapes`.

## Setup

```bulang
import Shapes;
using Shapes;
```

## Creating Meshes

```bulang
var box    = ShapeCreateBox(1.0, 1.0, 1.0);
var sphere = ShapeCreateSphere(1.0, 16, 16);
var plane  = ShapeCreatePlane(10.0, 10.0, 1, 1);
var torus  = ShapeCreateTorus(1.0, 0.3, 16, 8);
```

## All Creation Functions

| Function | Description |
|----------|-------------|
| `ShapeCreateBox(w, h, d)` | Box/Cube |
| `ShapeCreateSphere(r, slices, stacks)` | Sphere |
| `ShapeCreateCylinder(r, h, slices, stacks)` | Cylinder |
| `ShapeCreateCone(r, h, slices, stacks)` | Cone |
| `ShapeCreatePlane(w, d, slices, stacks)` | Flat plane |
| `ShapeCreateDisk(r, slices, stacks)` | Disk |
| `ShapeCreateHemisphere(r, slices, stacks)` | Hemisphere |
| `ShapeCreateTorus(major, minor, slices, stacks)` | Torus |
| `ShapeCreateIcoSphere(r, subdivisions)` | Icosphere |
| `ShapeCreateIcosahedron(r)` | Icosahedron |
| `ShapeCreateDodecahedron(r)` | Dodecahedron |
| `ShapeCreateOctahedron(r)` | Octahedron |
| `ShapeCreateTetrahedron(r)` | Tetrahedron |

## Accessing Data

```bulang
var positions = ShapeGetPositions(box);   // float Buffer (x,y,z,...)
var normals   = ShapeGetNormals(box);     // float Buffer
var uvs       = ShapeGetTexCoords(box);   // float Buffer or nil
var indices   = ShapeGetIndices(box);     // uint16 Buffer

var count = ShapeGetVertexCount(box);
var tris  = ShapeGetTriangleCount(box);
```

## Transforms (CPU)

```bulang
ShapeScale(box, 2.0, 1.0, 2.0);
ShapeTranslate(box, 0.0, 0.5, 0.0);
ShapeRotate(box, rad(45.0), 0.0, 1.0, 0.0);
ShapeComputeNormals(box);
```

## Cleanup

```bulang
ShapeDestroy(box);
ShapeClear();    // destroy all
```
