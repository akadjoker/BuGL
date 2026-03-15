#pragma once

#include "bindings.hpp"
#include "../../main/src/raymath.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/EPhysicsUpdateError.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Constraints/Constraint.h>
#include <Jolt/Physics/Constraints/MotorSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

#include <vector>

namespace JoltBindings
{
    using namespace JPH;

    namespace Layers
    {
        static constexpr ObjectLayer NON_MOVING = 0;
        static constexpr ObjectLayer MOVING = 1;
        static constexpr ObjectLayer NUM_LAYERS = 2;
    }

    namespace BroadPhaseLayers
    {
        static constexpr BroadPhaseLayer NON_MOVING(0);
        static constexpr BroadPhaseLayer MOVING(1);
        static constexpr uint NUM_LAYERS = 2;
    }

    class ObjectLayerPairFilterImpl final : public ObjectLayerPairFilter
    {
    public:
        bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
        {
            switch (inObject1)
            {
            case Layers::NON_MOVING:
                return inObject2 == Layers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                return false;
            }
        }
    };

    class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface
    {
    public:
        BPLayerInterfaceImpl()
        {
            mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
            mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
        }

        uint GetNumBroadPhaseLayers() const override
        {
            return BroadPhaseLayers::NUM_LAYERS;
        }

        BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override
        {
            return mObjectToBroadPhase[inLayer];
        }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        const char *GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
        {
            switch ((BroadPhaseLayer::Type)inLayer)
            {
            case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING: return "NON_MOVING";
            case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING: return "MOVING";
            default: return "UNKNOWN";
            }
        }
#endif

    private:
        BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
    };

    class ObjectVsBroadPhaseLayerFilterImpl final : public ObjectVsBroadPhaseLayerFilter
    {
    public:
        bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
        {
            switch (inLayer1)
            {
            case Layers::NON_MOVING:
                return inLayer2 == BroadPhaseLayers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                return false;
            }
        }
    };

    struct JoltBodyHandle;
    struct JoltConstraintHandle;

    struct JoltWorldHandle
    {
        bool valid = false;
        TempAllocatorImpl *tempAllocator = nullptr;
        JobSystemThreadPool *jobSystem = nullptr;
        BPLayerInterfaceImpl broadPhaseLayerInterface;
        ObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhaseLayerFilter;
        ObjectLayerPairFilterImpl objectLayerPairFilter;
        PhysicsSystem physicsSystem;
        std::vector<JoltBodyHandle *> bodies;
        std::vector<JoltConstraintHandle *> constraints;
    };

    struct JoltBodyHandle
    {
        JoltWorldHandle *world = nullptr;
        BodyID id;
        int worldSlot = -1;
        bool valid = false;
        bool wrapperAlive = false;
    };

    struct JoltConstraintHandle
    {
        JoltWorldHandle *world = nullptr;
        Constraint *constraint = nullptr;
        EConstraintSubType subType = EConstraintSubType::Fixed;
        int worldSlot = -1;
        bool valid = false;
        bool wrapperAlive = false;
    };

    extern NativeClassDef *g_worldClass;
    extern NativeClassDef *g_bodyClass;
    extern NativeStructDef *g_vector3Def;
    extern NativeStructDef *g_quaternionDef;
    extern bool g_joltRuntimeInitialized;

    bool ensure_jolt_runtime();
    void cleanup();

    int push_nil1(Interpreter *vm);

    bool read_struct_data(const Value &value, NativeStructDef *expectedDef, const char *typeName, const char *fn, int argIndex, void **out);
    bool read_number_arg(const Value &value, double *out, const char *fn, int argIndex);
    bool read_int_arg(const Value &value, int *out, const char *fn, int argIndex);
    bool read_vector3_arg(const Value &value, Vector3 *out, const char *fn, int argIndex);
    bool read_quaternion_arg(const Value &value, Quaternion *out, const char *fn, int argIndex);

    bool push_vector3(Interpreter *vm, const Vector3 &value);
    bool push_quaternion(Interpreter *vm, const Quaternion &value);
    bool push_body_handle(Interpreter *vm, JoltBodyHandle *body);

    NativeStructDef *get_native_struct_def(Interpreter *vm, const char *name);

    Vec3 to_jolt_vec3(const Vector3 &v);
    RVec3 to_jolt_rvec3(const Vector3 &v);
    Vector3 from_jolt_vec3(Vec3Arg v);
    Vector3 from_jolt_rvec3(RVec3Arg v);
    Quaternion from_jolt_quat(QuatArg q);

    JoltWorldHandle *require_world(void *instance, const char *fn);
    JoltBodyHandle *require_body(void *instance, const char *fn);

    void detach_body_from_world(JoltBodyHandle *body);
    void detach_constraint_from_world(JoltConstraintHandle *constraint);
    void destroy_body_runtime(JoltBodyHandle *body);
    void destroy_constraint_runtime(JoltConstraintHandle *constraint);
    void destroy_world_runtime(JoltWorldHandle *world);
    JoltBodyHandle *create_body_handle(JoltWorldHandle *world,
                                      const Shape *shape,
                                      const Vector3 &position,
                                      EMotionType motionType,
                                      ObjectLayer objectLayer,
                                      float friction,
                                      float restitution,
                                      float mass = 0.0f);
    JoltConstraintHandle *create_constraint_handle(JoltWorldHandle *world,
                                                  Constraint *constraint,
                                                  EConstraintSubType subType);

    void register_jolt_world(Interpreter &vm);
    void register_jolt_body(Interpreter &vm);
    void register_jolt_vehicle(Interpreter &vm);
}
