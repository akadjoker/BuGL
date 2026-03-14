#include "jolt_core.hpp"

#include <cstdarg>
#include <cstdio>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>

namespace JoltBindings
{
    NativeClassDef *g_worldClass = nullptr;
    NativeClassDef *g_bodyClass = nullptr;
    NativeStructDef *g_vector3Def = nullptr;
    NativeStructDef *g_quaternionDef = nullptr;
    bool g_joltRuntimeInitialized = false;

    static void trace_impl(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        Info("[Jolt] %s", buffer);
    }

#ifdef JPH_ENABLE_ASSERTS
    static bool assert_failed_impl(const char *expression, const char *message, const char *file, uint line)
    {
        Error("[Jolt] assert failed at %s:%u: (%s) %s",
              file ? file : "<unknown>",
              (unsigned)line,
              expression ? expression : "<expr>",
              message ? message : "");
        return true;
    }
#endif

    int push_nil1(Interpreter *vm)
    {
        vm->pushNil();
        return 1;
    }

    bool ensure_jolt_runtime()
    {
        if (g_joltRuntimeInitialized)
            return true;

        RegisterDefaultAllocator();
        JPH::Trace = trace_impl;
#ifdef JPH_ENABLE_ASSERTS
        JPH::AssertFailed = assert_failed_impl;
#endif

        if (Factory::sInstance == nullptr)
            Factory::sInstance = new Factory();

        RegisterTypes();
        g_joltRuntimeInitialized = true;
        return true;
    }

    void cleanup()
    {
        if (!g_joltRuntimeInitialized)
            return;

        UnregisterTypes();
        delete Factory::sInstance;
        Factory::sInstance = nullptr;
#ifdef JPH_ENABLE_ASSERTS
        JPH::AssertFailed = nullptr;
#endif
        JPH::Trace = nullptr;
        g_joltRuntimeInitialized = false;
    }

    bool read_struct_data(const Value &value, NativeStructDef *expectedDef, const char *typeName, const char *fn, int argIndex, void **out)
    {
        if (!out || !value.isNativeStructInstance())
        {
            Error("%s arg %d expects %s", fn, argIndex, typeName);
            return false;
        }

        NativeStructInstance *inst = value.asNativeStructInstance();
        if (!inst || !inst->data)
        {
            Error("%s arg %d expects %s", fn, argIndex, typeName);
            return false;
        }

        if (expectedDef != nullptr && inst->def != expectedDef)
        {
            Error("%s arg %d expects %s", fn, argIndex, typeName);
            return false;
        }

        *out = inst->data;
        return true;
    }

    bool read_number_arg(const Value &value, double *out, const char *fn, int argIndex)
    {
        if (!out || !value.isNumber())
        {
            Error("%s arg %d expects number", fn, argIndex);
            return false;
        }

        *out = value.asNumber();
        return true;
    }

    bool read_int_arg(const Value &value, int *out, const char *fn, int argIndex)
    {
        if (!out || !value.isNumber())
        {
            Error("%s arg %d expects int", fn, argIndex);
            return false;
        }

        *out = value.asInt();
        return true;
    }

    bool read_vector3_arg(const Value &value, Vector3 *out, const char *fn, int argIndex)
    {
        void *data = nullptr;
        if (!out || !read_struct_data(value, g_vector3Def, "Vector3", fn, argIndex, &data))
            return false;

        *out = *(Vector3 *)data;
        return true;
    }

    bool read_quaternion_arg(const Value &value, Quaternion *out, const char *fn, int argIndex)
    {
        void *data = nullptr;
        if (!out || !read_struct_data(value, g_quaternionDef, "Quaternion", fn, argIndex, &data))
            return false;

        *out = *(Quaternion *)data;
        return true;
    }

    bool push_vector3(Interpreter *vm, const Vector3 &value)
    {
        if (!vm || !g_vector3Def)
        {
            Error("Vector3 struct is not available");
            return false;
        }

        Value out = vm->createNativeStruct(g_vector3Def->id, 0, nullptr);
        NativeStructInstance *inst = out.asNativeStructInstance();
        if (!inst || !inst->data)
            return false;

        *(Vector3 *)inst->data = value;
        vm->push(out);
        return true;
    }

    bool push_quaternion(Interpreter *vm, const Quaternion &value)
    {
        if (!vm || !g_quaternionDef)
        {
            Error("Quaternion struct is not available");
            return false;
        }

        Value out = vm->createNativeStruct(g_quaternionDef->id, 0, nullptr);
        NativeStructInstance *inst = out.asNativeStructInstance();
        if (!inst || !inst->data)
            return false;

        *(Quaternion *)inst->data = value;
        vm->push(out);
        return true;
    }

