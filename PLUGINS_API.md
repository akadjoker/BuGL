# BuGL Plugins API Reference

This document describes all native plugin modules available via `require` in BuGL scripts.
Each plugin registers classes, functions, and constants into a named module.

---

## Table of Contents

- [BuAssimp — Module `Assimp`](#buassimp--module-assimp)
- [BuBox2d — Module `Box2D`](#bubox2d--module-box2d)
- [BuChipmunk — Module `Chip2D`](#buchipmunk--module-chip2d)
- [BuJolt — Module `Jolt`](#bujolt--module-jolt)
- [BuMicroPather — Module `MicroPather`](#bumicropather--module-micropather)
- [BuOde — Module `ODE`](#buode--module-ode)
- [BuOpenSteer — Module `OpenSteer`](#buopensteer--module-opensteer)
- [BuRecast — Module `Recast`](#burecast--module-recast)

---

## BuAssimp — Module `Assimp`

3D model importer based on the Open Asset Import Library (Assimp).

```bu
require "BuAssimp";
import Assimp;
```

### Classes

#### `AssimpScene`

Main entry point. Load a 3D model file and inspect its meshes, materials, nodes and animations.

| Method | Signature | Returns |
|--------|-----------|---------|
| `load` | `load(path: string [, flags: int])` | `bool` |
| `isValid` | `isValid()` | `bool` |
| `getMeshCount` | `getMeshCount()` | `int` |
| `getMaterialCount` | `getMaterialCount()` | `int` |
| `getMesh` | `getMesh(index: int)` | `AssimpMesh` |
| `getMaterial` | `getMaterial(index: int)` | `AssimpMaterial` |
| `getRootNode` | `getRootNode()` | `AssimpNode` |
| `findNodeByName` | `findNodeByName(name: string)` | `AssimpNode \| nil` |
| `getAnimationCount` | `getAnimationCount()` | `int` |
| `getAnimation` | `getAnimation(index: int)` | `AssimpAnimation` |
| `getError` | `getError()` | `string` |
| `free` | `free()` | — |

#### `AssimpMesh`

Vertex / face / bone data for a single mesh inside the scene.

| Method | Signature | Returns |
|--------|-----------|---------|
| `getName` | `getName()` | `string` |
| `getVertexCount` | `getVertexCount()` | `int` |
| `getFaceCount` | `getFaceCount()` | `int` |
| `getMaterialIndex` | `getMaterialIndex()` | `int` |
| `hasNormals` | `hasNormals()` | `bool` |
| `hasUVs` | `hasUVs([channel: int])` | `bool` |
| `hasTangents` | `hasTangents()` | `bool` |
| `getVertex` | `getVertex(index: int)` | `x, y, z` |
| `getNormal` | `getNormal(index: int)` | `x, y, z` |
| `getUV` | `getUV(index: int [, channel: int])` | `u, v` |
| `getTangent` | `getTangent(index: int)` | `x, y, z` |
| `getFace` | `getFace(index: int)` | `i0, i1, i2` |
| `getVertices` | `getVertices()` | `Array` |
| `getNormals` | `getNormals()` | `Array` |
| `getUVs` | `getUVs([channel: int])` | `Array` |
| `getTangents` | `getTangents()` | `Array` |
| `getIndices` | `getIndices()` | `Array` |
| `getBoneCount` | `getBoneCount()` | `int` |
| `getBoneName` | `getBoneName(boneIndex: int)` | `string` |
| `getBoneOffsetMatrix` | `getBoneOffsetMatrix(boneIndex: int)` | `Matrix` |
| `getBoneWeightCount` | `getBoneWeightCount(boneIndex: int)` | `int` |
| `getBoneWeight` | `getBoneWeight(boneIndex: int, weightIndex: int)` | `vertexId, weight` |
| `getBoneVertexWeights` | `getBoneVertexWeights(boneIndex: int)` | `Array` |

#### `AssimpMaterial`

Material properties and texture paths.

| Method | Signature | Returns |
|--------|-----------|---------|
| `getName` | `getName()` | `string` |
| `getDiffuseColor` | `getDiffuseColor()` | `r, g, b, a` |
| `getSpecularColor` | `getSpecularColor()` | `r, g, b, a` |
| `getAmbientColor` | `getAmbientColor()` | `r, g, b, a` |
| `getEmissiveColor` | `getEmissiveColor()` | `r, g, b, a` |
| `getOpacity` | `getOpacity()` | `float` |
| `getShininess` | `getShininess()` | `float` |
| `getTextureCount` | `getTextureCount([type: int])` | `int` |
| `hasTexture` | `hasTexture(type: int [, index: int])` | `bool` |
| `getTexturePath` | `getTexturePath(type: int [, index: int])` | `string` |
| `getFirstTexturePath` | `getFirstTexturePath(type: int)` | `string` |
| `getTexturePaths` | `getTexturePaths(type: int)` | `Array<string>` |
| `getDiffuseTexturePath` | `getDiffuseTexturePath([index: int])` | `string` |
| `getNormalTexturePath` | `getNormalTexturePath([index: int])` | `string` |
| `getEmissiveTexturePath` | `getEmissiveTexturePath([index: int])` | `string` |

#### `AssimpNode`

Scene-graph node (hierarchy of transforms).

| Method | Signature | Returns |
|--------|-----------|---------|
| `getName` | `getName()` | `string` |
| `getChildCount` | `getChildCount()` | `int` |
| `getChild` | `getChild(index: int)` | `AssimpNode` |
| `getMeshCount` | `getMeshCount()` | `int` |
| `getMeshIndex` | `getMeshIndex(index: int)` | `int` |
| `getParent` | `getParent()` | `AssimpNode \| nil` |
| `getTransform` | `getTransform()` | `Matrix` |

#### `AssimpAnimation`

Skeletal / node animation clip.

| Method | Signature | Returns |
|--------|-----------|---------|
| `getName` | `getName()` | `string` |
| `getDurationTicks` | `getDurationTicks()` | `double` |
| `getTicksPerSecond` | `getTicksPerSecond()` | `double` |
| `getDurationSeconds` | `getDurationSeconds()` | `double` |
| `getChannelCount` | `getChannelCount()` | `int` |
| `getChannel` | `getChannel(index: int)` | `AssimpNodeAnim` |
| `findChannel` | `findChannel(nodeName: string)` | `AssimpNodeAnim \| nil` |
| `sampleNodeTransform` | `sampleNodeTransform(nodeName: string, timeSec: double [, loop: bool])` | `Matrix` |

#### `AssimpNodeAnim`

Per-node keyframe channel within an animation.

| Method | Signature | Returns |
|--------|-----------|---------|
| `getNodeName` | `getNodeName()` | `string` |
| `getPositionKeyCount` | `getPositionKeyCount()` | `int` |
| `getRotationKeyCount` | `getRotationKeyCount()` | `int` |
| `getScalingKeyCount` | `getScalingKeyCount()` | `int` |
| `getPositionKey` | `getPositionKey(index: int)` | `time, x, y, z` |
| `getRotationKey` | `getRotationKey(index: int)` | `time, x, y, z, w` |
| `getScalingKey` | `getScalingKey(index: int)` | `time, x, y, z` |
| `getPreState` | `getPreState()` | `int` |
| `getPostState` | `getPostState()` | `int` |
| `sampleTransform` | `sampleTransform(timeSec: double [, loop: bool])` | `Matrix` |

### Constants — Process Flags

| Constant | Description |
|----------|-------------|
| `CalcTangentSpace` | Calculate tangent space vectors. |
| `JoinIdenticalVertices` | Join identical vertices / optimise indexing. |
| `MakeLeftHanded` | Convert to left-handed coordinate system. |
| `Triangulate` | Triangulate all faces. |
| `RemoveComponent` | Remove specific components from meshes. |
| `GenNormals` | Generate flat normals. |
| `GenSmoothNormals` | Generate smooth normals. |
| `SplitLargeMeshes` | Split large meshes into sub-meshes. |
| `PreTransformVertices` | Pre-transform vertices by node transforms. |
| `LimitBoneWeights` | Limit bone weights per vertex. |
| `ValidateDataStructure` | Validate the imported data. |
| `ImproveCacheLocality` | Improve vertex-cache locality. |
| `RemoveRedundantMaterials` | Remove redundant materials. |
| `FixInfacingNormals` | Fix normals pointing inward. |
| `PopulateArmatureData` | Populate armature / bone data. |
| `SortByPType` | Sort primitives by type. |
| `FindDegenerates` | Find degenerate primitives. |
| `FindInvalidData` | Find invalid data in the scene. |
| `GenUVCoords` | Generate UV coordinates. |
| `TransformUVCoords` | Transform UV coordinates. |
| `FindInstances` | Find instanced meshes. |
| `OptimizeMeshes` | Optimise mesh draw calls. |
| `OptimizeGraph` | Optimise scene graph. |
| `FlipUVs` | Flip UV coordinates vertically. |
| `FlipWindingOrder` | Flip winding order of faces. |
| `SplitByBoneCount` | Split by bone count. |
| `Debone` | Remove bones. |
| `GlobalScale` | Apply global scale. |
| `EmbedTextures` | Embed textures in the file. |
| `ForceGenNormals` | Force normal generation. |
| `DropNormals` | Drop existing normals. |
| `GenBoundingBoxes` | Generate bounding boxes. |

### Constants — Process Presets

| Constant | Description |
|----------|-------------|
| `Preset_Fast` | Fast real-time target. |
| `Preset_Quality` | Quality real-time target. |
| `Preset_MaxQuality` | Maximum quality real-time target. |
| `Preset_D3D` | Convert to left-handed (Direct3D). |

### Constants — Texture Types

| Constant | Description |
|----------|-------------|
| `TextureNone` | No texture. |
| `TextureDiffuse` | Diffuse / albedo. |
| `TextureSpecular` | Specular map. |
| `TextureAmbient` | Ambient map. |
| `TextureEmissive` | Emissive map. |
| `TextureHeight` | Height map. |
| `TextureNormals` | Normal map. |
| `TextureShininess` | Shininess / gloss map. |
| `TextureOpacity` | Opacity / alpha map. |
| `TextureDisplacement` | Displacement map. |
| `TextureLightmap` | Lightmap / AO bake. |
| `TextureReflection` | Reflection map. |
| `TextureBaseColor` | PBR base colour. |
| `TextureNormalCamera` | PBR camera-space normal. |
| `TextureEmissionColor` | PBR emission colour. |
| `TextureMetalness` | PBR metalness. |
| `TextureDiffuseRoughness` | PBR roughness. |
| `TextureAmbientOcclusion` | PBR ambient occlusion. |
| `TextureUnknown` | Unknown texture type. |

### Constants — Animation Behaviour

| Constant | Description |
|----------|-------------|
| `AnimDefault` | Default behaviour. |
| `AnimConstant` | Constant / hold last key. |
| `AnimLinear` | Linear extrapolation. |
| `AnimRepeat` | Repeat animation. |

---

## BuBox2d — Module `Box2D`

2D rigid-body physics via Box2D 2.x.

```bu
require "BuBox2d";
import Box2D;
```

### Classes

#### `B2World`

Physics world — creates bodies, runs simulation steps, performs queries.

| Method | Signature | Description |
|--------|-----------|-------------|
| `destroy` | `destroy()` | Destroy the world. |
| `isValid` | `isValid()` | `bool` — world still alive? |
| `step` | `step(dt, velIter, posIter)` | Advance the simulation. |
| `createBody` | `createBody(bodyDef)` | → `B2Body` |
| `setDebugDraw` | `setDebugDraw(debugDraw)` | Attach a debug drawer. |
| `debugDraw` | `debugDraw()` | Trigger debug rendering. |
| `setContactListener` | `setContactListener(listener)` | Attach a contact listener. |
| `queryAABB` | `queryAABB(callback, lx, ly, ux, uy)` | AABB query. |
| `rayCast` | `rayCast(callback, x1, y1, x2, y2)` | Ray-cast query. |

#### `B2Body`

A rigid body inside a `B2World`.

| Method | Signature | Description |
|--------|-----------|-------------|
| `destroy` | `destroy()` | Remove body from world. |
| `isValid` | `isValid()` | `bool` |
| `createFixture` | `createFixture(fixtureDef)` | Attach a shape. |
| `getPosition` | `getPosition()` | → `x, y` |
| `getAngle` | `getAngle()` | → `radians` |
| `setLinearVelocity` | `setLinearVelocity(vx, vy)` | Set velocity. |
| `getLinearVelocity` | `getLinearVelocity()` | → `vx, vy` |
| `applyForceToCenter` | `applyForceToCenter(fx, fy, wake)` | Apply force. |
| `applyLinearImpulseToCenter` | `applyLinearImpulseToCenter(ix, iy, wake)` | Apply impulse. |
| `setTransform` | `setTransform(x, y, angle)` | Teleport body. |
| `setType` | `setType(type)` | Change body type. |
| `setAngularVelocity` | `setAngularVelocity(omega)` | Set angular velocity. |
| `setFixedRotation` | `setFixedRotation(flag)` | Lock rotation. |
| `setAwake` | `setAwake(flag)` | Wake / sleep body. |
| `getMass` | `getMass()` | → `float` |
| `setUserId` | `setUserId(id)` | Store user int. |
| `getUserId` | `getUserId()` | → `int` |
| `setTag` | `setTag(tag)` | Store user string. |
| `getTag` | `getTag()` | → `string` |
| `setFlags` | `setFlags(bits)` | Set full bitmask. |
| `getFlags` | `getFlags()` | → `int` |
| `setFlag` | `setFlag(bit)` | Set single flag. |
| `clearFlag` | `clearFlag(bit)` | Clear single flag. |
| `hasFlag` | `hasFlag(bit)` | → `bool` |

#### `B2BodyDef`

Configuration passed to `B2World.createBody()`.

| Method | Signature |
|--------|-----------|
| `setType` | `setType(type: int)` |
| `setPosition` | `setPosition(x, y)` |
| `setAngle` | `setAngle(radians)` |
| `setFixedRotation` | `setFixedRotation(flag)` |
| `setBullet` | `setBullet(flag)` |
| `setAwake` | `setAwake(flag)` |
| `setGravityScale` | `setGravityScale(scale)` |

#### `B2PolygonShape`

| Method | Signature |
|--------|-----------|
| `setAsBox` | `setAsBox(halfW, halfH)` |

#### `B2CircleShape`

| Method | Signature |
|--------|-----------|
| `setRadius` | `setRadius(r)` |
| `setPosition` | `setPosition(x, y)` |

#### `B2EdgeShape`

| Method | Signature |
|--------|-----------|
| `setTwoSided` | `setTwoSided(x1, y1, x2, y2)` |
| `setOneSided` | `setOneSided(v0x, v0y, v1x, v1y, v2x, v2y, v3x, v3y)` |

#### `B2ChainShape`

| Method | Signature |
|--------|-----------|
| `clear` | `clear()` |
| `createLoop` | `createLoop(vertArray)` |
| `createChain` | `createChain(vertArray, prevX, prevY, nextX, nextY)` |

#### `B2FixtureDef`

Configuration for attaching shapes to bodies.

| Method | Signature |
|--------|-----------|
| `setShape` | `setShape(shape)` |
| `setDensity` | `setDensity(d)` |
| `setFriction` | `setFriction(f)` |
| `setRestitution` | `setRestitution(r)` |
| `setSensor` | `setSensor(flag)` |
| `setCategoryBits` | `setCategoryBits(bits)` |
| `setMaskBits` | `setMaskBits(bits)` |
| `setGroupIndex` | `setGroupIndex(index)` |

#### `B2DebugDraw`

Attach to a world to visualise physics shapes. Override callback methods in a subclass.

| Method | Signature |
|--------|-----------|
| `setFlags` | `setFlags(bits)` |
| `getFlags` | `getFlags()` |
| `appendFlags` | `appendFlags(bits)` |
| `clearFlags` | `clearFlags(bits)` |
| `setLineWidth` / `getLineWidth` | Line thickness. |
| `setFillAlpha` / `getFillAlpha` | Fill opacity. |
| `setAxisScale` / `getAxisScale` | Axis size. |
| `setPointSize` / `getPointSize` | Point size. |

**Script callbacks** (override in subclass):

| Callback | Args |
|----------|------|
| `DrawPolygon` | `(vertices, r, g, b)` |
| `DrawSolidPolygon` | `(vertices, r, g, b)` |
| `DrawCircle` | `(cx, cy, radius, r, g, b)` |
| `DrawSolidCircle` | `(cx, cy, radius, ax, ay, r, g, b)` |
| `DrawSegment` | `(x1, y1, x2, y2, r, g, b)` |
| `DrawTransform` | `(px, py, sin, cos)` |
| `DrawPoint` | `(px, py, size, r, g, b)` |

#### `B2ContactListener`

Override in subclass to receive contact events.

| Callback | Args |
|----------|------|
| `BeginContact` | `(bodyA, bodyB)` |
| `EndContact` | `(bodyA, bodyB)` |
| `PreSolve` | `(bodyA, bodyB)` |
| `PostSolve` | `(bodyA, bodyB)` |

#### `B2QueryCallback`

Override `ReportFixture(body)` → return `true` to continue, `false` to stop.

#### `B2RayCastCallback`

Override `ReportFixture(body, px, py, nx, ny, fraction)` → return fraction / `true` / `false`.

### Joints

#### `MouseJointDef` → `MouseJoint`

| Def Methods | Joint Methods |
|-------------|---------------|
| `set_body_a`, `set_body_b`, `initialize` | `set_target`, `target` / `get_target` |
| `set_target`, `set_max_force`, `set_force` | `set_max_force`, `set_force` |
| `set_stiffness`, `set_damping` | `set_stiffness`, `set_damping` |
| `set_collide_connected` | `destroy`, `exists` |

#### `RevoluteJointDef` → `RevoluteJoint`

| Def Methods | Joint Methods |
|-------------|---------------|
| `set_body_a`, `set_body_b`, `initialize` | `enable_limit`, `set_limits` |
| `set_local_anchor_a`, `set_local_anchor_b` | `enable_motor`, `set_motor_speed` |
| `set_reference_angle` | `set_max_motor_torque` |
| `set_enable_limit`, `set_limits` | `get_joint_angle`, `get_joint_speed` |
| `set_enable_motor`, `set_motor_speed` | `get_motor_torque` |
| `set_max_motor_torque`, `set_collide_connected` | `get_anchor_a`, `get_anchor_b`, `destroy`, `exists` |

#### `WheelJointDef` → `WheelJoint`

| Def Methods | Joint Methods |
|-------------|---------------|
| `set_body_a`, `set_body_b`, `initialize` | `enable_motor`, `set_max_motor_torque` |
| `set_local_anchor_a/b`, `set_local_axis_a` | `set_motor_speed`, `set_stiffness`, `set_damping` |
| `set_enable_motor`, `set_max_motor_torque` | `get_motor_speed`, `get_joint_translation` |
| `set_motor_speed`, `set_stiffness`, `set_damping` | `get_joint_linear_speed`, `get_motor_torque` |
| `set_collide_connected` | `get_anchor_a`, `get_anchor_b`, `destroy`, `exists` |

#### `DistanceJointDef` → `DistanceJoint`

| Def Methods | Joint Methods |
|-------------|---------------|
| `set_body_a`, `set_body_b`, `initialize` | `set_length`, `set_min_length`, `set_max_length` |
| `set_local_anchor_a/b` | `set_stiffness`, `set_damping` |
| `set_length`, `set_min_length`, `set_max_length` | `get_length`, `get_current_length` |
| `set_stiffness`, `set_damping`, `set_collide_connected` | `get_anchor_a`, `get_anchor_b`, `destroy`, `exists` |

#### `PrismaticJointDef` → `PrismaticJoint`

| Def Methods | Joint Methods |
|-------------|---------------|
| `set_body_a`, `set_body_b`, `initialize` | `enable_limit`, `set_limits` |
| `set_local_anchor_a/b`, `set_local_axis_a` | `enable_motor`, `set_motor_speed` |
| `set_reference_angle` | `set_max_motor_force` |
| `set_enable_limit`, `set_limits` | `get_joint_translation`, `get_joint_speed` |
| `set_enable_motor`, `set_motor_speed` | `get_motor_force` |
| `set_max_motor_force`, `set_collide_connected` | `get_anchor_a`, `get_anchor_b`, `destroy`, `exists` |

#### `PulleyJointDef` → `PulleyJoint`

| Def Methods | Joint Methods |
|-------------|---------------|
| `set_body_a`, `set_body_b`, `initialize` | `get_ratio` |
| `set_ground_anchor_a/b`, `set_local_anchor_a/b` | `get_length_a`, `get_length_b` |
| `set_length_a`, `set_length_b` | `get_current_length_a`, `get_current_length_b` |
| `set_ratio`, `set_collide_connected` | `get_anchor_a/b`, `get_ground_anchor_a/b`, `destroy`, `exists` |

#### `FrictionJointDef` → `FrictionJoint`

| Def Methods | Joint Methods |
|-------------|---------------|
| `set_body_a`, `set_body_b`, `initialize` | `set_max_force`, `get_max_force` |
| `set_local_anchor_a/b` | `set_max_torque`, `get_max_torque` |
| `set_max_force`, `set_max_torque`, `set_collide_connected` | `get_anchor_a`, `get_anchor_b`, `destroy`, `exists` |

#### `GearJointDef` → `GearJoint`

| Def Methods | Joint Methods |
|-------------|---------------|
| `set_body_a`, `set_body_b` | `set_ratio`, `get_ratio` |
| `set_joint1`, `set_joint2` | `get_anchor_a`, `get_anchor_b` |
| `set_ratio`, `set_collide_connected` | `destroy`, `exists` |

#### `MotorJointDef` → `MotorJoint`

| Def Methods | Joint Methods |
|-------------|---------------|
| `set_body_a`, `set_body_b`, `initialize` | `set_linear_offset`, `get_linear_offset` |
| `set_linear_offset`, `set_angular_offset` | `set_angular_offset`, `get_angular_offset` |
| `set_max_force`, `set_max_torque` | `set_max_force`, `get_max_force` |
| `set_correction_factor`, `set_collide_connected` | `set_max_torque`, `get_max_torque` |
| | `set_correction_factor`, `get_correction_factor` |
| | `get_anchor_a`, `get_anchor_b`, `destroy`, `exists` |

#### `b2RopeTuning`

| Method | Description |
|--------|-------------|
| `set_stretching_model` | Set stretching model. |
| `set_bending_model` | Set bending model. |
| `set_damping` | Set damping. |
| `set_stretch_stiffness` | Set stretch stiffness. |
| `set_stretch_hertz` | Set stretch hertz. |
| `set_stretch_damping` | Set stretch damping. |
| `set_bend_stiffness` | Set bend stiffness. |
| `set_bend_hertz` | Set bend hertz. |
| `set_bend_damping` | Set bend damping. |
| `set_isometric` | Toggle isometric mode. |
| `set_fixed_effective_mass` | Toggle fixed effective mass. |
| `set_warm_start` | Toggle warm start. |

#### `b2RopeDef`

| Method | Description |
|--------|-------------|
| `set_position` | `(x, y)` |
| `set_gravity` | `(gx, gy)` |
| `set_tuning` | `(b2RopeTuning)` |
| `set_vertices` | `(array)` — set flat vertex array `[x0, y0, x1, y1, …]` |
| `clear_vertices` | Clear vertices. |
| *(plus all tuning methods directly)* | |

#### `b2Rope`

| Method | Signature |
|--------|-----------|
| `create` | `create(ropeDef)` |
| `set_tuning` | `set_tuning(tuning)` |
| `step` | `step(dt, iterations)` |
| `reset` | `reset(position)` |
| `get_count` | `get_count()` → `int` |
| `get_point` | `get_point(index)` → `x, y` |

### Constants

| Constant | Description |
|----------|-------------|
| `B2_STATIC_BODY` | Static body type. |
| `B2_KINEMATIC_BODY` | Kinematic body type. |
| `B2_DYNAMIC_BODY` | Dynamic body type. |
| `B2_DRAW_SHAPE_BIT` | Debug draw flag — shapes. |
| `B2_DRAW_JOINT_BIT` | Debug draw flag — joints. |
| `B2_DRAW_AABB_BIT` | Debug draw flag — AABBs. |
| `B2_DRAW_PAIR_BIT` | Debug draw flag — pairs. |
| `B2_DRAW_CENTER_OF_MASS_BIT` | Debug draw flag — centre of mass. |

### Globals — Rope Models

| Global | Description |
|--------|-------------|
| `b2_pbdStretchingModel` | PBD stretching. |
| `b2_xpbdStretchingModel` | XPBD stretching. |
| `b2_springAngleBendingModel` | Spring angle bending. |
| `b2_pbdAngleBendingModel` | PBD angle bending. |
| `b2_xpbdAngleBendingModel` | XPBD angle bending. |
| `b2_pbdDistanceBendingModel` | PBD distance bending. |
| `b2_pbdHeightBendingModel` | PBD height bending. |
| `b2_pbdTriangleBendingModel` | PBD triangle bending. |

---

## BuChipmunk — Module `Chip2D`

2D rigid-body physics via Chipmunk2D. Uses a functional (handle-based) API.

```bu
require "BuChipmunk";
import Chip2D;
```

### Handle Classes

These are opaque handles — not directly constructible. Use factory functions.

| Class | Description |
|-------|-------------|
| `cpSpace` | Physics world. |
| `cpBody` | Rigid body. |
| `cpShape` | Collision shape. |
| `cpConstraint` | Joint / constraint. |

### Functions — Utility

| Function | Arity | Description |
|----------|-------|-------------|
| `cpMomentForCircle` | 4 | `(mass, r1, r2, offset)` → moment. |
| `cpAreaForCircle` | 2 | `(r1, r2)` → area. |
| `cpMomentForSegment` | 4 | `(mass, a, b, radius)` → moment. |
| `cpAreaForSegment` | 3 | `(a, b, radius)` → area. |
| `cpMomentForPoly` | 4 | `(mass, verts, offset, radius)` → moment. |
| `cpAreaForPoly` | 2 | `(verts, radius)` → area. |
| `cpCentroidForPoly` | 1 | `(verts)` → `x, y`. |
| `cpMomentForBox` | 3 | `(mass, width, height)` → moment. |

### Functions — Space

| Function | Arity | Description |
|----------|-------|-------------|
| `cpSpaceNew` | 0 | Create a new space. |
| `cpSpaceFree` | 1 | Destroy a space. |
| `cpSpaceStep` | 2 | `(space, dt)` — step simulation. |
| `cpSpaceIsLocked` | 1 | `(space)` → `bool`. |
| `cpSpaceGetIterations` | 1 | `(space)` → `int`. |
| `cpSpaceSetIterations` | 2 | `(space, iters)`. |
| `cpSpaceGetGravity` | 1 | `(space)` → `x, y`. |
| `cpSpaceSetGravity` | 2 | `(space, Vector2)`. |
| `cpSpaceGetDamping` | 1 | `(space)` → `float`. |
| `cpSpaceSetDamping` | 2 | `(space, damping)`. |
| `cpSpaceGetStaticBody` | 1 | `(space)` → `cpBody`. |
| `cpSpaceAddBody` | 2 | `(space, body)`. |
| `cpSpaceRemoveBody` | 2 | `(space, body)`. |
| `cpSpaceContainsBody` | 2 | `(space, body)` → `bool`. |
| `cpSpaceAddShape` | 2 | `(space, shape)`. |
| `cpSpaceRemoveShape` | 2 | `(space, shape)`. |
| `cpSpaceContainsShape` | 2 | `(space, shape)` → `bool`. |
| `cpSpaceAddConstraint` | 2 | `(space, constraint)`. |
| `cpSpaceRemoveConstraint` | 2 | `(space, constraint)`. |
| `cpSpaceContainsConstraint` | 2 | `(space, constraint)` → `bool`. |

### Functions — Body

| Function | Arity | Description |
|----------|-------|-------------|
| `cpBodyNew` | 2 | `(mass, moment)` → `cpBody`. |
| `cpBodyNewKinematic` | 0 | → `cpBody`. |
| `cpBodyNewStatic` | 0 | → `cpBody`. |
| `cpBodyFree` | 1 | `(body)`. |
| `cpBodyGetSpace` | 1 | `(body)` → `cpSpace`. |
| `cpBodyGetType` | 1 | `(body)` → `int`. |
| `cpBodySetType` | 2 | `(body, type)`. |
| `cpBodyGetMass` | 1 | `(body)` → `float`. |
| `cpBodySetMass` | 2 | `(body, mass)`. |
| `cpBodyGetMoment` | 1 | `(body)` → `float`. |
| `cpBodySetMoment` | 2 | `(body, moment)`. |
| `cpBodyGetPosition` | 1 | `(body)` → `x, y`. |
| `cpBodySetPosition` | 2 | `(body, Vector2)`. |
| `cpBodyGetVelocity` | 1 | `(body)` → `x, y`. |
| `cpBodySetVelocity` | 2 | `(body, Vector2)`. |
| `cpBodyGetAngle` | 1 | `(body)` → `float`. |
| `cpBodySetAngle` | 2 | `(body, radians)`. |
| `cpBodyGetAngularVelocity` | 1 | `(body)` → `float`. |
| `cpBodySetAngularVelocity` | 2 | `(body, omega)`. |
| `cpBodyGetRotation` | 1 | `(body)` → `cos, sin`. |
| `cpBodyApplyForceAtWorldPoint` | 3 | `(body, force, point)`. |
| `cpBodyApplyImpulseAtWorldPoint` | 3 | `(body, impulse, point)`. |

### Functions — Shape

| Function | Arity | Description |
|----------|-------|-------------|
| `cpCircleShapeNew` | 3 | `(body, radius, offset)` → `cpShape`. |
| `cpSegmentShapeNew` | 4 | `(body, a, b, radius)` → `cpShape`. |
| `cpPolyShapeNewRaw` | 3 | `(body, verts, radius)` → `cpShape`. |
| `cpBoxShapeNew` | 4 | `(body, width, height, radius)` → `cpShape`. |
| `cpShapeFree` | 1 | `(shape)`. |
| `cpShapeGetSpace` | 1 | `(shape)` → `cpSpace`. |
| `cpShapeGetBody` | 1 | `(shape)` → `cpBody`. |
| `cpShapeGetSensor` / `cpShapeSetSensor` | 1 / 2 | Sensor flag. |
| `cpShapeGetElasticity` / `cpShapeSetElasticity` | 1 / 2 | Elasticity. |
| `cpShapeGetFriction` / `cpShapeSetFriction` | 1 / 2 | Friction. |
| `cpShapeGetCollisionType` / `cpShapeSetCollisionType` | 1 / 2 | Collision type. |

### Functions — Constraints

Factory functions create constraints, get/set functions configure them.

**Common:**

| Function | Arity | Description |
|----------|-------|-------------|
| `cpConstraintFree` | 1 | `(constraint)`. |
| `cpConstraintGetSpace` | 1 | `(constraint)` → `cpSpace`. |
| `cpConstraintGetBodyA` / `GetBodyB` | 1 | → `cpBody`. |
| `cpConstraintGetMaxForce` / `SetMaxForce` | 1 / 2 | Max force. |
| `cpConstraintGetErrorBias` / `SetErrorBias` | 1 / 2 | Error bias. |
| `cpConstraintGetMaxBias` / `SetMaxBias` | 1 / 2 | Max bias. |
| `cpConstraintGetCollideBodies` / `SetCollideBodies` | 1 / 2 | Collide flag. |
| `cpConstraintGetImpulse` | 1 | → `float`. |
| `cpConstraintIs*` | 1 | Type-check (PinJoint, SlideJoint, PivotJoint, GrooveJoint, DampedSpring, DampedRotarySpring, RotaryLimitJoint, RatchetJoint, GearJoint, SimpleMotor). |

**Pin Joint:** `cpPinJointNew(bodyA, bodyB, anchorA, anchorB)`, get/set `AnchorA`, `AnchorB`, `Dist`.

**Slide Joint:** `cpSlideJointNew(bodyA, bodyB, anchorA, anchorB, min, max)`, get/set `AnchorA`, `AnchorB`, `Min`, `Max`.

**Pivot Joint:** `cpPivotJointNew(bodyA, bodyB, pivot)`, `cpPivotJointNew2(bodyA, bodyB, anchorA, anchorB)`, get/set `AnchorA`, `AnchorB`.

**Groove Joint:** `cpGrooveJointNew(bodyA, bodyB, grooveA, grooveB, anchorB)`, get/set `GrooveA`, `GrooveB`, `AnchorB`.

**Damped Spring:** `cpDampedSpringNew(bodyA, bodyB, anchorA, anchorB, restLength, stiffness, damping)`, get/set `AnchorA`, `AnchorB`, `RestLength`, `Stiffness`, `Damping`.

**Damped Rotary Spring:** `cpDampedRotarySpringNew(bodyA, bodyB, restAngle, stiffness, damping)`, get/set `RestAngle`, `Stiffness`, `Damping`.

**Rotary Limit Joint:** `cpRotaryLimitJointNew(bodyA, bodyB, min, max)`, get/set `Min`, `Max`.

**Ratchet Joint:** `cpRatchetJointNew(bodyA, bodyB, phase, ratchet)`, get/set `Angle`, `Phase`, `Ratchet`.

**Gear Joint:** `cpGearJointNew(bodyA, bodyB, phase, ratio)`, get/set `Phase`, `Ratio`.

**Simple Motor:** `cpSimpleMotorNew(bodyA, bodyB, rate)`, get/set `Rate`.

### Constants

| Constant | Description |
|----------|-------------|
| `cpVersionString` | Chipmunk version string. |
| `CP_BODY_TYPE_DYNAMIC` | Dynamic body. |
| `CP_BODY_TYPE_KINEMATIC` | Kinematic body. |
| `CP_BODY_TYPE_STATIC` | Static body. |

---

## BuJolt — Module `Jolt`

3D rigid-body physics via Jolt Physics.

```bu
require "BuJolt";
import Jolt;
```

### Classes

#### `JoltWorld`

3D physics world.

| Method | Signature | Description |
|--------|-----------|-------------|
| `destroy` | `destroy()` | Destroy the world. |
| `isValid` | `isValid()` | → `bool` |
| `step` | `step(dt, collisionSteps)` | Advance simulation. |
| `setGravity` | `setGravity(x, y, z)` | Set gravity vector. |
| `getGravity` | `getGravity()` | → `x, y, z` |
| `optimizeBroadPhase` | `optimizeBroadPhase()` | Optimise broad-phase tree. |
| `getBodyCount` | `getBodyCount()` | → `int` |
| `createStaticBox` | `createStaticBox(hx, hy, hz, px, py, pz)` | → `JoltBody` |
| `createStaticSphere` | `createStaticSphere(r, px, py, pz)` | → `JoltBody` |
| `createStaticCapsule` | `createStaticCapsule(halfH, r, px, py, pz)` | → `JoltBody` |
| `createBox` | `createBox(hx, hy, hz, px, py, pz, motionType)` | → `JoltBody` |
| `createSphere` | `createSphere(r, px, py, pz, motionType)` | → `JoltBody` |
| `createCapsule` | `createCapsule(halfH, r, px, py, pz, motionType)` | → `JoltBody` |
| `createOffsetBox` | `createOffsetBox(hx, hy, hz, ox, oy, oz, px, py, pz, motionType)` | → `JoltBody` (offset CoM) |
| `createPointConstraint` | `createPointConstraint(bodyA, bodyB, px, py, pz)` | → `JoltPointConstraint` |
| `createDistanceConstraint` | `createDistanceConstraint(bodyA, bodyB, p1, p2, min, max)` | → `JoltDistanceConstraint` |
| `createHingeConstraint` | `createHingeConstraint(bodyA, bodyB, point, axis)` | → `JoltHingeConstraint` |
| `createSliderConstraint` | `createSliderConstraint(bodyA, bodyB, axis)` | → `JoltSliderConstraint` |
| `createWheeledVehicle` | `createWheeledVehicle(body, wheelSettings[])` | → `JoltWheeledVehicle` |
| `createMotorcycle` | `createMotorcycle(body, wheelSettings[])` | → `JoltMotorcycle` |
| `createTrackedVehicle` | `createTrackedVehicle(body, wheelSettings[])` | → `JoltTrackedVehicle` |

#### `JoltBody`

A 3D rigid body.

| Method | Signature | Description |
|--------|-----------|-------------|
| `destroy` | `destroy()` | Remove body. |
| `isValid` | `isValid()` | → `bool` |
| `isActive` | `isActive()` | → `bool` |
| `activate` | `activate()` | Wake up. |
| `deactivate` | `deactivate()` | Put to sleep. |
| `getPosition` | `getPosition()` | → `x, y, z` |
| `getRotation` | `getRotation()` | → `x, y, z, w` (quaternion) |
| `getMotionType` | `getMotionType()` | → `int` |
| `getLinearVelocity` | `getLinearVelocity()` | → `vx, vy, vz` |
| `getAngularVelocity` | `getAngularVelocity()` | → `vx, vy, vz` |
| `setPosition` | `setPosition(x, y, z)` | Teleport. |
| `setLinearVelocity` | `setLinearVelocity(vx, vy, vz)` | Set velocity. |
| `setAngularVelocity` | `setAngularVelocity(vx, vy, vz)` | Set angular velocity. |
| `setMotionType` | `setMotionType(type)` | Change motion type. |
| `setRotation` | `setRotation(x, y, z, w)` | Set quaternion. |
| `setPositionAndRotation` | `setPositionAndRotation(px, py, pz, rx, ry, rz, rw)` | Combined set. |
| `moveKinematic` | `moveKinematic(px, py, pz, rx, ry, rz, rw, dt)` | Kinematic move. |
| `addForce` | `addForce(fx, fy, fz)` | Apply force. |
| `addForceAtPosition` | `addForceAtPosition(fx, fy, fz, px, py, pz)` | Force at point. |
| `addTorque` | `addTorque(tx, ty, tz)` | Apply torque. |
| `addImpulse` | `addImpulse(ix, iy, iz)` | Apply impulse. |
| `addImpulseAtPosition` | `addImpulseAtPosition(ix, iy, iz, px, py, pz)` | Impulse at point. |
| `addAngularImpulse` | `addAngularImpulse(ix, iy, iz)` | Angular impulse. |
| `setFriction` | `setFriction(f)` | Set friction. |
| `setRestitution` | `setRestitution(r)` | Set restitution. |

#### `JoltPointConstraint`

| Method | Description |
|--------|-------------|
| `destroy`, `isValid`, `getEnabled`, `setEnabled` | Common constraint ops. |
| `getUserData`, `setUserData`, `getSubType` | Metadata. |
| `setPoint1(x, y, z)`, `setPoint2(x, y, z)` | Set anchor points. |

#### `JoltDistanceConstraint`

| Method | Description |
|--------|-------------|
| Common constraint methods | See above. |
| `setDistance(min, max)` | Set distance limits. |
| `getMinDistance()`, `getMaxDistance()` | Query limits. |

#### `JoltHingeConstraint`

| Method | Description |
|--------|-------------|
| Common constraint methods | See above. |
| `getCurrentAngle()` | → `float` |
| `setLimits(min, max)`, `getLimitsMin()`, `getLimitsMax()` | Angle limits. |
| `setMaxFrictionTorque(t)`, `getMaxFrictionTorque()` | Friction. |
| `setMotorState(state)`, `getMotorState()` | Motor control. |
| `setTargetAngularVelocity(v)`, `getTargetAngularVelocity()` | Velocity target. |
| `setTargetAngle(a)`, `getTargetAngle()` | Position target. |
| `setMotorTorqueLimit(t)`, `getMotorTorqueLimit()` | Torque limit. |

#### `JoltSliderConstraint`

| Method | Description |
|--------|-------------|
| Common constraint methods | See above. |
| `getCurrentPosition()` | → `float` |
| `setLimits(min, max)`, `getLimitsMin()`, `getLimitsMax()` | Position limits. |
| `setMaxFrictionForce(f)`, `getMaxFrictionForce()` | Friction. |
| `setMotorState(state)`, `getMotorState()` | Motor control. |
| `setTargetVelocity(v)`, `getTargetVelocity()` | Velocity target. |
| `setTargetPosition(p)`, `getTargetPosition()` | Position target. |
| `setMotorForceLimit(f)`, `getMotorForceLimit()` | Force limit. |

#### `JoltWheelSettingsWV`

Wheeled-vehicle wheel settings (constructible).

| Method | Description |
|--------|-------------|
| `setPosition(x, y, z)` | Wheel position. |
| `setSuspensionForcePoint(x, y, z)` | Suspension force point. |
| `setSuspensionDirection(x, y, z)` | Suspension direction. |
| `setSteeringAxis(x, y, z)` | Steering axis. |
| `setWheelUp(x, y, z)` | Wheel up vector. |
| `setWheelForward(x, y, z)` | Wheel forward vector. |
| `setSuspensionMinLength(v)` | Min suspension length. |
| `setSuspensionMaxLength(v)` | Max suspension length. |
| `setSuspensionPreloadLength(v)` | Preload length. |
| `setSuspensionFrequency(v)` | Spring frequency. |
| `setSuspensionDamping(v)` | Damping ratio. |
| `setRadius(v)` / `getRadius()` | Wheel radius. |
| `setWidth(v)` | Wheel width. |
| `setEnableSuspensionForcePoint(b)` | Enable custom force point. |
| `setInertia(v)` | Wheel inertia. |
| `setAngularDamping(v)` | Angular damping. |
| `setMaxSteerAngle(v)` | Max steering angle. |
| `setMaxBrakeTorque(v)` | Max brake torque. |
| `setMaxHandBrakeTorque(v)` | Max handbrake torque. |

#### `JoltWheelSettingsTV`

Tracked-vehicle wheel settings (constructible).

| Method | Description |
|--------|-------------|
| `setPosition(x, y, z)` | Wheel position. |
| `setSuspensionDirection(x, y, z)` | Suspension direction. |
| `setSuspensionMinLength(v)` / `MaxLength` / `PreloadLength` | Suspension tuning. |
| `setSuspensionFrequency(v)` / `Damping` | Spring parameters. |
| `setRadius(v)` / `getRadius()` | Wheel radius. |
| `setWidth(v)` | Wheel width. |
| `setLongitudinalFriction(arr)` | Longitudinal friction curve. |
| `setLateralFriction(arr)` | Lateral friction curve. |

#### `JoltWheeledVehicle`

| Method | Description |
|--------|-------------|
| `destroy`, `isValid` | Lifecycle. |
| `setDriverInput(fwd, right, brake, handbrake)` | Set all inputs at once. |
| `setForwardInput(v)` / `setRightInput(v)` | Individual inputs. |
| `setBrakeInput(v)` / `setHandBrakeInput(v)` | Braking. |
| `getEngineRPM()`, `getCurrentGear()`, `getClutchFriction()` | Telemetry. |
| `setEngineMaxTorque(v)` | Engine torque. |
| `setEngineRPMRange(min, max)` | RPM range. |
| `setShiftRPM(up, down)` | Shift points. |
| `setClutchStrength(v)` | Clutch strength. |
| `setGearRatios(arr)` / `setReverseGearRatios(arr)` | Gear ratios. |
| `clearDifferentials()`, `addDifferential(...)` | Differentials. |
| `setTransmissionMode(mode)` / `getTransmissionMode()` | Auto / manual. |
| `setTransmission(...)` | Full transmission config. |
| `setDifferentialLimitedSlipRatio(v)` / `get...` | LSD ratio. |
| `getWheelCount()` | → `int` |
| `getWheelWorldPosition(i)` | → `x, y, z` |
| `getWheelWorldPose(i)` | → `px, py, pz, rx, ry, rz, rw` |
| `hasWheelContact(i)` | → `bool` |
| `getWheelContactPosition(i)` / `ContactNormal(i)` | Contact info. |
| `getWheelSuspensionLength(i)` | → `float` |
| `getWheelRotationAngle(i)` / `SteerAngle(i)` | Wheel state. |
| `setMaxPitchRollAngle(v)` / `getMaxPitchRollAngle()` | Stability. |
| `overrideGravity(x, y, z)` / `resetGravityOverride()` | Custom gravity. |
| `setNumStepsBetweenCollisionTestActive(n)` | Collision frequency (active). |
| `setNumStepsBetweenCollisionTestInactive(n)` | Collision frequency (inactive). |
| `setCollisionTesterRay(up)` | Use ray-cast for collision. |
| `setCollisionTesterSphere(up, maxAngle)` | Use sphere-cast. |
| `setCollisionTesterCylinder(up, maxAngle)` | Use cylinder-cast. |

#### `JoltMotorcycle`

Inherits all `JoltWheeledVehicle` methods plus:

| Method | Description |
|--------|-------------|
| `enableLeanController(flag)` | Enable / disable lean. |
| `isLeanControllerEnabled()` | → `bool` |
| `enableLeanSteeringLimit(flag)` | Lean-based steering limit. |
| `isLeanSteeringLimitEnabled()` | → `bool` |
| `setLeanSpringConstant(v)` / `getLeanSpringConstant()` | Spring constant. |

#### `JoltTrackedVehicle`

| Method | Description |
|--------|-------------|
| `destroy`, `isValid` | Lifecycle. |
| `setDriverInput(fwd, leftRatio, rightRatio, brake)` | Set all inputs. |
| `setForwardInput(v)` / `setLeftRatio(v)` / `setRightRatio(v)` | Individual. |
| `setBrakeInput(v)` | Braking. |
| `getEngineRPM()`, `getCurrentGear()` | Telemetry. |
| `setEngineMaxTorque(v)`, `setEngineRPMRange(min, max)` | Engine. |
| `setShiftRPM(up, down)` | Shift points. |
| Common wheel / collision methods | Same as `JoltWheeledVehicle`. |

### Constants

| Constant | Description |
|----------|-------------|
| `JOLT_STATIC` | Static motion type. |
| `JOLT_KINEMATIC` | Kinematic motion type. |
| `JOLT_DYNAMIC` | Dynamic motion type. |
| `JOLT_CONSTRAINT_LOCAL` | Local-to-body-COM constraint space. |
| `JOLT_CONSTRAINT_WORLD` | World-space constraint space. |
| `JOLT_MOTOR_OFF` | Motor off. |
| `JOLT_MOTOR_VELOCITY` | Velocity motor. |
| `JOLT_MOTOR_POSITION` | Position motor. |
| `JOLT_SUBTYPE_POINT` | Point constraint. |
| `JOLT_SUBTYPE_HINGE` | Hinge constraint. |
| `JOLT_SUBTYPE_DISTANCE` | Distance constraint. |
| `JOLT_SUBTYPE_SLIDER` | Slider constraint. |
| `JOLT_SUBTYPE_VEHICLE` | Vehicle constraint. |
| `JOLT_TRANSMISSION_AUTO` | Automatic transmission. |
| `JOLT_TRANSMISSION_MANUAL` | Manual transmission. |
| `JOLT_UPDATE_NONE` | No update error. |
| `JOLT_UPDATE_MANIFOLD_CACHE_FULL` | Manifold cache full. |
| `JOLT_UPDATE_BODY_PAIR_CACHE_FULL` | Body-pair cache full. |
| `JOLT_UPDATE_CONTACT_CONSTRAINTS_FULL` | Contact constraints full. |

---

## BuMicroPather — Module `MicroPather`

Grid-based A* pathfinding via MicroPather.

```bu
require "BuMicroPather";
import MicroPather;
```

### Classes

#### `GridPathfinder`

| Method | Signature | Returns |
|--------|-----------|---------|
| `destroy` | `destroy()` | — |
| `isValid` | `isValid()` | `bool` |
| `getWidth` | `getWidth()` | `int` |
| `getHeight` | `getHeight()` | `int` |
| `setDiagonal` | `setDiagonal(flag)` | — |
| `clear` | `clear()` | — |
| `resetCache` | `resetCache()` | — |
| `inBounds` | `inBounds(x, y)` | `bool` |
| `setBlocked` | `setBlocked(x, y, flag)` | — |
| `isBlocked` | `isBlocked(x, y)` | `bool` |
| `setRectBlocked` | `setRectBlocked(x, y, w, h, flag)` | — |
| `solve` | `solve(startX, startY, endX, endY)` | `Array` (path) |
| `getLastStatus` | `getLastStatus()` | `int` |
| `getLastCost` | `getLastCost()` | `float` |

### Constants

| Constant | Description |
|----------|-------------|
| `SOLVED` | Path found. |
| `NO_SOLUTION` | No path exists. |
| `START_END_SAME` | Start equals end. |

---

## BuOde — Module `ODE`

3D rigid-body physics via Open Dynamics Engine. Purely functional API with opaque integer handles.

```bu
require "BuOde";
import ODE;
```

### Functions — Init / Shutdown

| Function | Arity | Description |
|----------|-------|-------------|
| `dInitODE2` | 0..1 | Initialise ODE. |
| `dCloseODE` | 0 | Shutdown ODE. |

### Functions — World

| Function | Arity | Description |
|----------|-------|-------------|
| `dWorldCreate` | 0 | → world handle. |
| `dWorldDestroy` | 1 | `(world)`. |
| `dWorldSetGravity` | 4 | `(world, gx, gy, gz)`. |
| `dWorldGetGravity` | 1 | `(world)` → `gx, gy, gz`. |
| `dWorldSetERP` | 2 | `(world, erp)`. |
| `dWorldGetERP` | 1 | `(world)` → `float`. |
| `dWorldSetCFM` | 2 | `(world, cfm)`. |
| `dWorldGetCFM` | 1 | `(world)` → `float`. |
| `dWorldSetQuickStepNumIterations` | 2 | `(world, iters)`. |
| `dWorldGetQuickStepNumIterations` | 1 | `(world)` → `int`. |
| `dWorldStep` | 2 | `(world, dt)`. |
| `dWorldQuickStep` | 2 | `(world, dt)`. |

### Functions — Space

| Function | Arity | Description |
|----------|-------|-------------|
| `dSimpleSpaceCreate` | 1 | `(parent)` → space handle. |
| `dHashSpaceCreate` | 1 | `(parent)` → space handle. |
| `dSpaceDestroy` | 1 | `(space)`. |
| `dSpaceSetSublevel` / `GetSublevel` | 2 / 1 | Sub-level. |
| `dSpaceSetCleanup` / `GetCleanup` | 2 / 1 | Auto-cleanup flag. |
| `dSpaceAdd` | 2 | `(space, geom)`. |
| `dSpaceRemove` | 2 | `(space, geom)`. |
| `dSpaceCollideSimple` | var | Collide with contact buffer. |
| `dContactEventsClear` | 0 | Clear contact event buffer. |
| `dContactEventsCount` | 0 | → `int`. |
| `dContactEventGet` | 1 | `(index)` → contact data. |

### Functions — Body

| Function | Arity | Description |
|----------|-------|-------------|
| `dBodyCreate` | 1 | `(world)` → body handle. |
| `dBodyDestroy` | 1 | `(body)`. |
| `dBodySetPosition` | 4 | `(body, x, y, z)`. |
| `dBodyGetPosition` | 1 | `(body)` → `x, y, z`. |
| `dBodyGetRotation` | 1 | `(body)` → 12 floats (3×4 matrix). |
| `dBodySetLinearVel` | 4 | `(body, vx, vy, vz)`. |
| `dBodyGetLinearVel` | 1 | `(body)` → `vx, vy, vz`. |
| `dBodySetAngularVel` | 4 | `(body, vx, vy, vz)`. |
| `dBodyGetAngularVel` | 1 | `(body)` → `vx, vy, vz`. |
| `dBodyAddForce` | 4 | `(body, fx, fy, fz)`. |
| `dBodyAddTorque` | 4 | `(body, tx, ty, tz)`. |
| `dBodySetKinematic` | 1 | `(body)`. |
| `dBodySetDynamic` | 1 | `(body)`. |
| `dBodyIsKinematic` | 1 | `(body)` → `bool`. |
| `dBodySetAutoDisableFlag` / `Get` | 2 / 1 | Auto-disable. |
| `dBodySetGravityMode` / `Get` | 2 / 1 | Per-body gravity. |
| `dBodySetMassBoxTotal` | 5 | `(body, totalMass, lx, ly, lz)`. |
| `dBodySetMassSphereTotal` | 3 | `(body, totalMass, radius)`. |
| `dBodySetMassCapsuleTotal` | 5 | `(body, totalMass, dir, radius, length)`. |
| `dBodySetMassCylinderTotal` | 5 | `(body, totalMass, dir, radius, length)`. |

### Functions — Geometry

| Function | Arity | Description |
|----------|-------|-------------|
| `dCreatePlane` | 5 | `(space, a, b, c, d)` → geom. |
| `dCreateBox` | 4 | `(space, lx, ly, lz)` → geom. |
| `dCreateSphere` | 2 | `(space, radius)` → geom. |
| `dCreateCapsule` | 3 | `(space, radius, length)` → geom. |
| `dCreateCylinder` | 3 | `(space, radius, length)` → geom. |
| `dCreateRay` | 2 | `(space, length)` → geom. |
| `dGeomDestroy` | 1 | `(geom)`. |
| `dGeomSetBody` | 2 | `(geom, body)`. |
| `dGeomGetBody` | 1 | `(geom)` → body. |
| `dGeomSetPosition` | 4 | `(geom, x, y, z)`. |
| `dGeomGetPosition` | 1 | `(geom)` → `x, y, z`. |
| `dGeomGetClass` | 1 | `(geom)` → `int`. |
| `dGeomSetCategoryBits` / `Get` | 2 / 1 | Category bits. |
| `dGeomSetCollideBits` / `Get` | 2 / 1 | Collide bits. |
| `dGeomEnable` / `dGeomDisable` / `dGeomIsEnabled` | 1 | Enable / disable / query. |

### Functions — Joint Groups

| Function | Arity | Description |
|----------|-------|-------------|
| `dJointGroupCreate` | 0..1 | → group handle. |
| `dJointGroupDestroy` | 1 | `(group)`. |
| `dJointGroupEmpty` | 1 | `(group)`. |

### Functions — Joints

**Creation:** `dJointCreateBall`, `dJointCreateHinge`, `dJointCreateSlider`, `dJointCreateHinge2`, `dJointCreateUniversal`, `dJointCreateFixed`, `dJointCreateNull`, `dJointCreateAMotor`, `dJointCreateLMotor`, `dJointCreatePR`, `dJointCreatePU`, `dJointCreatePiston`, `dJointCreatePlane2D` — all `(world, group)` → joint handle.

**Management:** `dJointDestroy(j)`, `dJointAttach(j, body1, body2)`, `dJointEnable(j)`, `dJointDisable(j)`, `dJointIsEnabled(j)`, `dJointGetType(j)`, `dJointGetBody(j, index)`, `dAreConnected(b1, b2)`, `dAreConnectedExcluding(b1, b2, type)`.

**Ball:** `dJointSetBallAnchor(j, x, y, z)`, `dJointGetBallAnchor(j)`, `dJointGetBallAnchor2(j)`, `dJointSetBallParam(j, param, val)`, `dJointGetBallParam(j, param)`.

**Hinge:** `dJointSetHingeAnchor(j, x, y, z)`, `dJointSetHingeAxis(j, x, y, z)`, `dJointGetHingeAnchor(j)`, `dJointGetHingeAnchor2(j)`, `dJointGetHingeAxis(j)`, `dJointGetHingeAngle(j)`, `dJointGetHingeAngleRate(j)`, `dJointSetHingeParam(j, p, v)`, `dJointGetHingeParam(j, p)`.

**Slider:** `dJointSetSliderAxis(j, x, y, z)`, `dJointGetSliderAxis(j)`, `dJointGetSliderPosition(j)`, `dJointGetSliderPositionRate(j)`, `dJointSetSliderParam(j, p, v)`, `dJointGetSliderParam(j, p)`.

**Hinge2:** `dJointSetHinge2Anchor(j, x, y, z)`, `dJointSetHinge2Axis1/Axis2(j, x, y, z)`, `dJointSetHinge2Param(j, p, v)`, `dJointGetHinge2Anchor/Anchor2(j)`, `dJointGetHinge2Axis1/Axis2(j)`, `dJointGetHinge2Param(j, p)`, `dJointGetHinge2Angle1/Angle2(j)`, `dJointGetHinge2Angle1Rate/Angle2Rate(j)`.

**Universal:** `dJointSetUniversalAnchor(j, x, y, z)`, `dJointSetUniversalAxis1/Axis2(j, x, y, z)`, `dJointSetUniversalParam(j, p, v)`, `dJointGetUniversalAnchor/Anchor2(j)`, `dJointGetUniversalAxis1/Axis2(j)`, `dJointGetUniversalParam(j, p)`, `dJointGetUniversalAngles(j)`, `dJointGetUniversalAngle1/Angle2(j)`, `dJointGetUniversalAngle1Rate/Angle2Rate(j)`.

**Fixed:** `dJointSetFixed(j)`, `dJointSetFixedParam(j, p, v)`, `dJointGetFixedParam(j, p)`.

**AMotor:** `dJointSetAMotorMode(j, m)`, `dJointSetAMotorNumAxes(j, n)`, `dJointSetAMotorAxis(j, num, rel, x, y, z)`, `dJointSetAMotorParam(j, p, v)`, `dJointGetAMotorMode(j)`, `dJointGetAMotorNumAxes(j)`, `dJointGetAMotorAxis(j, num)`, `dJointGetAMotorAxisRel(j, num)`, `dJointGetAMotorAngle(j, num)`, `dJointGetAMotorAngleRate(j, num)`, `dJointGetAMotorParam(j, p)`, `dJointAddAMotorTorques(j, t0, t1, t2)`.

**LMotor:** `dJointSetLMotorNumAxes(j, n)`, `dJointSetLMotorAxis(j, num, rel, x, y, z)`, `dJointSetLMotorParam(j, p, v)`, `dJointGetLMotorNumAxes(j)`, `dJointGetLMotorAxis(j, num)`, `dJointGetLMotorParam(j, p)`.

### Constants — Geometry Classes

| Constant | Description |
|----------|-------------|
| `dSphereClass` | Sphere geometry. |
| `dBoxClass` | Box geometry. |
| `dCapsuleClass` | Capsule geometry. |
| `dCylinderClass` | Cylinder geometry. |
| `dPlaneClass` | Plane geometry. |
| `dRayClass` | Ray geometry. |

### Constants — Contact Flags

| Constant | Description |
|----------|-------------|
| `dContactEventsMax` | Max contact events. |
| `dContactApprox1` | Approximate friction. |
| `dContactBounce` | Enable bounce. |
| `dContactSoftERP` | Soft constraint ERP. |
| `dContactSoftCFM` | Soft constraint CFM. |

### Constants — Joint Types

| Constant | Description |
|----------|-------------|
| `dJointTypeNone` | No joint. |
| `dJointTypeBall` | Ball-and-socket. |
| `dJointTypeHinge` | Hinge. |
| `dJointTypeSlider` | Slider. |
| `dJointTypeContact` | Contact. |
| `dJointTypeUniversal` | Universal. |
| `dJointTypeHinge2` | Hinge-2. |
| `dJointTypeFixed` | Fixed. |
| `dJointTypeNull` | Null. |
| `dJointTypeAMotor` | Angular motor. |
| `dJointTypeLMotor` | Linear motor. |
| `dJointTypePlane2D` | Plane-2D. |
| `dJointTypePR` | Prismatic-rotoide. |
| `dJointTypePU` | Prismatic-universal. |
| `dJointTypePiston` | Piston. |

### Constants — Joint Parameters

**Group 1:** `dParamLoStop`, `dParamHiStop`, `dParamVel`, `dParamFMax`, `dParamBounce`, `dParamCFM`, `dParamStopERP`, `dParamStopCFM`, `dParamSuspensionERP`, `dParamSuspensionCFM`

**Group 2:** `dParamLoStop2`, `dParamHiStop2`, `dParamVel2`, `dParamFMax2`, `dParamBounce2`, `dParamCFM2`, `dParamStopERP2`, `dParamStopCFM2`

**Group 3:** `dParamLoStop3`, `dParamHiStop3`, `dParamVel3`, `dParamFMax3`, `dParamBounce3`, `dParamCFM3`, `dParamStopERP3`, `dParamStopCFM3`

### Constants — AMotor Modes

| Constant | Description |
|----------|-------------|
| `dAMotorUser` | User mode. |
| `dAMotorEuler` | Euler mode. |

### Constants — Float

| Constant | Description |
|----------|-------------|
| `dInfinity` | Infinity value. |

---

## BuOpenSteer — Module `OpenSteer`

AI steering behaviours via OpenSteer.

```bu
require "BuOpenSteer";
import OpenSteer;
```

### Classes

#### `SteerAgent`

Autonomous steering agent. Constructible.

| Method | Signature | Returns |
|--------|-----------|---------|
| `destroy` | `destroy()` | — |
| `isValid` | `isValid()` | `bool` |
| `reset` | `reset()` | — |
| `getPosition` | `getPosition()` | `x, y, z` |
| `setPosition` | `setPosition(x, y, z)` | — |
| `getForward` | `getForward()` | `x, y, z` |
| `setForward` | `setForward(x, y, z)` | — |
| `randomizeHeadingXZ` | `randomizeHeadingXZ()` | — |
| `getVelocity` | `getVelocity()` | `x, y, z` |
| `getSpeed` | `getSpeed()` | `float` |
| `setSpeed` | `setSpeed(v)` | — |
| `getMass` | `getMass()` | `float` |
| `setMass` | `setMass(v)` | — |
| `getRadius` | `getRadius()` | `float` |
| `setRadius` | `setRadius(v)` | — |
| `getMaxForce` | `getMaxForce()` | `float` |
| `setMaxForce` | `setMaxForce(v)` | — |
| `getMaxSpeed` | `getMaxSpeed()` | `float` |
| `setMaxSpeed` | `setMaxSpeed(v)` | — |
| `predictFuturePosition` | `predictFuturePosition(dt)` | `x, y, z` |
| `seek` | `seek(tx, ty, tz)` | `fx, fy, fz` |
| `flee` | `flee(tx, ty, tz)` | `fx, fy, fz` |
| `wander` | `wander(dt)` | `fx, fy, fz` |
| `targetSpeed` | `targetSpeed(speed)` | `fx, fy, fz` |
| `followPath` | `followPath(pathway, direction, dt)` | `fx, fy, fz` |
| `stayOnPath` | `stayOnPath(dt, pathway)` | `fx, fy, fz` |
| `arrive` | `arrive(tx, ty, tz, slowDist)` | `fx, fy, fz` |
| `pursuit` | `pursuit(otherAgent, dt)` | `fx, fy, fz` |
| `evasion` | `evasion(otherAgent, dt)` | `fx, fy, fz` |
| `avoidSphere` | `avoidSphere(obstacle)` | `fx, fy, fz` |
| `separation` | `separation(agents, maxDist, cosAngle)` | `fx, fy, fz` |
| `alignment` | `alignment(agents, maxDist, cosAngle)` | `fx, fy, fz` |
| `cohesion` | `cohesion(agents, maxDist, cosAngle)` | `fx, fy, fz` |
| `avoidNeighbors` | `avoidNeighbors(agents, minDist)` | `fx, fy, fz` |
| `applySteeringForce` | `applySteeringForce(fx, fy, fz, dt)` | — |
| `applyBrakingForce` | `applyBrakingForce(rate, dt)` | — |

#### `SteerSphereObstacle`

Spherical obstacle for avoidance.

| Method | Signature | Returns |
|--------|-----------|---------|
| `destroy` | `destroy()` | — |
| `isValid` | `isValid()` | `bool` |
| `getCenter` | `getCenter()` | `x, y, z` |
| `setCenter` | `setCenter(x, y, z)` | — |
| `getRadius` | `getRadius()` | `float` |
| `setRadius` | `setRadius(r)` | — |

#### `SteerPathway`

A path made of connected line segments with a radius.

| Method | Signature | Returns |
|--------|-----------|---------|
| `destroy` | `destroy()` | — |
| `isValid` | `isValid()` | `bool` |
| `setPath` | `setPath(points, radius, cyclic)` | — |
| `getRadius` | `getRadius()` | `float` |
| `setRadius` | `setRadius(r)` | — |
| `length` | `length()` | `float` |
| `isCyclic` | `isCyclic()` | `bool` |
| `getPointCount` | `getPointCount()` | `int` |
| `getPoint` | `getPoint(index)` | `x, y, z` |
| `mapPoint` | `mapPoint(x, y, z)` | `px, py, pz` |
| `mapDistanceToPoint` | `mapDistanceToPoint(dist)` | `x, y, z` |
| `mapPointToDistance` | `mapPointToDistance(x, y, z)` | `float` |

### Constants

| Constant | Description |
|----------|-------------|
| `SEEN_FROM_OUTSIDE` | Obstacle seen from outside. |
| `SEEN_FROM_INSIDE` | Obstacle seen from inside. |
| `SEEN_FROM_BOTH` | Obstacle seen from both sides. |

---

## BuRecast — Module `Recast`

Navigation mesh generation, pathfinding, and crowds via Recast/Detour.

```bu
require "BuRecast";
import Recast;
```

### Classes

#### `NavMesh`

Build a navigation mesh from geometry and query paths.

| Method | Signature | Returns |
|--------|-----------|---------|
| `build` | `build(verts, indices, config...)` | `bool` |
| `isValid` | `isValid()` | `bool` |
| `destroy` | `destroy()` | — |
| `findPath` | `findPath(sx, sy, sz, ex, ey, ez)` | `Array` (path points) |
| `findNearestPoint` | `findNearestPoint(x, y, z)` | `px, py, pz` |

#### `NavCrowd`

Crowd simulation on a navmesh.

| Method | Signature | Returns |
|--------|-----------|---------|
| `addAgent` | `addAgent(x, y, z, ...)` | `int` (agent ID) |
| `removeAgent` | `removeAgent(id)` | — |
| `setTarget` | `setTarget(id, x, y, z)` | — |
| `update` | `update(dt)` | — |
| `getPosition` | `getPosition(id)` | `x, y, z` |
| `getVelocity` | `getVelocity(id)` | `vx, vy, vz` |
| `isAgentActive` | `isAgentActive(id)` | `bool` |
| `getAgentCount` | `getAgentCount()` | `int` |
| `destroy` | `destroy()` | — |

#### `NavTileCache`

Tiled navmesh with dynamic obstacle support.

| Method | Signature | Returns |
|--------|-----------|---------|
| `isValid` | `isValid()` | `bool` |
| `destroy` | `destroy()` | — |
| `init` | `init(config...)` | `bool` |
| `buildFlatTile` | `buildFlatTile(x, z, verts, indices)` | `bool` |
| `addCylinderObstacle` | `addCylinderObstacle(x, y, z, r, h)` | `int` (obstacle ID) |
| `addBoxObstacle` | `addBoxObstacle(x, y, z, hx, hy, hz)` | `int` |
| `addOrientedBoxObstacle` | `addOrientedBoxObstacle(x, y, z, hx, hy, hz, angle)` | `int` |
| `removeObstacle` | `removeObstacle(id)` | — |
| `getObstacleState` | `getObstacleState(id)` | `int` |
| `update` | `update()` | — |
| `findPath` | `findPath(sx, sy, sz, ex, ey, ez)` | `Array` |
| `findNearestPoint` | `findNearestPoint(x, y, z)` | `px, py, pz` |
| `getStats` | `getStats()` | stats object |

### Constants

| Constant | Description |
|----------|-------------|
| `OBSTACLE_EMPTY` | No obstacle. |
| `OBSTACLE_PROCESSING` | Obstacle being processed. |
| `OBSTACLE_PROCESSED` | Obstacle active. |
| `OBSTACLE_REMOVING` | Obstacle being removed. |
| `OBSTACLE_CYLINDER` | Cylinder obstacle type. |
| `OBSTACLE_BOX` | Box obstacle type. |
| `OBSTACLE_ORIENTED_BOX` | Oriented box obstacle type. |
