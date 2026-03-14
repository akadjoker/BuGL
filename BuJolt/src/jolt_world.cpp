#include "jolt_core.hpp"

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

        if (argCount == 6 && !read_number_arg(args[5], &mass, "JoltWorld.createOffsetBox()", 6))
            return push_nil1(vm);

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
        vm.addNativeMethod(g_worldClass, "createStaticBox", jolt_world_create_static_box);
        vm.addNativeMethod(g_worldClass, "createBox", jolt_world_create_box);
        vm.addNativeMethod(g_worldClass, "createOffsetBox", jolt_world_create_offset_box);
        vm.addNativeMethod(g_worldClass, "createSphere", jolt_world_create_sphere);
    }
}