    Vec3 to_jolt_vec3(const Vector3 &v)
    {
        return Vec3(v.x, v.y, v.z);
    }

    RVec3 to_jolt_rvec3(const Vector3 &v)
    {
        return RVec3((double)v.x, (double)v.y, (double)v.z);
    }

    Vector3 from_jolt_vec3(Vec3Arg v)
    {
        Vector3 out;
        out.x = v.GetX();
        out.y = v.GetY();
        out.z = v.GetZ();
        return out;
    }

    Vector3 from_jolt_rvec3(RVec3Arg v)
    {
        Vector3 out;
        out.x = (float)v.GetX();
        out.y = (float)v.GetY();
        out.z = (float)v.GetZ();
        return out;
    }

    Quaternion from_jolt_quat(QuatArg q)
    {
        Quaternion out;
        out.x = q.GetX();
        out.y = q.GetY();
        out.z = q.GetZ();
        out.w = q.GetW();
        return out;
    }

    bool push_body_handle(Interpreter *vm, JoltBodyHandle *body)
    {
        if (!vm || !body || !g_bodyClass)
            return false;

        Value value = vm->makeNativeClassInstance(false);
        NativeClassInstance *inst = value.asNativeClassInstance();
        if (!inst)
            return false;

        inst->klass = g_bodyClass;
        inst->userData = body;
        body->wrapperAlive = true;
        vm->push(value);
        return true;
    }

    NativeStructDef *get_native_struct_def(Interpreter *vm, const char *name)
    {
        if (!vm || !name)
            return nullptr;

        Value structValue;
        if (!vm->tryGetGlobal(name, &structValue) || !structValue.isNativeStruct())
            return nullptr;

        Value instanceValue = vm->createNativeStruct(structValue.asNativeStructId(), 0, nullptr);
        NativeStructInstance *inst = instanceValue.asNativeStructInstance();
        return inst ? inst->def : nullptr;
    }

    JoltWorldHandle *require_world(void *instance, const char *fn)
    {
        JoltWorldHandle *world = (JoltWorldHandle *)instance;
        if (!world || !world->valid)
        {
            Error("%s on invalid JoltWorld", fn);
            return nullptr;
        }
        return world;
    }

    JoltBodyHandle *require_body(void *instance, const char *fn)
    {
        JoltBodyHandle *body = (JoltBodyHandle *)instance;
        if (!body || !body->valid || !body->world || !body->world->valid || body->id.IsInvalid())
        {
            Error("%s on invalid JoltBody", fn);
            return nullptr;
        }
        return body;
    }

    void detach_body_from_world(JoltBodyHandle *body)
    {
        if (!body || !body->world)
            return;

        std::vector<JoltBodyHandle *> &bodies = body->world->bodies;
        int slot = body->worldSlot;
        int last = (int)bodies.size() - 1;
        if (slot < 0 || slot > last)
            return;

        if (slot != last)
        {
            JoltBodyHandle *tail = bodies[(size_t)last];
            bodies[(size_t)slot] = tail;
            if (tail)
                tail->worldSlot = slot;
        }

        bodies.pop_back();
        body->worldSlot = -1;
    }

    void detach_constraint_from_world(JoltConstraintHandle *constraint)
    {
        if (!constraint || !constraint->world)
            return;

        std::vector<JoltConstraintHandle *> &constraints = constraint->world->constraints;
        int slot = constraint->worldSlot;
        int last = (int)constraints.size() - 1;
        if (slot < 0 || slot > last)
            return;

        if (slot != last)
        {
            JoltConstraintHandle *tail = constraints[(size_t)last];
            constraints[(size_t)slot] = tail;
            if (tail)
                tail->worldSlot = slot;
        }

        constraints.pop_back();
        constraint->worldSlot = -1;
    }

    void destroy_body_runtime(JoltBodyHandle *body)
    {
        if (!body || !body->valid)
            return;

        JoltWorldHandle *world = body->world;
        if (world && world->valid && !body->id.IsInvalid())
        {
            BodyInterface &bodyInterface = world->physicsSystem.GetBodyInterface();
            if (bodyInterface.IsAdded(body->id))
                bodyInterface.RemoveBody(body->id);
            bodyInterface.DestroyBody(body->id);
            detach_body_from_world(body);
        }

        body->world = nullptr;
        body->id = BodyID();
        body->worldSlot = -1;
        body->valid = false;
    }

