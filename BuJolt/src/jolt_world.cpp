#include "jolt_core.hpp"

#include <cmath>
#include <thread>

namespace JoltBindings
{
    static void *jolt_world_ctor(Interpreter *vm, int argCount, Value *args)
    {
        if (argCount != 0 && argCount != 1)
        {
            Error("JoltWorld expects 0 arguments or (Vector3 gravity)");
            return nullptr;
        }

        if (!ensure_jolt_runtime())
        {
            Error("Failed to initialize Jolt runtime");
            return nullptr;
        }

        Vector3 gravity = {0.0f, -9.81f, 0.0f};
        if (argCount == 1 && !read_vector3_arg(args[0], &gravity, "JoltWorld", 1))
            return nullptr;

        JoltWorldHandle *world = new JoltWorldHandle();
        const uint maxBodies = 4096;
        const uint numBodyMutexes = 0;
        const uint maxBodyPairs = 4096;
        const uint maxContactConstraints = 4096;

        uint32_t hardwareThreads = std::thread::hardware_concurrency();
        int workerThreads = hardwareThreads > 1 ? (int)hardwareThreads - 1 : 1;

        world->bodies.reserve((size_t)maxBodies);
        world->constraints.reserve((size_t)maxBodies);
        world->tempAllocator = new TempAllocatorImpl(10 * 1024 * 1024);
        world->jobSystem = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, workerThreads);
        world->physicsSystem.Init(maxBodies,
                                  numBodyMutexes,
                                  maxBodyPairs,
                                  maxContactConstraints,
                                  world->broadPhaseLayerInterface,
                                  world->objectVsBroadPhaseLayerFilter,
                                  world->objectLayerPairFilter);
        world->physicsSystem.SetGravity(to_jolt_vec3(gravity));
        world->valid = true;
        return world;
    }

    static void jolt_world_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        JoltWorldHandle *world = (JoltWorldHandle *)instance;
        destroy_world_runtime(world);
        delete world;
    }

    static int jolt_world_destroy(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("JoltWorld.destroy() expects 0 arguments");
            return 0;
        }

        destroy_world_runtime((JoltWorldHandle *)instance);
        return 0;
    }

    static int jolt_world_is_valid(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltWorld.isValid() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = (JoltWorldHandle *)instance;
        vm->pushBool(world != nullptr && world->valid);
        return 1;
    }

    static int jolt_world_step(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1 && argCount != 2)
        {
            Error("JoltWorld.step() expects (dt[, collisionSteps])");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.step()");
        if (!world)
            return push_nil1(vm);

        double dt = 0.0;
        if (!read_number_arg(args[0], &dt, "JoltWorld.step()", 1))
            return push_nil1(vm);
        if (!std::isfinite(dt) || dt < 0.0)
        {
            Error("JoltWorld.step() arg 1 expects finite dt >= 0");
            return push_nil1(vm);
        }

        int collisionSteps = 1;
        if (argCount == 2 && !read_int_arg(args[1], &collisionSteps, "JoltWorld.step()", 2))
            return push_nil1(vm);
        if (collisionSteps < 1)
            collisionSteps = 1;

        EPhysicsUpdateError err = world->physicsSystem.Update((float)dt,
                                                              collisionSteps,
                                                              world->tempAllocator,
                                                              world->jobSystem);
        vm->pushInt((int)err);
        return 1;
    }

    static int jolt_world_set_gravity(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltWorld.setGravity() expects (Vector3)");
            return 0;
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.setGravity()");
        if (!world)
            return 0;

        Vector3 gravity;
        if (!read_vector3_arg(args[0], &gravity, "JoltWorld.setGravity()", 1))
            return 0;

        world->physicsSystem.SetGravity(to_jolt_vec3(gravity));
        return 0;
    }

    static int jolt_world_get_gravity(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltWorld.getGravity() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.getGravity()");
        if (!world)
            return push_nil1(vm);

        return push_vector3(vm, from_jolt_vec3(world->physicsSystem.GetGravity())) ? 1 : 0;
    }

    static int jolt_world_optimize_broad_phase(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("JoltWorld.optimizeBroadPhase() expects 0 arguments");
            return 0;
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.optimizeBroadPhase()");
        if (!world)
            return 0;

        world->physicsSystem.OptimizeBroadPhase();
        return 0;
    }

    static int jolt_world_get_body_count(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltWorld.getBodyCount() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.getBodyCount()");
        if (!world)
            return push_nil1(vm);

        vm->pushInt((int)world->physicsSystem.GetNumBodies());
        return 1;
    }

    static int jolt_world_create_static_box(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 4)
        {
            Error("JoltWorld.createStaticBox() expects (hx, hy, hz, Vector3)");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.createStaticBox()");
        if (!world)
            return push_nil1(vm);

        double hx = 0.0;
        double hy = 0.0;
        double hz = 0.0;
        Vector3 position;
        if (!read_number_arg(args[0], &hx, "JoltWorld.createStaticBox()", 1) ||
            !read_number_arg(args[1], &hy, "JoltWorld.createStaticBox()", 2) ||
            !read_number_arg(args[2], &hz, "JoltWorld.createStaticBox()", 3) ||
            !read_vector3_arg(args[3], &position, "JoltWorld.createStaticBox()", 4))
        {
            return push_nil1(vm);
        }
        if (hx <= 0.0 || hy <= 0.0 || hz <= 0.0)
        {
            Error("JoltWorld.createStaticBox() expects hx, hy, hz > 0");
            return push_nil1(vm);
        }

        JoltBodyHandle *body = create_body_handle(world,
                                                  new BoxShape(Vec3((float)hx, (float)hy, (float)hz)),
                                                  position,
                                                  EMotionType::Static,
                                                  Layers::NON_MOVING,
                                                  0.9f,
                                                  0.05f,
                                                  0.0f);
        if (!body)
            return push_nil1(vm);

        return push_body_handle(vm, body) ? 1 : push_nil1(vm);
    }

    static int jolt_world_create_box(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 4)
        {
            Error("JoltWorld.createBox() expects (hx, hy, hz, Vector3)");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.createBox()");
        if (!world)
            return push_nil1(vm);

        double hx = 0.0;
        double hy = 0.0;
        double hz = 0.0;
        Vector3 position;
        if (!read_number_arg(args[0], &hx, "JoltWorld.createBox()", 1) ||
            !read_number_arg(args[1], &hy, "JoltWorld.createBox()", 2) ||
            !read_number_arg(args[2], &hz, "JoltWorld.createBox()", 3) ||
            !read_vector3_arg(args[3], &position, "JoltWorld.createBox()", 4))
        {
            return push_nil1(vm);
        }
        if (hx <= 0.0 || hy <= 0.0 || hz <= 0.0)
        {
            Error("JoltWorld.createBox() expects hx, hy, hz > 0");
            return push_nil1(vm);
        }

        JoltBodyHandle *body = create_body_handle(world,
                                                  new BoxShape(Vec3((float)hx, (float)hy, (float)hz)),
                                                  position,
                                                  EMotionType::Dynamic,
                                                  Layers::MOVING,
                                                  0.7f,
                                                  0.12f,
                                                  0.0f);
        if (!body)
            return push_nil1(vm);

        return push_body_handle(vm, body) ? 1 : push_nil1(vm);
    }

    static int jolt_world_create_sphere(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("JoltWorld.createSphere() expects (radius, Vector3)");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.createSphere()");
        if (!world)
            return push_nil1(vm);

        double radius = 0.0;
        Vector3 position;
        if (!read_number_arg(args[0], &radius, "JoltWorld.createSphere()", 1) ||
            !read_vector3_arg(args[1], &position, "JoltWorld.createSphere()", 2))
        {
            return push_nil1(vm);
        }
        if (radius <= 0.0)
        {
            Error("JoltWorld.createSphere() expects radius > 0");
            return push_nil1(vm);
        }

        JoltBodyHandle *body = create_body_handle(world,
                                                  new SphereShape((float)radius),
                                                  position,
                                                  EMotionType::Dynamic,
                                                  Layers::MOVING,
                                                  0.55f,
                                                  0.35f,
                                                  0.0f);
        if (!body)
            return push_nil1(vm);

        return push_body_handle(vm, body) ? 1 : push_nil1(vm);
    }

    static int jolt_world_create_capsule(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 3)
        {
            Error("JoltWorld.createCapsule() expects (halfHeight, radius, Vector3)");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.createCapsule()");
        if (!world)
            return push_nil1(vm);

        double halfHeight = 0.0;
        double radius = 0.0;
        Vector3 position;
        if (!read_number_arg(args[0], &halfHeight, "JoltWorld.createCapsule()", 1) ||
            !read_number_arg(args[1], &radius, "JoltWorld.createCapsule()", 2) ||
            !read_vector3_arg(args[2], &position, "JoltWorld.createCapsule()", 3))
        {
            return push_nil1(vm);
        }
        if (halfHeight <= 0.0 || radius <= 0.0)
        {
            Error("JoltWorld.createCapsule() expects halfHeight > 0 and radius > 0");
            return push_nil1(vm);
        }

        JoltBodyHandle *body = create_body_handle(world,
                                                  new CapsuleShape((float)halfHeight, (float)radius),
                                                  position,
                                                  EMotionType::Dynamic,
                                                  Layers::MOVING,
                                                  0.55f,
                                                  0.35f,
                                                  0.0f);
        if (!body)
            return push_nil1(vm);

        return push_body_handle(vm, body) ? 1 : push_nil1(vm);
    }

    static int jolt_world_create_static_capsule(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 3)
        {
            Error("JoltWorld.createStaticCapsule() expects (halfHeight, radius, Vector3)");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.createStaticCapsule()");
        if (!world)
            return push_nil1(vm);

        double halfHeight = 0.0;
        double radius = 0.0;
        Vector3 position;
        if (!read_number_arg(args[0], &halfHeight, "JoltWorld.createStaticCapsule()", 1) ||
            !read_number_arg(args[1], &radius, "JoltWorld.createStaticCapsule()", 2) ||
            !read_vector3_arg(args[2], &position, "JoltWorld.createStaticCapsule()", 3))
        {
            return push_nil1(vm);
        }
        if (halfHeight <= 0.0 || radius <= 0.0)
        {
            Error("JoltWorld.createStaticCapsule() expects halfHeight > 0 and radius > 0");
            return push_nil1(vm);
        }

        JoltBodyHandle *body = create_body_handle(world,
                                                  new CapsuleShape((float)halfHeight, (float)radius),
                                                  position,
                                                  EMotionType::Static,
                                                  Layers::NON_MOVING,
                                                  0.9f,
                                                  0.05f,
                                                  0.0f);
        if (!body)
            return push_nil1(vm);

        return push_body_handle(vm, body) ? 1 : push_nil1(vm);
    }

    static int jolt_world_create_offset_box(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 5 && argCount != 6)
        {
            Error("JoltWorld.createOffsetBox() expects (hx, hy, hz, offset, position[, mass])");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.createOffsetBox()");
        if (!world)
            return push_nil1(vm);

        double hx = 0.0;
        double hy = 0.0;
        double hz = 0.0;
        double mass = 0.0;
        Vector3 offset;
        Vector3 position;
        if (!read_number_arg(args[0], &hx, "JoltWorld.createOffsetBox()", 1) ||
            !read_number_arg(args[1], &hy, "JoltWorld.createOffsetBox()", 2) ||
            !read_number_arg(args[2], &hz, "JoltWorld.createOffsetBox()", 3) ||
            !read_vector3_arg(args[3], &offset, "JoltWorld.createOffsetBox()", 4) ||
            !read_vector3_arg(args[4], &position, "JoltWorld.createOffsetBox()", 5))
        {
            return push_nil1(vm);
        }
        if (hx <= 0.0 || hy <= 0.0 || hz <= 0.0)
        {
            Error("JoltWorld.createOffsetBox() expects hx, hy, hz > 0");
            return push_nil1(vm);
        }

        if (argCount == 6 && !read_number_arg(args[5], &mass, "JoltWorld.createOffsetBox()", 6))
            return push_nil1(vm);
        if (mass < 0.0)
        {
            Error("JoltWorld.createOffsetBox() expects mass >= 0");
            return push_nil1(vm);
        }

        RefConst<Shape> shape = OffsetCenterOfMassShapeSettings(
                                    to_jolt_vec3(offset),
                                    new BoxShape(Vec3((float)hx, (float)hy, (float)hz)))
                                    .Create()
                                    .Get();
        if (shape == nullptr)
        {
            Error("JoltWorld.createOffsetBox() could not create offset shape");
            return push_nil1(vm);
        }

        JoltBodyHandle *body = create_body_handle(world,
                                                  shape.GetPtr(),
                                                  position,
                                                  EMotionType::Dynamic,
                                                  Layers::MOVING,
                                                  0.7f,
                                                  0.12f,
                                                  (float)mass);
        if (!body)
            return push_nil1(vm);

        return push_body_handle(vm, body) ? 1 : push_nil1(vm);
    }

    static int jolt_world_create_static_sphere(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("JoltWorld.createStaticSphere() expects (radius, Vector3)");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.createStaticSphere()");
        if (!world)
            return push_nil1(vm);

        double radius = 0.0;
        Vector3 position;
        if (!read_number_arg(args[0], &radius, "JoltWorld.createStaticSphere()", 1) ||
            !read_vector3_arg(args[1], &position, "JoltWorld.createStaticSphere()", 2))
        {
            return push_nil1(vm);
        }
        if (radius <= 0.0)
        {
            Error("JoltWorld.createStaticSphere() expects radius > 0");
            return push_nil1(vm);
        }

        JoltBodyHandle *body = create_body_handle(world,
                                                  new SphereShape((float)radius),
                                                  position,
                                                  EMotionType::Static,
                                                  Layers::NON_MOVING,
                                                  0.9f,
                                                  0.05f,
                                                  0.0f);
        if (!body)
            return push_nil1(vm);

        return push_body_handle(vm, body) ? 1 : push_nil1(vm);
    }

    // ─── Cylinder Shape ───────────────────────────────────────────────────

    static int jolt_world_create_cylinder(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 3)
        {
            Error("JoltWorld.createCylinder() expects (halfHeight, radius, Vector3)");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.createCylinder()");
        if (!world)
            return push_nil1(vm);

        double halfHeight = 0.0;
        double radius = 0.0;
        Vector3 position;
        if (!read_number_arg(args[0], &halfHeight, "JoltWorld.createCylinder()", 1) ||
            !read_number_arg(args[1], &radius, "JoltWorld.createCylinder()", 2) ||
            !read_vector3_arg(args[2], &position, "JoltWorld.createCylinder()", 3))
        {
            return push_nil1(vm);
        }
        if (halfHeight <= 0.0 || radius <= 0.0)
        {
            Error("JoltWorld.createCylinder() expects halfHeight > 0 and radius > 0");
            return push_nil1(vm);
        }

        JoltBodyHandle *body = create_body_handle(world,
                                                  new CylinderShape((float)halfHeight, (float)radius),
                                                  position,
                                                  EMotionType::Dynamic,
                                                  Layers::MOVING,
                                                  0.7f,
                                                  0.12f,
                                                  0.0f);
        if (!body)
            return push_nil1(vm);

        return push_body_handle(vm, body) ? 1 : push_nil1(vm);
    }

    static int jolt_world_create_static_cylinder(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 3)
        {
            Error("JoltWorld.createStaticCylinder() expects (halfHeight, radius, Vector3)");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.createStaticCylinder()");
        if (!world)
            return push_nil1(vm);

        double halfHeight = 0.0;
        double radius = 0.0;
        Vector3 position;
        if (!read_number_arg(args[0], &halfHeight, "JoltWorld.createStaticCylinder()", 1) ||
            !read_number_arg(args[1], &radius, "JoltWorld.createStaticCylinder()", 2) ||
            !read_vector3_arg(args[2], &position, "JoltWorld.createStaticCylinder()", 3))
        {
            return push_nil1(vm);
        }
        if (halfHeight <= 0.0 || radius <= 0.0)
        {
            Error("JoltWorld.createStaticCylinder() expects halfHeight > 0 and radius > 0");
            return push_nil1(vm);
        }

        JoltBodyHandle *body = create_body_handle(world,
                                                  new CylinderShape((float)halfHeight, (float)radius),
                                                  position,
                                                  EMotionType::Static,
                                                  Layers::NON_MOVING,
                                                  0.9f,
                                                  0.05f,
                                                  0.0f);
        if (!body)
            return push_nil1(vm);

        return push_body_handle(vm, body) ? 1 : push_nil1(vm);
    }

    // ─── Raycasting ───────────────────────────────────────────────────────

    static int jolt_world_cast_ray(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("JoltWorld.castRay() expects (Vector3 origin, Vector3 direction)");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.castRay()");
        if (!world)
            return push_nil1(vm);

        Vector3 origin;
        Vector3 direction;
        if (!read_vector3_arg(args[0], &origin, "JoltWorld.castRay()", 1) ||
            !read_vector3_arg(args[1], &direction, "JoltWorld.castRay()", 2))
        {
            return push_nil1(vm);
        }

        RRayCast ray(to_jolt_rvec3(origin), to_jolt_vec3(direction));
        RayCastResult hit;
        hit.mFraction = 1.0f + FLT_EPSILON;

        const NarrowPhaseQuery &query = world->physicsSystem.GetNarrowPhaseQuery();
        bool didHit = query.CastRay(ray, hit);

        if (!didHit)
        {
            vm->pushBool(false);
            return 1;
        }

        // Return hit point as Vector3
        Vector3 hitPoint;
        hitPoint.x = origin.x + hit.mFraction * direction.x;
        hitPoint.y = origin.y + hit.mFraction * direction.y;
        hitPoint.z = origin.z + hit.mFraction * direction.z;

        if (!push_vector3(vm, hitPoint))
            return push_nil1(vm);

        return 1;
    }

    static int jolt_world_cast_ray_detailed(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("JoltWorld.castRayDetailed() expects (Vector3 origin, Vector3 direction)");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.castRayDetailed()");
        if (!world)
            return push_nil1(vm);

        Vector3 origin;
        Vector3 direction;
        if (!read_vector3_arg(args[0], &origin, "JoltWorld.castRayDetailed()", 1) ||
            !read_vector3_arg(args[1], &direction, "JoltWorld.castRayDetailed()", 2))
        {
            return push_nil1(vm);
        }

        RRayCast ray(to_jolt_rvec3(origin), to_jolt_vec3(direction));
        RayCastResult hit;
        hit.mFraction = 1.0f + FLT_EPSILON;

        const NarrowPhaseQuery &query = world->physicsSystem.GetNarrowPhaseQuery();
        bool didHit = query.CastRay(ray, hit);

        if (!didHit)
        {
            vm->pushBool(false);
            return 1;
        }

        // Push 3 values: hitPoint (Vector3), fraction (number), bodyID (int)
        Vector3 hitPoint;
        hitPoint.x = origin.x + hit.mFraction * direction.x;
        hitPoint.y = origin.y + hit.mFraction * direction.y;
        hitPoint.z = origin.z + hit.mFraction * direction.z;

        if (!push_vector3(vm, hitPoint))
            return push_nil1(vm);

        vm->pushFloat(hit.mFraction);
        vm->pushInt((int)hit.mBodyID.GetIndexAndSequenceNumber());
        return 3;
    }

    // ─── Get active body count ────────────────────────────────────────────

    static int jolt_world_get_active_body_count(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltWorld.getActiveBodyCount() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.getActiveBodyCount()");
        if (!world)
            return push_nil1(vm);

        vm->pushInt((int)world->physicsSystem.GetNumActiveBodies(EBodyType::RigidBody));
        return 1;
    }

    // ─── Helper: read flat float array from script ────────────────────────

    static bool read_float_array_arg(const Value &value, std::vector<float> &out, const char *fn, int argIndex)
    {
        if (value.isArray())
        {
            ArrayInstance *arr = value.asArray();
            if (!arr)
            {
                Error("%s arg %d expects array of numbers", fn, argIndex);
                return false;
            }
            out.reserve(arr->values.size());
            for (size_t i = 0; i < arr->values.size(); ++i)
            {
                if (!arr->values[i].isNumber())
                {
                    Error("%s arg %d item %d expects number", fn, argIndex, (int)i + 1);
                    return false;
                }
                out.push_back((float)arr->values[i].asNumber());
            }
            return true;
        }
        if (value.isBuffer())
        {
            BufferInstance *buf = value.asBuffer();
            if (!buf || !buf->data || buf->count == 0)
            {
                Error("%s arg %d expects non-empty buffer", fn, argIndex);
                return false;
            }
            if (buf->type == BufferType::FLOAT)
            {
                float *ptr = (float *)buf->data;
                out.assign(ptr, ptr + buf->count);
                return true;
            }
            if (buf->type == BufferType::DOUBLE)
            {
                double *ptr = (double *)buf->data;
                out.reserve(buf->count);
                for (int i = 0; i < buf->count; ++i)
                    out.push_back((float)ptr[i]);
                return true;
            }
            Error("%s arg %d buffer type must be FLOAT or DOUBLE", fn, argIndex);
            return false;
        }
        Error("%s arg %d expects array or buffer of floats", fn, argIndex);
        return false;
    }

    static bool read_int_array_arg(const Value &value, std::vector<int> &out, const char *fn, int argIndex)
    {
        if (value.isArray())
        {
            ArrayInstance *arr = value.asArray();
            if (!arr)
            {
                Error("%s arg %d expects array of ints", fn, argIndex);
                return false;
            }
            out.reserve(arr->values.size());
            for (size_t i = 0; i < arr->values.size(); ++i)
            {
                if (!arr->values[i].isNumber())
                {
                    Error("%s arg %d item %d expects int", fn, argIndex, (int)i + 1);
                    return false;
                }
                out.push_back(arr->values[i].asInt());
            }
            return true;
        }
        if (value.isBuffer())
        {
            BufferInstance *buf = value.asBuffer();
            if (!buf || !buf->data || buf->count == 0)
            {
                Error("%s arg %d expects non-empty buffer", fn, argIndex);
                return false;
            }
            if (buf->type == BufferType::INT32)
            {
                int32_t *ptr = (int32_t *)buf->data;
                out.assign(ptr, ptr + buf->count);
                return true;
            }
            if (buf->type == BufferType::UINT32)
            {
                uint32_t *ptr = (uint32_t *)buf->data;
                out.reserve(buf->count);
                for (int i = 0; i < buf->count; ++i)
                    out.push_back((int)ptr[i]);
                return true;
            }
            Error("%s arg %d buffer type must be INT32 or UINT32", fn, argIndex);
            return false;
        }
        Error("%s arg %d expects array or buffer of ints", fn, argIndex);
        return false;
    }

    // ─── Mesh Shape (static triangle mesh) ───────────────────────────────

    static int jolt_world_create_mesh(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 3)
        {
            Error("JoltWorld.createMesh() expects (vertices, indices, Vector3 position)");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.createMesh()");
        if (!world)
            return push_nil1(vm);

        std::vector<float> verts;
        std::vector<int> idxs;
        Vector3 position;
        if (!read_float_array_arg(args[0], verts, "JoltWorld.createMesh()", 1) ||
            !read_int_array_arg(args[1], idxs, "JoltWorld.createMesh()", 2) ||
            !read_vector3_arg(args[2], &position, "JoltWorld.createMesh()", 3))
        {
            return push_nil1(vm);
        }

        if (verts.size() < 9 || verts.size() % 3 != 0)
        {
            Error("JoltWorld.createMesh() vertices must have at least 9 floats (3 verts) and be multiple of 3");
            return push_nil1(vm);
        }
        if (idxs.size() < 3 || idxs.size() % 3 != 0)
        {
            Error("JoltWorld.createMesh() indices must have at least 3 and be multiple of 3");
            return push_nil1(vm);
        }

        int vertexCount = (int)verts.size() / 3;

        // Build Jolt vertex list
        VertexList joltVerts;
        joltVerts.resize(vertexCount);
        for (int i = 0; i < vertexCount; ++i)
        {
            joltVerts[i] = Float3(verts[i * 3 + 0], verts[i * 3 + 1], verts[i * 3 + 2]);
        }

        // Build Jolt indexed triangle list
        int triCount = (int)idxs.size() / 3;
        IndexedTriangleList joltTris;
        joltTris.resize(triCount);
        for (int i = 0; i < triCount; ++i)
        {
            joltTris[i] = IndexedTriangle((uint32)idxs[i * 3 + 0],
                                           (uint32)idxs[i * 3 + 1],
                                           (uint32)idxs[i * 3 + 2],
                                           0);
        }

        MeshShapeSettings settings(std::move(joltVerts), std::move(joltTris));
        ShapeSettings::ShapeResult result = settings.Create();
        if (result.HasError())
        {
            Error("JoltWorld.createMesh() failed: %s", result.GetError().c_str());
            return push_nil1(vm);
        }

        JoltBodyHandle *body = create_body_handle(world,
                                                  result.Get().GetPtr(),
                                                  position,
                                                  EMotionType::Static,
                                                  Layers::NON_MOVING,
                                                  0.9f,
                                                  0.05f,
                                                  0.0f);
        if (!body)
            return push_nil1(vm);

        return push_body_handle(vm, body) ? 1 : push_nil1(vm);
    }

    // ─── Convex Hull Shape ───────────────────────────────────────────────

    static int jolt_world_create_convex_hull(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2 && argCount != 3)
        {
            Error("JoltWorld.createConvexHull() expects (vertices, Vector3 position[, mass])");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.createConvexHull()");
        if (!world)
            return push_nil1(vm);

        std::vector<float> verts;
        Vector3 position;
        if (!read_float_array_arg(args[0], verts, "JoltWorld.createConvexHull()", 1) ||
            !read_vector3_arg(args[1], &position, "JoltWorld.createConvexHull()", 2))
        {
            return push_nil1(vm);
        }

        double mass = 0.0;
        if (argCount == 3 && !read_number_arg(args[2], &mass, "JoltWorld.createConvexHull()", 3))
            return push_nil1(vm);

        if (verts.size() < 12 || verts.size() % 3 != 0)
        {
            Error("JoltWorld.createConvexHull() vertices must have at least 12 floats (4 points) and be multiple of 3");
            return push_nil1(vm);
        }

        int pointCount = (int)verts.size() / 3;
        JPH::Array<Vec3> points;
        points.resize(pointCount);
        for (int i = 0; i < pointCount; ++i)
            points[i] = Vec3(verts[i * 3 + 0], verts[i * 3 + 1], verts[i * 3 + 2]);

        ConvexHullShapeSettings settings(points, cDefaultConvexRadius);
        ShapeSettings::ShapeResult result = settings.Create();
        if (result.HasError())
        {
            Error("JoltWorld.createConvexHull() failed: %s", result.GetError().c_str());
            return push_nil1(vm);
        }

        bool isDynamic = mass > 0.0;
        JoltBodyHandle *body = create_body_handle(world,
                                                  result.Get().GetPtr(),
                                                  position,
                                                  isDynamic ? EMotionType::Dynamic : EMotionType::Static,
                                                  isDynamic ? Layers::MOVING : Layers::NON_MOVING,
                                                  isDynamic ? 0.7f : 0.9f,
                                                  isDynamic ? 0.12f : 0.05f,
                                                  (float)mass);
        if (!body)
            return push_nil1(vm);

        return push_body_handle(vm, body) ? 1 : push_nil1(vm);
    }

    // ─── Height Field Shape ──────────────────────────────────────────────

    static int jolt_world_create_height_field(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 3 || argCount > 4)
        {
            Error("JoltWorld.createHeightField() expects (heights, sampleCount, Vector3 position[, Vector3 scale])");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.createHeightField()");
        if (!world)
            return push_nil1(vm);

        std::vector<float> heights;
        if (!read_float_array_arg(args[0], heights, "JoltWorld.createHeightField()", 1))
            return push_nil1(vm);

        int sampleCount = 0;
        if (!read_int_arg(args[1], &sampleCount, "JoltWorld.createHeightField()", 2))
            return push_nil1(vm);

        Vector3 position;
        if (!read_vector3_arg(args[2], &position, "JoltWorld.createHeightField()", 3))
            return push_nil1(vm);

        Vector3 scale = {1.0f, 1.0f, 1.0f};
        if (argCount == 4 && !read_vector3_arg(args[3], &scale, "JoltWorld.createHeightField()", 4))
            return push_nil1(vm);

        if (sampleCount < 2)
        {
            Error("JoltWorld.createHeightField() expects sampleCount >= 2");
            return push_nil1(vm);
        }
        if ((int)heights.size() != sampleCount * sampleCount)
        {
            Error("JoltWorld.createHeightField() heights array size (%d) != sampleCount^2 (%d)", (int)heights.size(), sampleCount * sampleCount);
            return push_nil1(vm);
        }

        // Jolt HeightFieldShape: NxN square grid, offset centers the mesh, scale defines cell size
        Vec3 offset(-0.5f * scale.x * (float)sampleCount, 0.0f, -0.5f * scale.z * (float)sampleCount);
        Vec3 joltScale(scale.x, scale.y, scale.z);
        HeightFieldShapeSettings settings(heights.data(), offset, joltScale, (uint32)sampleCount);
        ShapeSettings::ShapeResult result = settings.Create();
        if (result.HasError())
        {
            Error("JoltWorld.createHeightField() failed: %s", result.GetError().c_str());
            return push_nil1(vm);
        }

        JoltBodyHandle *body = create_body_handle(world,
                                                  result.Get().GetPtr(),
                                                  position,
                                                  EMotionType::Static,
                                                  Layers::NON_MOVING,
                                                  0.9f,
                                                  0.05f,
                                                  0.0f);
        if (!body)
            return push_nil1(vm);

        return push_body_handle(vm, body) ? 1 : push_nil1(vm);
    }

    void register_jolt_world(Interpreter &vm)
    {
        g_worldClass = vm.registerNativeClass("JoltWorld", jolt_world_ctor, jolt_world_dtor, -1, false);

        vm.addNativeMethod(g_worldClass, "destroy", jolt_world_destroy);
        vm.addNativeMethod(g_worldClass, "isValid", jolt_world_is_valid);
        vm.addNativeMethod(g_worldClass, "step", jolt_world_step);
        vm.addNativeMethod(g_worldClass, "setGravity", jolt_world_set_gravity);
        vm.addNativeMethod(g_worldClass, "getGravity", jolt_world_get_gravity);
        vm.addNativeMethod(g_worldClass, "optimizeBroadPhase", jolt_world_optimize_broad_phase);
        vm.addNativeMethod(g_worldClass, "getBodyCount", jolt_world_get_body_count);
        vm.addNativeMethod(g_worldClass, "getActiveBodyCount", jolt_world_get_active_body_count);
        vm.addNativeMethod(g_worldClass, "createStaticBox", jolt_world_create_static_box);
        vm.addNativeMethod(g_worldClass, "createStaticSphere", jolt_world_create_static_sphere);
        vm.addNativeMethod(g_worldClass, "createStaticCapsule", jolt_world_create_static_capsule);
        vm.addNativeMethod(g_worldClass, "createStaticCylinder", jolt_world_create_static_cylinder);
        vm.addNativeMethod(g_worldClass, "createBox", jolt_world_create_box);
        vm.addNativeMethod(g_worldClass, "createCapsule", jolt_world_create_capsule);
        vm.addNativeMethod(g_worldClass, "createCylinder", jolt_world_create_cylinder);
        vm.addNativeMethod(g_worldClass, "createOffsetBox", jolt_world_create_offset_box);
        vm.addNativeMethod(g_worldClass, "createSphere", jolt_world_create_sphere);
        vm.addNativeMethod(g_worldClass, "createMesh", jolt_world_create_mesh);
        vm.addNativeMethod(g_worldClass, "createConvexHull", jolt_world_create_convex_hull);
        vm.addNativeMethod(g_worldClass, "createHeightField", jolt_world_create_height_field);
        vm.addNativeMethod(g_worldClass, "castRay", jolt_world_cast_ray);
        vm.addNativeMethod(g_worldClass, "castRayDetailed", jolt_world_cast_ray_detailed);
    }
}
