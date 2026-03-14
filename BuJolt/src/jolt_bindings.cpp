#include "jolt_core.hpp"
#include "jolt_joints.hpp"
#include "jolt_vehicle.hpp"

#include <Jolt/Physics/Vehicle/VehicleTransmission.h>

namespace JoltBindings
{
    void registerAll(Interpreter &vm)
    {
        ensure_jolt_runtime();

        g_vector3Def = get_native_struct_def(&vm, "Vector3");
        g_quaternionDef = get_native_struct_def(&vm, "Quaternion");

        register_jolt_world(vm);
        register_jolt_body(vm);
        register_jolt_joints(vm);
        register_jolt_vehicle(vm);

        ModuleBuilder module = vm.addModule("Jolt");
        module.addInt("JOLT_STATIC", (int)EMotionType::Static)
              .addInt("JOLT_KINEMATIC", (int)EMotionType::Kinematic)
              .addInt("JOLT_DYNAMIC", (int)EMotionType::Dynamic)
              .addInt("JOLT_CONSTRAINT_LOCAL", (int)EConstraintSpace::LocalToBodyCOM)
              .addInt("JOLT_CONSTRAINT_WORLD", (int)EConstraintSpace::WorldSpace)
              .addInt("JOLT_MOTOR_OFF", (int)EMotorState::Off)
              .addInt("JOLT_MOTOR_VELOCITY", (int)EMotorState::Velocity)
              .addInt("JOLT_MOTOR_POSITION", (int)EMotorState::Position)
              .addInt("JOLT_SUBTYPE_POINT", (int)EConstraintSubType::Point)
              .addInt("JOLT_SUBTYPE_HINGE", (int)EConstraintSubType::Hinge)
              .addInt("JOLT_SUBTYPE_DISTANCE", (int)EConstraintSubType::Distance)
              .addInt("JOLT_SUBTYPE_SLIDER", (int)EConstraintSubType::Slider)
              .addInt("JOLT_SUBTYPE_VEHICLE", (int)EConstraintSubType::Vehicle)
              .addInt("JOLT_TRANSMISSION_AUTO", (int)ETransmissionMode::Auto)
              .addInt("JOLT_TRANSMISSION_MANUAL", (int)ETransmissionMode::Manual)
              .addInt("JOLT_UPDATE_NONE", (int)EPhysicsUpdateError::None)
              .addInt("JOLT_UPDATE_MANIFOLD_CACHE_FULL", (int)EPhysicsUpdateError::ManifoldCacheFull)
              .addInt("JOLT_UPDATE_BODY_PAIR_CACHE_FULL", (int)EPhysicsUpdateError::BodyPairCacheFull)
              .addInt("JOLT_UPDATE_CONTACT_CONSTRAINTS_FULL", (int)EPhysicsUpdateError::ContactConstraintsFull);
    }
}