    void destroy_constraint_runtime(JoltConstraintHandle *constraint)
    {
        if (!constraint || !constraint->valid || !constraint->constraint)
            return;

        JoltWorldHandle *world = constraint->world;
        if (world && world->valid)
        {
            if (constraint->subType == EConstraintSubType::Vehicle)
                world->physicsSystem.RemoveStepListener((VehicleConstraint *)constraint->constraint);
            world->physicsSystem.RemoveConstraint(constraint->constraint);
            detach_constraint_from_world(constraint);
        }

        constraint->world = nullptr;
        constraint->constraint = nullptr;
        constraint->worldSlot = -1;
        constraint->valid = false;
    }

    void destroy_world_runtime(JoltWorldHandle *world)
    {
        if (!world || !world->valid)
            return;

        for (JoltConstraintHandle *constraint : world->constraints)
        {
            if (constraint != nullptr && constraint->valid && constraint->constraint != nullptr)
            {
                if (constraint->subType == EConstraintSubType::Vehicle)
                    world->physicsSystem.RemoveStepListener((VehicleConstraint *)constraint->constraint);
                world->physicsSystem.RemoveConstraint(constraint->constraint);
                constraint->constraint = nullptr;
                constraint->worldSlot = -1;
                constraint->valid = false;
            }

            if (constraint != nullptr)
            {
                constraint->world = nullptr;
                if (!constraint->wrapperAlive)
                    delete constraint;
            }
        }

        world->constraints.clear();

        BodyInterface &bodyInterface = world->physicsSystem.GetBodyInterface();
        for (JoltBodyHandle *body : world->bodies)
        {
            if (body != nullptr && body->valid && !body->id.IsInvalid())
            {
                if (bodyInterface.IsAdded(body->id))
                    bodyInterface.RemoveBody(body->id);
                bodyInterface.DestroyBody(body->id);
                body->id = BodyID();
                body->worldSlot = -1;
                body->valid = false;
            }

            if (body != nullptr)
            {
                body->world = nullptr;
                if (!body->wrapperAlive)
                    delete body;
            }
        }

        world->bodies.clear();
        delete world->jobSystem;
        world->jobSystem = nullptr;
        delete world->tempAllocator;
        world->tempAllocator = nullptr;
        world->valid = false;
    }

    JoltBodyHandle *create_body_handle(JoltWorldHandle *world,
                                       const Shape *shape,
                                       const Vector3 &position,
                                       EMotionType motionType,
                                       ObjectLayer objectLayer,
                                       float friction,
                                       float restitution,
                                       float mass)
    {
        if (!world || !world->valid || !shape)
            return nullptr;

        BodyCreationSettings settings(shape,
                                      to_jolt_rvec3(position),
                                      Quat::sIdentity(),
                                      motionType,
                                      objectLayer);
        settings.mFriction = friction;
        settings.mRestitution = restitution;

        if (motionType == EMotionType::Dynamic)
        {
            settings.mMotionQuality = EMotionQuality::LinearCast;
            if (mass > 0.0f)
            {
                settings.mOverrideMassProperties = EOverrideMassProperties::CalculateInertia;
                settings.mMassPropertiesOverride.mMass = mass;
            }
        }

        BodyInterface &bodyInterface = world->physicsSystem.GetBodyInterface();
        BodyID id = bodyInterface.CreateAndAddBody(settings,
                                                   motionType == EMotionType::Static ? EActivation::DontActivate
                                                                                     : EActivation::Activate);
        if (id.IsInvalid())
        {
            Error("Failed to create Jolt body");
            return nullptr;
        }

        JoltBodyHandle *handle = new JoltBodyHandle();
        handle->world = world;
        handle->id = id;
        handle->worldSlot = (int)world->bodies.size();
        handle->valid = true;
        world->bodies.push_back(handle);
        return handle;
    }

    JoltConstraintHandle *create_constraint_handle(JoltWorldHandle *world,
                                                   Constraint *constraint,
                                                   EConstraintSubType subType)
    {
        if (!world || !world->valid || !constraint)
            return nullptr;

        world->physicsSystem.AddConstraint(constraint);
        if (subType == EConstraintSubType::Vehicle)
            world->physicsSystem.AddStepListener((VehicleConstraint *)constraint);

        JoltConstraintHandle *handle = new JoltConstraintHandle();
        handle->world = world;
        handle->constraint = constraint;
        handle->subType = subType;
        handle->worldSlot = (int)world->constraints.size();
        handle->valid = true;
        world->constraints.push_back(handle);
        return handle;
    }
}
