#include "jolt_vehicle.hpp"
#include "jolt_core.hpp"

#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/Physics/Vehicle/VehicleCollisionTester.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>
#include <Jolt/Physics/Vehicle/MotorcycleController.h>
#include <Jolt/Physics/Vehicle/TrackedVehicleController.h>
#include <algorithm>
#include <vector>

namespace JoltBindings
{
    struct JoltWheelSettingsWVHandle
    {
        Ref<WheelSettingsWV> settings;
    };

    struct JoltWheelSettingsTVHandle
    {
        Ref<WheelSettingsTV> settings;
    };

    static NativeClassDef *g_wheelSettingsWVClass = nullptr;
    static NativeClassDef *g_wheelSettingsTVClass = nullptr;
    static NativeClassDef *g_wheeledVehicleClass = nullptr;
    static NativeClassDef *g_motorcycleClass = nullptr;
    static NativeClassDef *g_trackedVehicleClass = nullptr;

    static JoltBodyHandle *require_body_value_arg(const Value &value, const char *fn, int argIndex)
    {
        if (!value.isNativeClassInstance())
        {
            Error("%s arg %d expects JoltBody", fn, argIndex);
            return nullptr;
        }

        NativeClassInstance *inst = value.asNativeClassInstance();
        if (!inst || inst->klass != g_bodyClass || !inst->userData)
        {
            Error("%s arg %d expects JoltBody", fn, argIndex);
            return nullptr;
        }

        return require_body(inst->userData, fn);
    }

    static JoltWheelSettingsWVHandle *require_wheel_settings(void *instance, const char *fn)
    {
        JoltWheelSettingsWVHandle *wheel = (JoltWheelSettingsWVHandle *)instance;
        if (!wheel || !wheel->settings)
        {
            Error("%s on invalid JoltWheelSettingsWV", fn);
            return nullptr;
        }

        return wheel;
    }

    static JoltWheelSettingsTVHandle *require_tracked_wheel_settings(void *instance, const char *fn)
    {
        JoltWheelSettingsTVHandle *wheel = (JoltWheelSettingsTVHandle *)instance;
        if (!wheel || !wheel->settings)
        {
            Error("%s on invalid JoltWheelSettingsTV", fn);
            return nullptr;
        }

        return wheel;
    }

    static JoltConstraintHandle *require_vehicle(void *instance, const char *fn)
    {
        JoltConstraintHandle *vehicle = (JoltConstraintHandle *)instance;
        if (!vehicle || !vehicle->valid || !vehicle->world || !vehicle->world->valid || !vehicle->constraint)
        {
            Error("%s on invalid vehicle", fn);
            return nullptr;
        }

        if (vehicle->subType != EConstraintSubType::Vehicle)
        {
            Error("%s on wrong constraint type", fn);
            return nullptr;
        }

        return vehicle;
    }

    static VehicleConstraint *require_vehicle_constraint(void *instance, const char *fn)
    {
        JoltConstraintHandle *handle = require_vehicle(instance, fn);
        return handle ? (VehicleConstraint *)handle->constraint : nullptr;
    }

    static WheeledVehicleController *require_wheeled_controller(void *instance, const char *fn)
    {
        VehicleConstraint *vehicle = require_vehicle_constraint(instance, fn);
        if (!vehicle)
            return nullptr;

        VehicleController *controller = vehicle->GetController();
        if (!controller)
        {
            Error("%s missing controller", fn);
            return nullptr;
        }

        return (WheeledVehicleController *)controller;
    }

    static MotorcycleController *require_motorcycle_controller(void *instance, const char *fn)
    {
        VehicleConstraint *vehicle = require_vehicle_constraint(instance, fn);
        if (!vehicle)
            return nullptr;

        VehicleController *controller = vehicle->GetController();
        if (!controller)
        {
            Error("%s missing controller", fn);
            return nullptr;
        }

        return (MotorcycleController *)controller;
    }

    static TrackedVehicleController *require_tracked_controller(void *instance, const char *fn)
    {
        VehicleConstraint *vehicle = require_vehicle_constraint(instance, fn);
        if (!vehicle)
            return nullptr;

        VehicleController *controller = vehicle->GetController();
        if (!controller)
        {
            Error("%s missing controller", fn);
            return nullptr;
        }

        return (TrackedVehicleController *)controller;
    }

    static bool push_vehicle_handle(Interpreter *vm, NativeClassDef *klass, JoltConstraintHandle *handle)
    {
        if (!vm || !klass || !handle)
            return false;

        Value value = vm->makeNativeClassInstance(false);
        NativeClassInstance *inst = value.asNativeClassInstance();
        if (!inst)
            return false;

        inst->klass = klass;
        inst->userData = handle;
        handle->wrapperAlive = true;
        vm->push(value);
        return true;
    }

    static bool read_wheel_settings_array(const Value &value, JPH::Array<Ref<WheelSettings>> *out, const char *fn, int argIndex)
    {
        if (!out || !value.isArray())
        {
            Error("%s arg %d expects array of JoltWheelSettingsWV", fn, argIndex);
            return false;
        }

        ArrayInstance *arr = value.asArray();
        if (!arr)
        {
            Error("%s arg %d expects array of JoltWheelSettingsWV", fn, argIndex);
            return false;
        }

        out->clear();
        out->reserve(arr->values.size());
        for (size_t i = 0; i < arr->values.size(); ++i)
        {
            const Value &item = arr->values[i];
            if (!item.isNativeClassInstance())
            {
                Error("%s arg %d item %d expects JoltWheelSettingsWV", fn, argIndex, (int)i + 1);
                return false;
            }

            NativeClassInstance *inst = item.asNativeClassInstance();
            if (!inst || inst->klass != g_wheelSettingsWVClass || !inst->userData)
            {
                Error("%s arg %d item %d expects JoltWheelSettingsWV", fn, argIndex, (int)i + 1);
                return false;
            }

            JoltWheelSettingsWVHandle *wheel = (JoltWheelSettingsWVHandle *)inst->userData;
            if (!wheel || !wheel->settings)
            {
                Error("%s arg %d item %d is invalid", fn, argIndex, (int)i + 1);
                return false;
            }

            out->push_back(Ref<WheelSettings>(wheel->settings.GetPtr()));
        }

        return true;
    }

    static bool read_tracked_wheel_settings_array(const Value &value, JPH::Array<Ref<WheelSettings>> *out, const char *fn, int argIndex)
    {
        if (!out || !value.isArray())
        {
            Error("%s arg %d expects array of JoltWheelSettingsTV", fn, argIndex);
            return false;
        }

        ArrayInstance *arr = value.asArray();
        if (!arr)
        {
            Error("%s arg %d expects array of JoltWheelSettingsTV", fn, argIndex);
            return false;
        }

        out->clear();
        out->reserve(arr->values.size());
        for (size_t i = 0; i < arr->values.size(); ++i)
        {
            const Value &item = arr->values[i];
            if (!item.isNativeClassInstance())
            {
                Error("%s arg %d item %d expects JoltWheelSettingsTV", fn, argIndex, (int)i + 1);
                return false;
            }

            NativeClassInstance *inst = item.asNativeClassInstance();
            if (!inst || inst->klass != g_wheelSettingsTVClass || !inst->userData)
            {
                Error("%s arg %d item %d expects JoltWheelSettingsTV", fn, argIndex, (int)i + 1);
                return false;
            }

            JoltWheelSettingsTVHandle *wheel = (JoltWheelSettingsTVHandle *)inst->userData;
            if (!wheel || !wheel->settings)
            {
                Error("%s arg %d item %d is invalid", fn, argIndex, (int)i + 1);
                return false;
            }

            out->push_back(Ref<WheelSettings>(wheel->settings.GetPtr()));
        }

        return true;
    }

    static bool read_number_array_arg(const Value &value, std::vector<float> *out, const char *fn, int argIndex)
    {
        if (!out || !value.isArray())
        {
            Error("%s arg %d expects number array", fn, argIndex);
            return false;
        }

        ArrayInstance *arr = value.asArray();
        if (!arr)
        {
            Error("%s arg %d expects number array", fn, argIndex);
            return false;
        }

        out->clear();
        out->reserve(arr->values.size());
        for (size_t i = 0; i < arr->values.size(); ++i)
        {
            if (!arr->values[i].isNumber())
            {
                Error("%s arg %d item %d expects number", fn, argIndex, (int)i + 1);
                return false;
            }

            out->push_back((float)arr->values[i].asNumber());
        }

        return true;
    }

    static void configure_default_differentials(VehicleControllerSettings *controller_settings,
                                                const JPH::Array<Ref<WheelSettings>> &wheels,
                                                bool motorcycle)
    {
        if (!controller_settings || wheels.empty())
            return;

        auto *wheeled = static_cast<WheeledVehicleControllerSettings *>(controller_settings);
        if (!wheeled || !wheeled->mDifferentials.empty())
            return;

        std::vector<int> indices;
        indices.reserve(wheels.size());
        for (size_t i = 0; i < wheels.size(); ++i)
            indices.push_back((int)i);

        auto left_before_right = [&wheels](int a, int b) {
            return wheels[(size_t)a]->mPosition.GetX() < wheels[(size_t)b]->mPosition.GetX();
        };

        auto front_before_back = [&wheels](int a, int b) {
            float az = wheels[(size_t)a]->mPosition.GetZ();
            float bz = wheels[(size_t)b]->mPosition.GetZ();
            if (az != bz)
                return az > bz;
            return wheels[(size_t)a]->mPosition.GetX() < wheels[(size_t)b]->mPosition.GetX();
        };

        auto add_differential = [&wheeled](int left_wheel, int right_wheel, float torque_ratio) {
            VehicleDifferentialSettings diff;
            diff.mLeftWheel = left_wheel;
            diff.mRightWheel = right_wheel;
            diff.mEngineTorqueRatio = torque_ratio;
            wheeled->mDifferentials.push_back(diff);
        };

        if (indices.size() == 1)
        {
            add_differential(indices[0], -1, 1.0f);
            return;
        }

        if (indices.size() == 2)
        {
            if (motorcycle)
            {
                int front = indices[0];
                int rear = indices[1];
                if (!front_before_back(front, rear))
                    std::swap(front, rear);

                VehicleDifferentialSettings diff;
                diff.mLeftWheel = -1;
                diff.mRightWheel = rear;
                diff.mDifferentialRatio = 1.93f * 40.0f / 16.0f;
                diff.mEngineTorqueRatio = 1.0f;
                wheeled->mDifferentials.push_back(diff);
            }
            else
            {
                if (!left_before_right(indices[0], indices[1]))
                    std::swap(indices[0], indices[1]);
                add_differential(indices[0], indices[1], 1.0f);
            }
            return;
        }

        std::sort(indices.begin(), indices.end(), front_before_back);

        if (indices.size() == 4)
        {
            int front_left = indices[0];
            int front_right = indices[1];
            int rear_left = indices[2];
            int rear_right = indices[3];

            if (!left_before_right(front_left, front_right))
                std::swap(front_left, front_right);
            if (!left_before_right(rear_left, rear_right))
                std::swap(rear_left, rear_right);

            add_differential(front_left, front_right, 0.5f);
            add_differential(rear_left, rear_right, 0.5f);
            return;
        }

        int pair_count = (int)(indices.size() + 1) / 2;
        float torque_ratio = 1.0f / (float)pair_count;
        for (size_t i = 0; i < indices.size(); i += 2)
        {
            int left_wheel = indices[i];
            int right_wheel = i + 1 < indices.size() ? indices[i + 1] : -1;
            if (right_wheel != -1 && !left_before_right(left_wheel, right_wheel))
                std::swap(left_wheel, right_wheel);
            add_differential(left_wheel, right_wheel, torque_ratio);
        }
    }

    static void configure_default_powertrain(VehicleControllerSettings *controller_settings, bool motorcycle)
    {
        if (!controller_settings)
            return;

        auto *wheeled = static_cast<WheeledVehicleControllerSettings *>(controller_settings);
        if (!wheeled)
            return;

        if (wheeled->mEngine.mMaxTorque == 500.0f)
            wheeled->mEngine.mMaxTorque = motorcycle ? 150.0f : 1400.0f;

        if (motorcycle && wheeled->mEngine.mMinRPM == 1000.0f)
            wheeled->mEngine.mMinRPM = 1000.0f;

        if (motorcycle && wheeled->mEngine.mMaxRPM == 6000.0f)
            wheeled->mEngine.mMaxRPM = 10000.0f;

        if (wheeled->mTransmission.mClutchStrength == 10.0f)
            wheeled->mTransmission.mClutchStrength = motorcycle ? 2.0f : 26.0f;

        if (wheeled->mTransmission.mGearRatios.size() == 5
            && wheeled->mTransmission.mGearRatios[0] == 2.66f
            && wheeled->mTransmission.mGearRatios[1] == 1.78f)
        {
            if (motorcycle)
                wheeled->mTransmission.mGearRatios = { 2.27f, 1.63f, 1.3f, 1.09f, 0.96f, 0.88f };
            else
                wheeled->mTransmission.mGearRatios = { 3.80f, 2.35f, 1.60f, 1.16f, 0.92f };
        }

        if (wheeled->mTransmission.mReverseGearRatios.size() == 1
            && wheeled->mTransmission.mReverseGearRatios[0] == -2.90f)
        {
            wheeled->mTransmission.mReverseGearRatios = { motorcycle ? -4.0f : -3.20f };
        }

        if (wheeled->mTransmission.mShiftUpRPM == 4000.0f)
            wheeled->mTransmission.mShiftUpRPM = motorcycle ? 8000.0f : 5000.0f;

        if (wheeled->mTransmission.mShiftDownRPM == 2000.0f)
            wheeled->mTransmission.mShiftDownRPM = motorcycle ? 2000.0f : 1800.0f;
    }

    static bool configure_default_tracks(TrackedVehicleControllerSettings *controller_settings,
                                         const JPH::Array<Ref<WheelSettings>> &wheels,
                                         const char *fn)
    {
        if (!controller_settings)
            return false;

        std::vector<int> left_indices;
        std::vector<int> right_indices;
        left_indices.reserve(wheels.size());
        right_indices.reserve(wheels.size());

        for (size_t i = 0; i < wheels.size(); ++i)
        {
            if (wheels[i]->mPosition.GetX() >= 0.0f)
                left_indices.push_back((int)i);
            else
                right_indices.push_back((int)i);
        }

        if (left_indices.empty() || right_indices.empty())
        {
            Error("%s needs wheels on both left and right sides", fn);
            return false;
        }

        auto front_before_back = [&wheels](int a, int b) {
            return wheels[(size_t)a]->mPosition.GetZ() > wheels[(size_t)b]->mPosition.GetZ();
        };

        std::sort(left_indices.begin(), left_indices.end(), front_before_back);
        std::sort(right_indices.begin(), right_indices.end(), front_before_back);

        VehicleTrackSettings &left_track = controller_settings->mTracks[(int)ETrackSide::Left];
        VehicleTrackSettings &right_track = controller_settings->mTracks[(int)ETrackSide::Right];
        left_track.mWheels.clear();
        right_track.mWheels.clear();

        for (int index : left_indices)
            left_track.mWheels.push_back((uint)index);
        for (int index : right_indices)
            right_track.mWheels.push_back((uint)index);

        left_track.mDrivenWheel = (uint)left_indices.back();
        right_track.mDrivenWheel = (uint)right_indices.back();
        return true;
    }

    static int vehicle_destroy(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("Vehicle.destroy() expects 0 arguments");
            return 0;
        }

        destroy_constraint_runtime((JoltConstraintHandle *)instance);
        return 0;
    }

    static int vehicle_is_valid(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("Vehicle.isValid() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *vehicle = (JoltConstraintHandle *)instance;
        vm->pushBool(vehicle != nullptr && vehicle->valid && vehicle->constraint != nullptr && vehicle->world != nullptr && vehicle->world->valid);
        return 1;
    }

    static int vehicle_get_wheel_count(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("Vehicle.getWheelCount() expects 0 arguments");
            return push_nil1(vm);
        }

        VehicleConstraint *vehicle = require_vehicle_constraint(instance, "Vehicle.getWheelCount()");
        if (!vehicle)
            return push_nil1(vm);

        vm->pushInt((int)vehicle->GetWheels().size());
        return 1;
    }

    static Wheel *require_wheel(VehicleConstraint *vehicle, const Value &value, const char *fn)
    {
        if (!vehicle || !value.isNumber())
        {
            Error("%s expects wheel index", fn);
            return nullptr;
        }

        int index = value.asInt();
        if (index < 0 || index >= (int)vehicle->GetWheels().size())
        {
            Error("%s wheel index out of range", fn);
            return nullptr;
        }

        return vehicle->GetWheel((uint)index);
    }

    static int vehicle_get_wheel_world_position(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("Vehicle.getWheelWorldPosition() expects (index)");
            return push_nil1(vm);
        }

        VehicleConstraint *vehicle = require_vehicle_constraint(instance, "Vehicle.getWheelWorldPosition()");
        if (!vehicle)
            return push_nil1(vm);

        int index = args[0].asInt();
        if (index < 0 || index >= (int)vehicle->GetWheels().size())
        {
            Error("Vehicle.getWheelWorldPosition() wheel index out of range");
            return push_nil1(vm);
        }

        RMat44 transform = vehicle->GetWheelWorldTransform((uint)index, Vec3::sAxisX(), Vec3::sAxisY());
        return push_vector3(vm, from_jolt_rvec3(transform.GetTranslation())) ? 1 : push_nil1(vm);
    }

    static int vehicle_get_wheel_world_pose(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 3)
        {
            Error("Vehicle.getWheelWorldPose() expects (index, wheelRight, wheelUp)");
            vm->pushNil();
            vm->pushNil();
            return 2;
        }

        VehicleConstraint *vehicle = require_vehicle_constraint(instance, "Vehicle.getWheelWorldPose()");
        if (!vehicle)
        {
            vm->pushNil();
            vm->pushNil();
            return 2;
        }

        int index = -1;
        Vector3 wheelRight;
        Vector3 wheelUp;
        if (!read_int_arg(args[0], &index, "Vehicle.getWheelWorldPose()", 1) ||
            !read_vector3_arg(args[1], &wheelRight, "Vehicle.getWheelWorldPose()", 2) ||
            !read_vector3_arg(args[2], &wheelUp, "Vehicle.getWheelWorldPose()", 3))
        {
            vm->pushNil();
            vm->pushNil();
            return 2;
        }

        if (index < 0 || index >= (int)vehicle->GetWheels().size())
        {
            Error("Vehicle.getWheelWorldPose() wheel index out of range");
            vm->pushNil();
            vm->pushNil();
            return 2;
        }

        RMat44 transform = vehicle->GetWheelWorldTransform((uint)index, to_jolt_vec3(wheelRight), to_jolt_vec3(wheelUp));
        push_vector3(vm, from_jolt_rvec3(transform.GetTranslation()));
        push_quaternion(vm, from_jolt_quat(transform.GetQuaternion()));
        return 2;
    }

    static int vehicle_has_wheel_contact(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("Vehicle.hasWheelContact() expects (index)");
            return push_nil1(vm);
        }

        VehicleConstraint *vehicle = require_vehicle_constraint(instance, "Vehicle.hasWheelContact()");
        if (!vehicle)
            return push_nil1(vm);

        Wheel *wheel = require_wheel(vehicle, args[0], "Vehicle.hasWheelContact()");
        if (!wheel)
            return push_nil1(vm);

        vm->pushBool(wheel->HasContact());
        return 1;
    }

    static int vehicle_get_wheel_contact_position(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("Vehicle.getWheelContactPosition() expects (index)");
            return push_nil1(vm);
        }

        VehicleConstraint *vehicle = require_vehicle_constraint(instance, "Vehicle.getWheelContactPosition()");
        if (!vehicle)
            return push_nil1(vm);

        Wheel *wheel = require_wheel(vehicle, args[0], "Vehicle.getWheelContactPosition()");
        if (!wheel || !wheel->HasContact())
            return push_nil1(vm);

        return push_vector3(vm, from_jolt_rvec3(wheel->GetContactPosition())) ? 1 : push_nil1(vm);
    }

    static int vehicle_get_wheel_contact_normal(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("Vehicle.getWheelContactNormal() expects (index)");
            return push_nil1(vm);
        }

        VehicleConstraint *vehicle = require_vehicle_constraint(instance, "Vehicle.getWheelContactNormal()");
        if (!vehicle)
            return push_nil1(vm);

        Wheel *wheel = require_wheel(vehicle, args[0], "Vehicle.getWheelContactNormal()");
        if (!wheel || !wheel->HasContact())
            return push_nil1(vm);

        return push_vector3(vm, from_jolt_vec3(wheel->GetContactNormal())) ? 1 : push_nil1(vm);
    }

    static int vehicle_get_wheel_suspension_length(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("Vehicle.getWheelSuspensionLength() expects (index)");
            return push_nil1(vm);
        }

        VehicleConstraint *vehicle = require_vehicle_constraint(instance, "Vehicle.getWheelSuspensionLength()");
        if (!vehicle)
            return push_nil1(vm);

        Wheel *wheel = require_wheel(vehicle, args[0], "Vehicle.getWheelSuspensionLength()");
        if (!wheel)
            return push_nil1(vm);

        vm->pushDouble((double)wheel->GetSuspensionLength());
        return 1;
    }

    static int vehicle_get_wheel_rotation_angle(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("Vehicle.getWheelRotationAngle() expects (index)");
            return push_nil1(vm);
        }

        VehicleConstraint *vehicle = require_vehicle_constraint(instance, "Vehicle.getWheelRotationAngle()");
        if (!vehicle)
            return push_nil1(vm);

        Wheel *wheel = require_wheel(vehicle, args[0], "Vehicle.getWheelRotationAngle()");
        if (!wheel)
            return push_nil1(vm);

        vm->pushDouble((double)wheel->GetRotationAngle());
        return 1;
    }

    static int vehicle_get_wheel_steer_angle(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("Vehicle.getWheelSteerAngle() expects (index)");
            return push_nil1(vm);
        }

        VehicleConstraint *vehicle = require_vehicle_constraint(instance, "Vehicle.getWheelSteerAngle()");
        if (!vehicle)
            return push_nil1(vm);

        Wheel *wheel = require_wheel(vehicle, args[0], "Vehicle.getWheelSteerAngle()");
        if (!wheel)
            return push_nil1(vm);

        vm->pushDouble((double)wheel->GetSteerAngle());
        return 1;
    }

    static int vehicle_set_max_pitch_roll_angle(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("Vehicle.setMaxPitchRollAngle() expects (number)");
            return 0;
        }

        VehicleConstraint *vehicle = require_vehicle_constraint(instance, "Vehicle.setMaxPitchRollAngle()");
        if (!vehicle)
            return 0;

        double angle = 0.0;
        if (!read_number_arg(args[0], &angle, "Vehicle.setMaxPitchRollAngle()", 1))
            return 0;

        vehicle->SetMaxPitchRollAngle((float)angle);
        return 0;
    }

    static int vehicle_get_max_pitch_roll_angle(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("Vehicle.getMaxPitchRollAngle() expects 0 arguments");
            return push_nil1(vm);
        }

        VehicleConstraint *vehicle = require_vehicle_constraint(instance, "Vehicle.getMaxPitchRollAngle()");
        if (!vehicle)
            return push_nil1(vm);

        vm->pushDouble((double)vehicle->GetMaxPitchRollAngle());
        return 1;
    }

    static int vehicle_override_gravity(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("Vehicle.overrideGravity() expects (Vector3)");
            return 0;
        }

        VehicleConstraint *vehicle = require_vehicle_constraint(instance, "Vehicle.overrideGravity()");
        if (!vehicle)
            return 0;

        Vector3 gravity;
        if (!read_vector3_arg(args[0], &gravity, "Vehicle.overrideGravity()", 1))
            return 0;

        vehicle->OverrideGravity(to_jolt_vec3(gravity));
        return 0;
    }

    static int vehicle_reset_gravity_override(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("Vehicle.resetGravityOverride() expects 0 arguments");
            return 0;
        }

        VehicleConstraint *vehicle = require_vehicle_constraint(instance, "Vehicle.resetGravityOverride()");
        if (!vehicle)
            return 0;

        vehicle->ResetGravityOverride();
        return 0;
    }

    static int vehicle_set_num_steps_active(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("Vehicle.setNumStepsBetweenCollisionTestActive() expects (int)");
            return 0;
        }

        VehicleConstraint *vehicle = require_vehicle_constraint(instance, "Vehicle.setNumStepsBetweenCollisionTestActive()");
        if (!vehicle)
            return 0;

        vehicle->SetNumStepsBetweenCollisionTestActive((uint)args[0].asInt());
        return 0;
    }

    static int vehicle_set_num_steps_inactive(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("Vehicle.setNumStepsBetweenCollisionTestInactive() expects (int)");
            return 0;
        }

        VehicleConstraint *vehicle = require_vehicle_constraint(instance, "Vehicle.setNumStepsBetweenCollisionTestInactive()");
        if (!vehicle)
            return 0;

        vehicle->SetNumStepsBetweenCollisionTestInactive((uint)args[0].asInt());
        return 0;
    }

    static int vehicle_set_collision_tester_ray(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        VehicleConstraint *vehicle = require_vehicle_constraint(instance, "Vehicle.setCollisionTesterRay()");
        if (!vehicle)
            return 0;

        Vec3 up = Vec3::sAxisY();
        float maxSlope = DegreesToRadians(80.0f);
        if (argCount == 2)
        {
            Vector3 upArg;
            double slopeArg = 0.0;
            if (!read_vector3_arg(args[0], &upArg, "Vehicle.setCollisionTesterRay()", 1) ||
                !read_number_arg(args[1], &slopeArg, "Vehicle.setCollisionTesterRay()", 2))
                return 0;
            up = to_jolt_vec3(upArg);
            maxSlope = (float)slopeArg;
        }
        else if (argCount != 0)
        {
            Error("Vehicle.setCollisionTesterRay() expects () or (up, maxSlopeAngle)");
            return 0;
        }

        vehicle->SetVehicleCollisionTester(new VehicleCollisionTesterRay(Layers::MOVING, up, maxSlope));
        return 0;
    }

    static int vehicle_set_collision_tester_sphere(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        VehicleConstraint *vehicle = require_vehicle_constraint(instance, "Vehicle.setCollisionTesterSphere()");
        if (!vehicle)
            return 0;

        double radius = 0.0;
        Vec3 up = Vec3::sAxisY();
        float maxSlope = DegreesToRadians(80.0f);
        if (argCount != 1 && argCount != 3)
        {
            Error("Vehicle.setCollisionTesterSphere() expects (radius) or (radius, up, maxSlopeAngle)");
            return 0;
        }

        if (!read_number_arg(args[0], &radius, "Vehicle.setCollisionTesterSphere()", 1))
            return 0;

        if (argCount == 3)
        {
            Vector3 upArg;
            double slopeArg = 0.0;
            if (!read_vector3_arg(args[1], &upArg, "Vehicle.setCollisionTesterSphere()", 2) ||
                !read_number_arg(args[2], &slopeArg, "Vehicle.setCollisionTesterSphere()", 3))
                return 0;
            up = to_jolt_vec3(upArg);
            maxSlope = (float)slopeArg;
        }

        vehicle->SetVehicleCollisionTester(new VehicleCollisionTesterCastSphere(Layers::MOVING, (float)radius, up, maxSlope));
        return 0;
    }

    static int vehicle_set_collision_tester_cylinder(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        VehicleConstraint *vehicle = require_vehicle_constraint(instance, "Vehicle.setCollisionTesterCylinder()");
        if (!vehicle)
            return 0;

        double fraction = 0.1;
        if (argCount == 1)
        {
            if (!read_number_arg(args[0], &fraction, "Vehicle.setCollisionTesterCylinder()", 1))
                return 0;
        }
        else if (argCount != 0)
        {
            Error("Vehicle.setCollisionTesterCylinder() expects () or (convexRadiusFraction)");
            return 0;
        }

        vehicle->SetVehicleCollisionTester(new VehicleCollisionTesterCastCylinder(Layers::MOVING, (float)fraction));
        return 0;
    }

    static int wheeled_set_driver_input(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 4)
        {
            Error("Vehicle.setDriverInput() expects (forward, right, brake, handBrake)");
            return 0;
        }

        WheeledVehicleController *controller = require_wheeled_controller(instance, "Vehicle.setDriverInput()");
        if (!controller)
            return 0;

        double forward = 0.0;
        double right = 0.0;
        double brake = 0.0;
        double handBrake = 0.0;
        if (!read_number_arg(args[0], &forward, "Vehicle.setDriverInput()", 1) ||
            !read_number_arg(args[1], &right, "Vehicle.setDriverInput()", 2) ||
            !read_number_arg(args[2], &brake, "Vehicle.setDriverInput()", 3) ||
            !read_number_arg(args[3], &handBrake, "Vehicle.setDriverInput()", 4))
            return 0;

        controller->SetDriverInput((float)forward, (float)right, (float)brake, (float)handBrake);
        return 0;
    }

    static int wheeled_set_forward_input(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("Vehicle.setForwardInput() expects (number)");
            return 0;
        }

        WheeledVehicleController *controller = require_wheeled_controller(instance, "Vehicle.setForwardInput()");
        if (!controller)
            return 0;

        double value = 0.0;
        if (!read_number_arg(args[0], &value, "Vehicle.setForwardInput()", 1))
            return 0;

        controller->SetForwardInput((float)value);
        return 0;
    }

    static int wheeled_set_right_input(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("Vehicle.setRightInput() expects (number)");
            return 0;
        }

        WheeledVehicleController *controller = require_wheeled_controller(instance, "Vehicle.setRightInput()");
        if (!controller)
            return 0;

        double value = 0.0;
        if (!read_number_arg(args[0], &value, "Vehicle.setRightInput()", 1))
            return 0;

        controller->SetRightInput((float)value);
        return 0;
    }

    static int wheeled_set_brake_input(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("Vehicle.setBrakeInput() expects (number)");
            return 0;
        }

        WheeledVehicleController *controller = require_wheeled_controller(instance, "Vehicle.setBrakeInput()");
        if (!controller)
            return 0;

        double value = 0.0;
        if (!read_number_arg(args[0], &value, "Vehicle.setBrakeInput()", 1))
            return 0;

        controller->SetBrakeInput((float)value);
        return 0;
    }

    static int wheeled_set_hand_brake_input(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("Vehicle.setHandBrakeInput() expects (number)");
            return 0;
        }

        WheeledVehicleController *controller = require_wheeled_controller(instance, "Vehicle.setHandBrakeInput()");
        if (!controller)
            return 0;

        double value = 0.0;
        if (!read_number_arg(args[0], &value, "Vehicle.setHandBrakeInput()", 1))
            return 0;

        controller->SetHandBrakeInput((float)value);
        return 0;
    }

    static int wheeled_get_engine_rpm(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("Vehicle.getEngineRPM() expects 0 arguments");
            return push_nil1(vm);
        }

        WheeledVehicleController *controller = require_wheeled_controller(instance, "Vehicle.getEngineRPM()");
        if (!controller)
            return push_nil1(vm);

        vm->pushDouble((double)controller->GetEngine().GetCurrentRPM());
        return 1;
    }

    static int wheeled_get_current_gear(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("Vehicle.getCurrentGear() expects 0 arguments");
            return push_nil1(vm);
        }

        WheeledVehicleController *controller = require_wheeled_controller(instance, "Vehicle.getCurrentGear()");
        if (!controller)
            return push_nil1(vm);

        vm->pushInt(controller->GetTransmission().GetCurrentGear());
        return 1;
    }

    static int wheeled_get_clutch_friction(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("Vehicle.getClutchFriction() expects 0 arguments");
            return push_nil1(vm);
        }

        WheeledVehicleController *controller = require_wheeled_controller(instance, "Vehicle.getClutchFriction()");
        if (!controller)
            return push_nil1(vm);

        vm->pushDouble((double)controller->GetTransmission().GetClutchFriction());
        return 1;
    }

    static int wheeled_set_engine_max_torque(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("Vehicle.setEngineMaxTorque() expects (number)");
            return 0;
        }

        WheeledVehicleController *controller = require_wheeled_controller(instance, "Vehicle.setEngineMaxTorque()");
        if (!controller)
            return 0;

        double value = 0.0;
        if (!read_number_arg(args[0], &value, "Vehicle.setEngineMaxTorque()", 1))
            return 0;

        controller->GetEngine().mMaxTorque = (float)value;
        return 0;
    }

    static int wheeled_set_engine_rpm_range(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("Vehicle.setEngineRPMRange() expects (minRPM, maxRPM)");
            return 0;
        }

        WheeledVehicleController *controller = require_wheeled_controller(instance, "Vehicle.setEngineRPMRange()");
        if (!controller)
            return 0;

        double min_rpm = 0.0;
        double max_rpm = 0.0;
        if (!read_number_arg(args[0], &min_rpm, "Vehicle.setEngineRPMRange()", 1) ||
            !read_number_arg(args[1], &max_rpm, "Vehicle.setEngineRPMRange()", 2))
            return 0;

        controller->GetEngine().mMinRPM = (float)min_rpm;
        controller->GetEngine().mMaxRPM = (float)max_rpm;
        return 0;
    }

    static int wheeled_set_shift_rpm(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("Vehicle.setShiftRPM() expects (downRPM, upRPM)");
            return 0;
        }

        WheeledVehicleController *controller = require_wheeled_controller(instance, "Vehicle.setShiftRPM()");
        if (!controller)
            return 0;

        double down_rpm = 0.0;
        double up_rpm = 0.0;
        if (!read_number_arg(args[0], &down_rpm, "Vehicle.setShiftRPM()", 1) ||
            !read_number_arg(args[1], &up_rpm, "Vehicle.setShiftRPM()", 2))
            return 0;

        controller->GetTransmission().mShiftDownRPM = (float)down_rpm;
        controller->GetTransmission().mShiftUpRPM = (float)up_rpm;
        return 0;
    }

    static int wheeled_set_clutch_strength(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("Vehicle.setClutchStrength() expects (number)");
            return 0;
        }

        WheeledVehicleController *controller = require_wheeled_controller(instance, "Vehicle.setClutchStrength()");
        if (!controller)
            return 0;

        double value = 0.0;
        if (!read_number_arg(args[0], &value, "Vehicle.setClutchStrength()", 1))
            return 0;

        controller->GetTransmission().mClutchStrength = (float)value;
        return 0;
    }

    static int wheeled_set_gear_ratios(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("Vehicle.setGearRatios() expects (numberArray)");
            return 0;
        }

        WheeledVehicleController *controller = require_wheeled_controller(instance, "Vehicle.setGearRatios()");
        if (!controller)
            return 0;

        std::vector<float> values;
        if (!read_number_array_arg(args[0], &values, "Vehicle.setGearRatios()", 1) || values.empty())
        {
            Error("Vehicle.setGearRatios() expects non-empty numberArray");
            return 0;
        }

        controller->GetTransmission().mGearRatios.clear();
        for (float value : values)
            controller->GetTransmission().mGearRatios.push_back(value);
        return 0;
    }

    static int wheeled_set_reverse_gear_ratios(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("Vehicle.setReverseGearRatios() expects (numberArray)");
            return 0;
        }

        WheeledVehicleController *controller = require_wheeled_controller(instance, "Vehicle.setReverseGearRatios()");
        if (!controller)
            return 0;

        std::vector<float> values;
        if (!read_number_array_arg(args[0], &values, "Vehicle.setReverseGearRatios()", 1) || values.empty())
        {
            Error("Vehicle.setReverseGearRatios() expects non-empty numberArray");
            return 0;
        }

        controller->GetTransmission().mReverseGearRatios.clear();
        for (float value : values)
            controller->GetTransmission().mReverseGearRatios.push_back(value);
        return 0;
    }

    static int wheeled_clear_differentials(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("Vehicle.clearDifferentials() expects 0 arguments");
            return 0;
        }

        WheeledVehicleController *controller = require_wheeled_controller(instance, "Vehicle.clearDifferentials()");
        if (!controller)
            return 0;

        controller->GetDifferentials().clear();
        return 0;
    }

    static int wheeled_add_differential(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 3 && argCount != 4)
        {
            Error("Vehicle.addDifferential() expects (leftWheel, rightWheel, differentialRatio[, engineTorqueRatio])");
            return 0;
        }

        WheeledVehicleController *controller = require_wheeled_controller(instance, "Vehicle.addDifferential()");
        if (!controller)
            return 0;

        int left_wheel = 0;
        int right_wheel = 0;
        double differential_ratio = 0.0;
        double torque_ratio = 1.0;
        if (!read_int_arg(args[0], &left_wheel, "Vehicle.addDifferential()", 1) ||
            !read_int_arg(args[1], &right_wheel, "Vehicle.addDifferential()", 2) ||
            !read_number_arg(args[2], &differential_ratio, "Vehicle.addDifferential()", 3))
            return 0;

        if (argCount == 4 && !read_number_arg(args[3], &torque_ratio, "Vehicle.addDifferential()", 4))
            return 0;

        VehicleDifferentialSettings diff;
        diff.mLeftWheel = left_wheel;
        diff.mRightWheel = right_wheel;
        diff.mDifferentialRatio = (float)differential_ratio;
        diff.mEngineTorqueRatio = (float)torque_ratio;
        controller->GetDifferentials().push_back(diff);
        return 0;
    }

    static int wheeled_set_transmission_mode(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("Vehicle.setTransmissionMode() expects (int)");
            return 0;
        }

        WheeledVehicleController *controller = require_wheeled_controller(instance, "Vehicle.setTransmissionMode()");
        if (!controller)
            return 0;

        controller->GetTransmission().mMode = (ETransmissionMode)args[0].asInt();
        return 0;
    }

    static int wheeled_get_transmission_mode(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("Vehicle.getTransmissionMode() expects 0 arguments");
            return push_nil1(vm);
        }

        WheeledVehicleController *controller = require_wheeled_controller(instance, "Vehicle.getTransmissionMode()");
        if (!controller)
            return push_nil1(vm);

        vm->pushInt((int)controller->GetTransmission().mMode);
        return 1;
    }

    static int wheeled_set_transmission(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2 || !args[0].isNumber())
        {
            Error("Vehicle.setTransmission() expects (gear, clutchFriction)");
            return 0;
        }

        WheeledVehicleController *controller = require_wheeled_controller(instance, "Vehicle.setTransmission()");
        if (!controller)
            return 0;

        double clutch = 0.0;
        if (!read_number_arg(args[1], &clutch, "Vehicle.setTransmission()", 2))
            return 0;

        controller->GetTransmission().Set(args[0].asInt(), (float)clutch);
        return 0;
    }

    static int wheeled_set_differential_limited_slip_ratio(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("Vehicle.setDifferentialLimitedSlipRatio() expects (number)");
            return 0;
        }

        WheeledVehicleController *controller = require_wheeled_controller(instance, "Vehicle.setDifferentialLimitedSlipRatio()");
        if (!controller)
            return 0;

        double value = 0.0;
        if (!read_number_arg(args[0], &value, "Vehicle.setDifferentialLimitedSlipRatio()", 1))
            return 0;

        controller->SetDifferentialLimitedSlipRatio((float)value);
        return 0;
    }

    static int wheeled_get_differential_limited_slip_ratio(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("Vehicle.getDifferentialLimitedSlipRatio() expects 0 arguments");
            return push_nil1(vm);
        }

        WheeledVehicleController *controller = require_wheeled_controller(instance, "Vehicle.getDifferentialLimitedSlipRatio()");
        if (!controller)
            return push_nil1(vm);

        vm->pushDouble((double)controller->GetDifferentialLimitedSlipRatio());
        return 1;
    }

    static int tracked_set_driver_input(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 4)
        {
            Error("TrackedVehicle.setDriverInput() expects (forward, leftRatio, rightRatio, brake)");
            return 0;
        }

        TrackedVehicleController *controller = require_tracked_controller(instance, "TrackedVehicle.setDriverInput()");
        if (!controller)
            return 0;

        double forward = 0.0;
        double left_ratio = 1.0;
        double right_ratio = 1.0;
        double brake = 0.0;
        if (!read_number_arg(args[0], &forward, "TrackedVehicle.setDriverInput()", 1) ||
            !read_number_arg(args[1], &left_ratio, "TrackedVehicle.setDriverInput()", 2) ||
            !read_number_arg(args[2], &right_ratio, "TrackedVehicle.setDriverInput()", 3) ||
            !read_number_arg(args[3], &brake, "TrackedVehicle.setDriverInput()", 4))
            return 0;

        controller->SetDriverInput((float)forward, (float)left_ratio, (float)right_ratio, (float)brake);
        return 0;
    }

    static int tracked_set_forward_input(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("TrackedVehicle.setForwardInput() expects (number)");
            return 0;
        }

        TrackedVehicleController *controller = require_tracked_controller(instance, "TrackedVehicle.setForwardInput()");
        if (!controller)
            return 0;

        double value = 0.0;
        if (!read_number_arg(args[0], &value, "TrackedVehicle.setForwardInput()", 1))
            return 0;

        controller->SetForwardInput((float)value);
        return 0;
    }

    static int tracked_set_left_ratio(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("TrackedVehicle.setLeftRatio() expects (number)");
            return 0;
        }

        TrackedVehicleController *controller = require_tracked_controller(instance, "TrackedVehicle.setLeftRatio()");
        if (!controller)
            return 0;

        double value = 1.0;
        if (!read_number_arg(args[0], &value, "TrackedVehicle.setLeftRatio()", 1))
            return 0;

        controller->SetLeftRatio((float)value);
        return 0;
    }

    static int tracked_set_right_ratio(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("TrackedVehicle.setRightRatio() expects (number)");
            return 0;
        }

        TrackedVehicleController *controller = require_tracked_controller(instance, "TrackedVehicle.setRightRatio()");
        if (!controller)
            return 0;

        double value = 1.0;
        if (!read_number_arg(args[0], &value, "TrackedVehicle.setRightRatio()", 1))
            return 0;

        controller->SetRightRatio((float)value);
        return 0;
    }

    static int tracked_set_brake_input(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("TrackedVehicle.setBrakeInput() expects (number)");
            return 0;
        }

        TrackedVehicleController *controller = require_tracked_controller(instance, "TrackedVehicle.setBrakeInput()");
        if (!controller)
            return 0;

        double value = 0.0;
        if (!read_number_arg(args[0], &value, "TrackedVehicle.setBrakeInput()", 1))
            return 0;

        controller->SetBrakeInput((float)value);
        return 0;
    }

    static int tracked_get_engine_rpm(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("TrackedVehicle.getEngineRPM() expects 0 arguments");
            return push_nil1(vm);
        }

        TrackedVehicleController *controller = require_tracked_controller(instance, "TrackedVehicle.getEngineRPM()");
        if (!controller)
            return push_nil1(vm);

        vm->pushDouble((double)controller->GetEngine().GetCurrentRPM());
        return 1;
    }

    static int tracked_get_current_gear(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("TrackedVehicle.getCurrentGear() expects 0 arguments");
            return push_nil1(vm);
        }

        TrackedVehicleController *controller = require_tracked_controller(instance, "TrackedVehicle.getCurrentGear()");
        if (!controller)
            return push_nil1(vm);

        vm->pushInt(controller->GetTransmission().GetCurrentGear());
        return 1;
    }

    static int tracked_set_engine_max_torque(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("TrackedVehicle.setEngineMaxTorque() expects (number)");
            return 0;
        }

        TrackedVehicleController *controller = require_tracked_controller(instance, "TrackedVehicle.setEngineMaxTorque()");
        if (!controller)
            return 0;

        double value = 0.0;
        if (!read_number_arg(args[0], &value, "TrackedVehicle.setEngineMaxTorque()", 1))
            return 0;

        controller->GetEngine().mMaxTorque = (float)value;
        return 0;
    }

    static int tracked_set_engine_rpm_range(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("TrackedVehicle.setEngineRPMRange() expects (minRPM, maxRPM)");
            return 0;
        }

        TrackedVehicleController *controller = require_tracked_controller(instance, "TrackedVehicle.setEngineRPMRange()");
        if (!controller)
            return 0;

        double min_rpm = 0.0;
        double max_rpm = 0.0;
        if (!read_number_arg(args[0], &min_rpm, "TrackedVehicle.setEngineRPMRange()", 1) ||
            !read_number_arg(args[1], &max_rpm, "TrackedVehicle.setEngineRPMRange()", 2))
            return 0;

        controller->GetEngine().mMinRPM = (float)min_rpm;
        controller->GetEngine().mMaxRPM = (float)max_rpm;
        return 0;
    }

    static int tracked_set_shift_rpm(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("TrackedVehicle.setShiftRPM() expects (downRPM, upRPM)");
            return 0;
        }

        TrackedVehicleController *controller = require_tracked_controller(instance, "TrackedVehicle.setShiftRPM()");
        if (!controller)
            return 0;

        double down_rpm = 0.0;
        double up_rpm = 0.0;
        if (!read_number_arg(args[0], &down_rpm, "TrackedVehicle.setShiftRPM()", 1) ||
            !read_number_arg(args[1], &up_rpm, "TrackedVehicle.setShiftRPM()", 2))
            return 0;

        controller->GetTransmission().mShiftDownRPM = (float)down_rpm;
        controller->GetTransmission().mShiftUpRPM = (float)up_rpm;
        return 0;
    }

    static int motorcycle_enable_lean_controller(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isBool())
        {
            Error("Motorcycle.enableLeanController() expects (bool)");
            return 0;
        }

        MotorcycleController *controller = require_motorcycle_controller(instance, "Motorcycle.enableLeanController()");
        if (!controller)
            return 0;

        controller->EnableLeanController(args[0].asBool());
        return 0;
    }

    static int motorcycle_is_lean_controller_enabled(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("Motorcycle.isLeanControllerEnabled() expects 0 arguments");
            return push_nil1(vm);
        }

        MotorcycleController *controller = require_motorcycle_controller(instance, "Motorcycle.isLeanControllerEnabled()");
        if (!controller)
            return push_nil1(vm);

        vm->pushBool(controller->IsLeanControllerEnabled());
        return 1;
    }

    static int motorcycle_enable_lean_steering_limit(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isBool())
        {
            Error("Motorcycle.enableLeanSteeringLimit() expects (bool)");
            return 0;
        }

        MotorcycleController *controller = require_motorcycle_controller(instance, "Motorcycle.enableLeanSteeringLimit()");
        if (!controller)
            return 0;

        controller->EnableLeanSteeringLimit(args[0].asBool());
        return 0;
    }

    static int motorcycle_is_lean_steering_limit_enabled(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("Motorcycle.isLeanSteeringLimitEnabled() expects 0 arguments");
            return push_nil1(vm);
        }

        MotorcycleController *controller = require_motorcycle_controller(instance, "Motorcycle.isLeanSteeringLimitEnabled()");
        if (!controller)
            return push_nil1(vm);

        vm->pushBool(controller->IsLeanSteeringLimitEnabled());
        return 1;
    }

    static int motorcycle_set_lean_spring_constant(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("Motorcycle.setLeanSpringConstant() expects (number)");
            return 0;
        }

        MotorcycleController *controller = require_motorcycle_controller(instance, "Motorcycle.setLeanSpringConstant()");
        if (!controller)
            return 0;

        double value = 0.0;
        if (!read_number_arg(args[0], &value, "Motorcycle.setLeanSpringConstant()", 1))
            return 0;

        controller->SetLeanSpringConstant((float)value);
        return 0;
    }

    static int motorcycle_get_lean_spring_constant(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("Motorcycle.getLeanSpringConstant() expects 0 arguments");
            return push_nil1(vm);
        }

        MotorcycleController *controller = require_motorcycle_controller(instance, "Motorcycle.getLeanSpringConstant()");
        if (!controller)
            return push_nil1(vm);

        vm->pushDouble((double)controller->GetLeanSpringConstant());
        return 1;
    }

    static void *wheel_settings_tv_ctor(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("JoltWheelSettingsTV() expects 0 arguments");
            return nullptr;
        }

        JoltWheelSettingsTVHandle *handle = new JoltWheelSettingsTVHandle();
        handle->settings = new WheelSettingsTV();
        return handle;
    }

    static void wheel_settings_tv_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        delete (JoltWheelSettingsTVHandle *)instance;
    }

    static void *wheel_settings_wv_ctor(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("JoltWheelSettingsWV() expects 0 arguments");
            return nullptr;
        }

        JoltWheelSettingsWVHandle *handle = new JoltWheelSettingsWVHandle();
        handle->settings = new WheelSettingsWV();
        return handle;
    }

    static void wheel_settings_wv_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        delete (JoltWheelSettingsWVHandle *)instance;
    }

    static bool set_tracked_wheel_vec3_arg(void *instance, const Value &arg, Vec3 WheelSettings::*member, const char *fn)
    {
        JoltWheelSettingsTVHandle *wheel = require_tracked_wheel_settings(instance, fn);
        if (!wheel)
            return false;

        Vector3 value;
        if (!read_vector3_arg(arg, &value, fn, 1))
            return false;

        wheel->settings.GetPtr()->*member = to_jolt_vec3(value);
        return true;
    }

    static bool set_tracked_wheel_float_arg(void *instance, const Value &arg, float WheelSettings::*member, const char *fn)
    {
        JoltWheelSettingsTVHandle *wheel = require_tracked_wheel_settings(instance, fn);
        if (!wheel)
            return false;

        double value = 0.0;
        if (!read_number_arg(arg, &value, fn, 1))
            return false;

        wheel->settings.GetPtr()->*member = (float)value;
        return true;
    }

    static bool set_tracked_wheel_tv_float_arg(void *instance, const Value &arg, float WheelSettingsTV::*member, const char *fn)
    {
        JoltWheelSettingsTVHandle *wheel = require_tracked_wheel_settings(instance, fn);
        if (!wheel)
            return false;

        double value = 0.0;
        if (!read_number_arg(arg, &value, fn, 1))
            return false;

        wheel->settings.GetPtr()->*member = (float)value;
        return true;
    }

    static bool set_wheel_vec3_arg(void *instance, const Value &arg, Vec3 WheelSettingsWV::*member, const char *fn)
    {
        JoltWheelSettingsWVHandle *wheel = require_wheel_settings(instance, fn);
        if (!wheel)
            return false;

        Vector3 value;
        if (!read_vector3_arg(arg, &value, fn, 1))
            return false;

        wheel->settings->*member = to_jolt_vec3(value);
        return true;
    }

    static bool set_wheel_float_arg(void *instance, const Value &arg, float WheelSettingsWV::*member, const char *fn)
    {
        JoltWheelSettingsWVHandle *wheel = require_wheel_settings(instance, fn);
        if (!wheel)
            return false;

        double value = 0.0;
        if (!read_number_arg(arg, &value, fn, 1))
            return false;

        wheel->settings->*member = (float)value;
        return true;
    }

    static int wheel_settings_set_position(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsWV.setPosition() expects (Vector3)");
            return 0;
        }
        return set_wheel_vec3_arg(instance, args[0], &WheelSettingsWV::mPosition, "JoltWheelSettingsWV.setPosition()") ? 0 : 0;
    }

    static int wheel_settings_set_suspension_force_point(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsWV.setSuspensionForcePoint() expects (Vector3)");
            return 0;
        }
        return set_wheel_vec3_arg(instance, args[0], &WheelSettingsWV::mSuspensionForcePoint, "JoltWheelSettingsWV.setSuspensionForcePoint()") ? 0 : 0;
    }

    static int wheel_settings_set_suspension_direction(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsWV.setSuspensionDirection() expects (Vector3)");
            return 0;
        }
        return set_wheel_vec3_arg(instance, args[0], &WheelSettingsWV::mSuspensionDirection, "JoltWheelSettingsWV.setSuspensionDirection()") ? 0 : 0;
    }

    static int wheel_settings_set_steering_axis(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsWV.setSteeringAxis() expects (Vector3)");
            return 0;
        }
        return set_wheel_vec3_arg(instance, args[0], &WheelSettingsWV::mSteeringAxis, "JoltWheelSettingsWV.setSteeringAxis()") ? 0 : 0;
    }

    static int wheel_settings_set_wheel_up(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsWV.setWheelUp() expects (Vector3)");
            return 0;
        }
        return set_wheel_vec3_arg(instance, args[0], &WheelSettingsWV::mWheelUp, "JoltWheelSettingsWV.setWheelUp()") ? 0 : 0;
    }

    static int wheel_settings_set_wheel_forward(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsWV.setWheelForward() expects (Vector3)");
            return 0;
        }
        return set_wheel_vec3_arg(instance, args[0], &WheelSettingsWV::mWheelForward, "JoltWheelSettingsWV.setWheelForward()") ? 0 : 0;
    }

    static int wheel_settings_set_suspension_min_length(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsWV.setSuspensionMinLength() expects (number)");
            return 0;
        }
        return set_wheel_float_arg(instance, args[0], &WheelSettingsWV::mSuspensionMinLength, "JoltWheelSettingsWV.setSuspensionMinLength()") ? 0 : 0;
    }

    static int wheel_settings_set_suspension_max_length(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsWV.setSuspensionMaxLength() expects (number)");
            return 0;
        }
        return set_wheel_float_arg(instance, args[0], &WheelSettingsWV::mSuspensionMaxLength, "JoltWheelSettingsWV.setSuspensionMaxLength()") ? 0 : 0;
    }

    static int wheel_settings_set_suspension_preload_length(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsWV.setSuspensionPreloadLength() expects (number)");
            return 0;
        }
        return set_wheel_float_arg(instance, args[0], &WheelSettingsWV::mSuspensionPreloadLength, "JoltWheelSettingsWV.setSuspensionPreloadLength()") ? 0 : 0;
    }

    static int wheel_settings_set_suspension_frequency(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsWV.setSuspensionFrequency() expects (number)");
            return 0;
        }

        JoltWheelSettingsWVHandle *wheel = require_wheel_settings(instance, "JoltWheelSettingsWV.setSuspensionFrequency()");
        if (!wheel)
            return 0;

        double value = 0.0;
        if (!read_number_arg(args[0], &value, "JoltWheelSettingsWV.setSuspensionFrequency()", 1))
            return 0;

        wheel->settings->mSuspensionSpring.mFrequency = (float)value;
        return 0;
    }

    static int wheel_settings_set_suspension_damping(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsWV.setSuspensionDamping() expects (number)");
            return 0;
        }

        JoltWheelSettingsWVHandle *wheel = require_wheel_settings(instance, "JoltWheelSettingsWV.setSuspensionDamping()");
        if (!wheel)
            return 0;

        double value = 0.0;
        if (!read_number_arg(args[0], &value, "JoltWheelSettingsWV.setSuspensionDamping()", 1))
            return 0;

        wheel->settings->mSuspensionSpring.mDamping = (float)value;
        return 0;
    }

    static int wheel_settings_set_radius(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsWV.setRadius() expects (number)");
            return 0;
        }
        return set_wheel_float_arg(instance, args[0], &WheelSettingsWV::mRadius, "JoltWheelSettingsWV.setRadius()") ? 0 : 0;
    }

    static int wheel_settings_get_radius(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltWheelSettingsWV.getRadius() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltWheelSettingsWVHandle *wheel = require_wheel_settings(instance, "JoltWheelSettingsWV.getRadius()");
        if (!wheel)
            return push_nil1(vm);

        vm->pushDouble((double)wheel->settings->mRadius);
        return 1;
    }

    static int wheel_settings_set_width(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsWV.setWidth() expects (number)");
            return 0;
        }
        return set_wheel_float_arg(instance, args[0], &WheelSettingsWV::mWidth, "JoltWheelSettingsWV.setWidth()") ? 0 : 0;
    }

    static int wheel_settings_set_enable_suspension_force_point(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isBool())
        {
            Error("JoltWheelSettingsWV.setEnableSuspensionForcePoint() expects (bool)");
            return 0;
        }

        JoltWheelSettingsWVHandle *wheel = require_wheel_settings(instance, "JoltWheelSettingsWV.setEnableSuspensionForcePoint()");
        if (!wheel)
            return 0;

        wheel->settings->mEnableSuspensionForcePoint = args[0].asBool();
        return 0;
    }

    static int wheel_settings_set_inertia(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsWV.setInertia() expects (number)");
            return 0;
        }
        return set_wheel_float_arg(instance, args[0], &WheelSettingsWV::mInertia, "JoltWheelSettingsWV.setInertia()") ? 0 : 0;
    }

    static int wheel_settings_set_angular_damping(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsWV.setAngularDamping() expects (number)");
            return 0;
        }
        return set_wheel_float_arg(instance, args[0], &WheelSettingsWV::mAngularDamping, "JoltWheelSettingsWV.setAngularDamping()") ? 0 : 0;
    }

    static int wheel_settings_set_max_steer_angle(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsWV.setMaxSteerAngle() expects (number)");
            return 0;
        }
        return set_wheel_float_arg(instance, args[0], &WheelSettingsWV::mMaxSteerAngle, "JoltWheelSettingsWV.setMaxSteerAngle()") ? 0 : 0;
    }

    static int wheel_settings_set_max_brake_torque(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsWV.setMaxBrakeTorque() expects (number)");
            return 0;
        }
        return set_wheel_float_arg(instance, args[0], &WheelSettingsWV::mMaxBrakeTorque, "JoltWheelSettingsWV.setMaxBrakeTorque()") ? 0 : 0;
    }

    static int wheel_settings_set_max_hand_brake_torque(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsWV.setMaxHandBrakeTorque() expects (number)");
            return 0;
        }
        return set_wheel_float_arg(instance, args[0], &WheelSettingsWV::mMaxHandBrakeTorque, "JoltWheelSettingsWV.setMaxHandBrakeTorque()") ? 0 : 0;
    }

    static int tracked_wheel_settings_set_position(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsTV.setPosition() expects (Vector3)");
            return 0;
        }
        return set_tracked_wheel_vec3_arg(instance, args[0], &WheelSettings::mPosition, "JoltWheelSettingsTV.setPosition()") ? 0 : 0;
    }

    static int tracked_wheel_settings_set_suspension_direction(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsTV.setSuspensionDirection() expects (Vector3)");
            return 0;
        }
        return set_tracked_wheel_vec3_arg(instance, args[0], &WheelSettings::mSuspensionDirection, "JoltWheelSettingsTV.setSuspensionDirection()") ? 0 : 0;
    }

    static int tracked_wheel_settings_set_suspension_min_length(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsTV.setSuspensionMinLength() expects (number)");
            return 0;
        }
        return set_tracked_wheel_float_arg(instance, args[0], &WheelSettings::mSuspensionMinLength, "JoltWheelSettingsTV.setSuspensionMinLength()") ? 0 : 0;
    }

    static int tracked_wheel_settings_set_suspension_max_length(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsTV.setSuspensionMaxLength() expects (number)");
            return 0;
        }
        return set_tracked_wheel_float_arg(instance, args[0], &WheelSettings::mSuspensionMaxLength, "JoltWheelSettingsTV.setSuspensionMaxLength()") ? 0 : 0;
    }

    static int tracked_wheel_settings_set_suspension_preload_length(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsTV.setSuspensionPreloadLength() expects (number)");
            return 0;
        }
        return set_tracked_wheel_float_arg(instance, args[0], &WheelSettings::mSuspensionPreloadLength, "JoltWheelSettingsTV.setSuspensionPreloadLength()") ? 0 : 0;
    }

    static int tracked_wheel_settings_set_suspension_frequency(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsTV.setSuspensionFrequency() expects (number)");
            return 0;
        }

        JoltWheelSettingsTVHandle *wheel = require_tracked_wheel_settings(instance, "JoltWheelSettingsTV.setSuspensionFrequency()");
        if (!wheel)
            return 0;

        double value = 0.0;
        if (!read_number_arg(args[0], &value, "JoltWheelSettingsTV.setSuspensionFrequency()", 1))
            return 0;

        wheel->settings->mSuspensionSpring.mFrequency = (float)value;
        return 0;
    }

    static int tracked_wheel_settings_set_suspension_damping(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsTV.setSuspensionDamping() expects (number)");
            return 0;
        }

        JoltWheelSettingsTVHandle *wheel = require_tracked_wheel_settings(instance, "JoltWheelSettingsTV.setSuspensionDamping()");
        if (!wheel)
            return 0;

        double value = 0.0;
        if (!read_number_arg(args[0], &value, "JoltWheelSettingsTV.setSuspensionDamping()", 1))
            return 0;

        wheel->settings->mSuspensionSpring.mDamping = (float)value;
        return 0;
    }

    static int tracked_wheel_settings_set_radius(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsTV.setRadius() expects (number)");
            return 0;
        }
        return set_tracked_wheel_float_arg(instance, args[0], &WheelSettings::mRadius, "JoltWheelSettingsTV.setRadius()") ? 0 : 0;
    }

    static int tracked_wheel_settings_get_radius(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltWheelSettingsTV.getRadius() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltWheelSettingsTVHandle *wheel = require_tracked_wheel_settings(instance, "JoltWheelSettingsTV.getRadius()");
        if (!wheel)
            return push_nil1(vm);

        vm->pushDouble((double)wheel->settings->mRadius);
        return 1;
    }

    static int tracked_wheel_settings_set_width(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsTV.setWidth() expects (number)");
            return 0;
        }
        return set_tracked_wheel_float_arg(instance, args[0], &WheelSettings::mWidth, "JoltWheelSettingsTV.setWidth()") ? 0 : 0;
    }

    static int tracked_wheel_settings_set_longitudinal_friction(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsTV.setLongitudinalFriction() expects (number)");
            return 0;
        }
        return set_tracked_wheel_tv_float_arg(instance, args[0], &WheelSettingsTV::mLongitudinalFriction, "JoltWheelSettingsTV.setLongitudinalFriction()") ? 0 : 0;
    }

    static int tracked_wheel_settings_set_lateral_friction(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1)
        {
            Error("JoltWheelSettingsTV.setLateralFriction() expects (number)");
            return 0;
        }
        return set_tracked_wheel_tv_float_arg(instance, args[0], &WheelSettingsTV::mLateralFriction, "JoltWheelSettingsTV.setLateralFriction()") ? 0 : 0;
    }

    static void *vehicle_ctor_error(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)argCount;
        (void)args;
        Error("Jolt vehicles cannot be constructed directly; use JoltWorld.createWheeledVehicle(), JoltWorld.createMotorcycle() or JoltWorld.createTrackedVehicle()");
        return nullptr;
    }

    static void vehicle_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        JoltConstraintHandle *vehicle = (JoltConstraintHandle *)instance;
        if (!vehicle)
            return;

        vehicle->wrapperAlive = false;
        if (vehicle->world == nullptr || !vehicle->valid)
            delete vehicle;
    }

    static int world_create_vehicle_common(Interpreter *vm, void *instance, int argCount, Value *args, bool motorcycle)
    {
        if (argCount != 2 && argCount != 4)
        {
            Error(motorcycle
                      ? "JoltWorld.createMotorcycle() expects (body, wheels[, up, forward])"
                      : "JoltWorld.createWheeledVehicle() expects (body, wheels[, up, forward])");
            return push_nil1(vm);
        }

        const char *fn = motorcycle ? "JoltWorld.createMotorcycle()" : "JoltWorld.createWheeledVehicle()";
        JoltWorldHandle *world = require_world(instance, fn);
        if (!world)
            return push_nil1(vm);

        JoltBodyHandle *body = require_body_value_arg(args[0], fn, 1);
        if (!body)
            return push_nil1(vm);

        if (body->world != world)
        {
            Error("%s body belongs to another world", fn);
            return push_nil1(vm);
        }

        JPH::Array<Ref<WheelSettings>> wheels;
        if (!read_wheel_settings_array(args[1], &wheels, fn, 2))
            return push_nil1(vm);

        VehicleConstraintSettings settings;
        settings.mController = motorcycle ? (VehicleControllerSettings *)new MotorcycleControllerSettings()
                                          : (VehicleControllerSettings *)new WheeledVehicleControllerSettings();
        settings.mWheels = wheels;
        configure_default_powertrain(settings.mController, motorcycle);
        configure_default_differentials(settings.mController, settings.mWheels, motorcycle);

        if (argCount == 4)
        {
            Vector3 up;
            Vector3 forward;
            if (!read_vector3_arg(args[2], &up, fn, 3) ||
                !read_vector3_arg(args[3], &forward, fn, 4))
                return push_nil1(vm);
            settings.mUp = to_jolt_vec3(up);
            settings.mForward = to_jolt_vec3(forward);
        }

        BodyLockWrite lock(world->physicsSystem.GetBodyLockInterface(), body->id);
        if (!lock.Succeeded())
        {
            Error("%s could not lock chassis body", fn);
            return push_nil1(vm);
        }

        VehicleConstraint *vehicle = new VehicleConstraint(lock.GetBody(), settings);
        if (motorcycle)
            vehicle->SetVehicleCollisionTester(new VehicleCollisionTesterCastCylinder(Layers::MOVING, 1.0f));
        else
            vehicle->SetVehicleCollisionTester(new VehicleCollisionTesterRay(Layers::MOVING));

        JoltConstraintHandle *handle = create_constraint_handle(world, vehicle, EConstraintSubType::Vehicle);
        if (!handle)
        {
            delete vehicle;
            return push_nil1(vm);
        }

        NativeClassDef *klass = motorcycle ? g_motorcycleClass : g_wheeledVehicleClass;
        if (push_vehicle_handle(vm, klass, handle))
            return 1;

        destroy_constraint_runtime(handle);
        delete handle;
        return push_nil1(vm);
    }

    static int world_create_wheeled_vehicle(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        return world_create_vehicle_common(vm, instance, argCount, args, false);
    }

    static int world_create_motorcycle(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        return world_create_vehicle_common(vm, instance, argCount, args, true);
    }

    static int world_create_tracked_vehicle(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2 && argCount != 4)
        {
            Error("JoltWorld.createTrackedVehicle() expects (body, wheels[, up, forward])");
            return push_nil1(vm);
        }

        const char *fn = "JoltWorld.createTrackedVehicle()";
        JoltWorldHandle *world = require_world(instance, fn);
        if (!world)
            return push_nil1(vm);

        JoltBodyHandle *body = require_body_value_arg(args[0], fn, 1);
        if (!body)
            return push_nil1(vm);

        if (body->world != world)
        {
            Error("%s body belongs to another world", fn);
            return push_nil1(vm);
        }

        JPH::Array<Ref<WheelSettings>> wheels;
        if (!read_tracked_wheel_settings_array(args[1], &wheels, fn, 2))
            return push_nil1(vm);

        VehicleConstraintSettings settings;
        auto *controller = new TrackedVehicleControllerSettings();
        if (!configure_default_tracks(controller, wheels, fn))
        {
            delete controller;
            return push_nil1(vm);
        }

        settings.mController = controller;
        settings.mWheels = wheels;

        if (argCount == 4)
        {
            Vector3 up;
            Vector3 forward;
            if (!read_vector3_arg(args[2], &up, fn, 3) ||
                !read_vector3_arg(args[3], &forward, fn, 4))
            {
                delete controller;
                return push_nil1(vm);
            }
            settings.mUp = to_jolt_vec3(up);
            settings.mForward = to_jolt_vec3(forward);
        }

        BodyLockWrite lock(world->physicsSystem.GetBodyLockInterface(), body->id);
        if (!lock.Succeeded())
        {
            Error("%s could not lock chassis body", fn);
            delete controller;
            return push_nil1(vm);
        }

        VehicleConstraint *vehicle = new VehicleConstraint(lock.GetBody(), settings);
        vehicle->SetVehicleCollisionTester(new VehicleCollisionTesterRay(Layers::MOVING));

        JoltConstraintHandle *handle = create_constraint_handle(world, vehicle, EConstraintSubType::Vehicle);
        if (!handle)
        {
            delete vehicle;
            return push_nil1(vm);
        }

        if (push_vehicle_handle(vm, g_trackedVehicleClass, handle))
            return 1;

        destroy_constraint_runtime(handle);
        delete handle;
        return push_nil1(vm);
    }

    static void add_base_vehicle_methods(Interpreter &vm, NativeClassDef *klass)
    {
        vm.addNativeMethod(klass, "destroy", vehicle_destroy);
        vm.addNativeMethod(klass, "isValid", vehicle_is_valid);
        vm.addNativeMethod(klass, "getWheelCount", vehicle_get_wheel_count);
        vm.addNativeMethod(klass, "getWheelWorldPosition", vehicle_get_wheel_world_position);
        vm.addNativeMethod(klass, "getWheelWorldPose", vehicle_get_wheel_world_pose);
        vm.addNativeMethod(klass, "hasWheelContact", vehicle_has_wheel_contact);
        vm.addNativeMethod(klass, "getWheelContactPosition", vehicle_get_wheel_contact_position);
        vm.addNativeMethod(klass, "getWheelContactNormal", vehicle_get_wheel_contact_normal);
        vm.addNativeMethod(klass, "getWheelSuspensionLength", vehicle_get_wheel_suspension_length);
        vm.addNativeMethod(klass, "getWheelRotationAngle", vehicle_get_wheel_rotation_angle);
        vm.addNativeMethod(klass, "getWheelSteerAngle", vehicle_get_wheel_steer_angle);
        vm.addNativeMethod(klass, "setMaxPitchRollAngle", vehicle_set_max_pitch_roll_angle);
        vm.addNativeMethod(klass, "getMaxPitchRollAngle", vehicle_get_max_pitch_roll_angle);
        vm.addNativeMethod(klass, "overrideGravity", vehicle_override_gravity);
        vm.addNativeMethod(klass, "resetGravityOverride", vehicle_reset_gravity_override);
        vm.addNativeMethod(klass, "setNumStepsBetweenCollisionTestActive", vehicle_set_num_steps_active);
        vm.addNativeMethod(klass, "setNumStepsBetweenCollisionTestInactive", vehicle_set_num_steps_inactive);
        vm.addNativeMethod(klass, "setCollisionTesterRay", vehicle_set_collision_tester_ray);
        vm.addNativeMethod(klass, "setCollisionTesterSphere", vehicle_set_collision_tester_sphere);
        vm.addNativeMethod(klass, "setCollisionTesterCylinder", vehicle_set_collision_tester_cylinder);
    }

    static void add_wheeled_vehicle_methods(Interpreter &vm, NativeClassDef *klass)
    {
        add_base_vehicle_methods(vm, klass);
        vm.addNativeMethod(klass, "setDriverInput", wheeled_set_driver_input);
        vm.addNativeMethod(klass, "setForwardInput", wheeled_set_forward_input);
        vm.addNativeMethod(klass, "setRightInput", wheeled_set_right_input);
        vm.addNativeMethod(klass, "setBrakeInput", wheeled_set_brake_input);
        vm.addNativeMethod(klass, "setHandBrakeInput", wheeled_set_hand_brake_input);
        vm.addNativeMethod(klass, "getEngineRPM", wheeled_get_engine_rpm);
        vm.addNativeMethod(klass, "getCurrentGear", wheeled_get_current_gear);
        vm.addNativeMethod(klass, "getClutchFriction", wheeled_get_clutch_friction);
        vm.addNativeMethod(klass, "setEngineMaxTorque", wheeled_set_engine_max_torque);
        vm.addNativeMethod(klass, "setEngineRPMRange", wheeled_set_engine_rpm_range);
        vm.addNativeMethod(klass, "setShiftRPM", wheeled_set_shift_rpm);
        vm.addNativeMethod(klass, "setClutchStrength", wheeled_set_clutch_strength);
        vm.addNativeMethod(klass, "setGearRatios", wheeled_set_gear_ratios);
        vm.addNativeMethod(klass, "setReverseGearRatios", wheeled_set_reverse_gear_ratios);
        vm.addNativeMethod(klass, "clearDifferentials", wheeled_clear_differentials);
        vm.addNativeMethod(klass, "addDifferential", wheeled_add_differential);
        vm.addNativeMethod(klass, "setTransmissionMode", wheeled_set_transmission_mode);
        vm.addNativeMethod(klass, "getTransmissionMode", wheeled_get_transmission_mode);
        vm.addNativeMethod(klass, "setTransmission", wheeled_set_transmission);
        vm.addNativeMethod(klass, "setDifferentialLimitedSlipRatio", wheeled_set_differential_limited_slip_ratio);
        vm.addNativeMethod(klass, "getDifferentialLimitedSlipRatio", wheeled_get_differential_limited_slip_ratio);
    }

    static void add_tracked_vehicle_methods(Interpreter &vm, NativeClassDef *klass)
    {
        add_base_vehicle_methods(vm, klass);
        vm.addNativeMethod(klass, "setDriverInput", tracked_set_driver_input);
        vm.addNativeMethod(klass, "setForwardInput", tracked_set_forward_input);
        vm.addNativeMethod(klass, "setLeftRatio", tracked_set_left_ratio);
        vm.addNativeMethod(klass, "setRightRatio", tracked_set_right_ratio);
        vm.addNativeMethod(klass, "setBrakeInput", tracked_set_brake_input);
        vm.addNativeMethod(klass, "getEngineRPM", tracked_get_engine_rpm);
        vm.addNativeMethod(klass, "getCurrentGear", tracked_get_current_gear);
        vm.addNativeMethod(klass, "setEngineMaxTorque", tracked_set_engine_max_torque);
        vm.addNativeMethod(klass, "setEngineRPMRange", tracked_set_engine_rpm_range);
        vm.addNativeMethod(klass, "setShiftRPM", tracked_set_shift_rpm);
    }

    void register_jolt_vehicle(Interpreter &vm)
    {
        g_wheelSettingsWVClass = vm.registerNativeClass("JoltWheelSettingsWV", wheel_settings_wv_ctor, wheel_settings_wv_dtor, 0, false);
        g_wheelSettingsTVClass = vm.registerNativeClass("JoltWheelSettingsTV", wheel_settings_tv_ctor, wheel_settings_tv_dtor, 0, false);
        g_wheeledVehicleClass = vm.registerNativeClass("JoltWheeledVehicle", vehicle_ctor_error, vehicle_dtor, 0, false);
        g_motorcycleClass = vm.registerNativeClass("JoltMotorcycle", vehicle_ctor_error, vehicle_dtor, 0, false);
        g_trackedVehicleClass = vm.registerNativeClass("JoltTrackedVehicle", vehicle_ctor_error, vehicle_dtor, 0, false);

        vm.addNativeMethod(g_wheelSettingsWVClass, "setPosition", wheel_settings_set_position);
        vm.addNativeMethod(g_wheelSettingsWVClass, "setSuspensionForcePoint", wheel_settings_set_suspension_force_point);
        vm.addNativeMethod(g_wheelSettingsWVClass, "setSuspensionDirection", wheel_settings_set_suspension_direction);
        vm.addNativeMethod(g_wheelSettingsWVClass, "setSteeringAxis", wheel_settings_set_steering_axis);
        vm.addNativeMethod(g_wheelSettingsWVClass, "setWheelUp", wheel_settings_set_wheel_up);
        vm.addNativeMethod(g_wheelSettingsWVClass, "setWheelForward", wheel_settings_set_wheel_forward);
        vm.addNativeMethod(g_wheelSettingsWVClass, "setSuspensionMinLength", wheel_settings_set_suspension_min_length);
        vm.addNativeMethod(g_wheelSettingsWVClass, "setSuspensionMaxLength", wheel_settings_set_suspension_max_length);
        vm.addNativeMethod(g_wheelSettingsWVClass, "setSuspensionPreloadLength", wheel_settings_set_suspension_preload_length);
        vm.addNativeMethod(g_wheelSettingsWVClass, "setSuspensionFrequency", wheel_settings_set_suspension_frequency);
        vm.addNativeMethod(g_wheelSettingsWVClass, "setSuspensionDamping", wheel_settings_set_suspension_damping);
        vm.addNativeMethod(g_wheelSettingsWVClass, "setRadius", wheel_settings_set_radius);
        vm.addNativeMethod(g_wheelSettingsWVClass, "getRadius", wheel_settings_get_radius);
        vm.addNativeMethod(g_wheelSettingsWVClass, "setWidth", wheel_settings_set_width);
        vm.addNativeMethod(g_wheelSettingsWVClass, "setEnableSuspensionForcePoint", wheel_settings_set_enable_suspension_force_point);
        vm.addNativeMethod(g_wheelSettingsWVClass, "setInertia", wheel_settings_set_inertia);
        vm.addNativeMethod(g_wheelSettingsWVClass, "setAngularDamping", wheel_settings_set_angular_damping);
        vm.addNativeMethod(g_wheelSettingsWVClass, "setMaxSteerAngle", wheel_settings_set_max_steer_angle);
        vm.addNativeMethod(g_wheelSettingsWVClass, "setMaxBrakeTorque", wheel_settings_set_max_brake_torque);
        vm.addNativeMethod(g_wheelSettingsWVClass, "setMaxHandBrakeTorque", wheel_settings_set_max_hand_brake_torque);

        vm.addNativeMethod(g_wheelSettingsTVClass, "setPosition", tracked_wheel_settings_set_position);
        vm.addNativeMethod(g_wheelSettingsTVClass, "setSuspensionDirection", tracked_wheel_settings_set_suspension_direction);
        vm.addNativeMethod(g_wheelSettingsTVClass, "setSuspensionMinLength", tracked_wheel_settings_set_suspension_min_length);
        vm.addNativeMethod(g_wheelSettingsTVClass, "setSuspensionMaxLength", tracked_wheel_settings_set_suspension_max_length);
        vm.addNativeMethod(g_wheelSettingsTVClass, "setSuspensionPreloadLength", tracked_wheel_settings_set_suspension_preload_length);
        vm.addNativeMethod(g_wheelSettingsTVClass, "setSuspensionFrequency", tracked_wheel_settings_set_suspension_frequency);
        vm.addNativeMethod(g_wheelSettingsTVClass, "setSuspensionDamping", tracked_wheel_settings_set_suspension_damping);
        vm.addNativeMethod(g_wheelSettingsTVClass, "setRadius", tracked_wheel_settings_set_radius);
        vm.addNativeMethod(g_wheelSettingsTVClass, "getRadius", tracked_wheel_settings_get_radius);
        vm.addNativeMethod(g_wheelSettingsTVClass, "setWidth", tracked_wheel_settings_set_width);
        vm.addNativeMethod(g_wheelSettingsTVClass, "setLongitudinalFriction", tracked_wheel_settings_set_longitudinal_friction);
        vm.addNativeMethod(g_wheelSettingsTVClass, "setLateralFriction", tracked_wheel_settings_set_lateral_friction);

        add_wheeled_vehicle_methods(vm, g_wheeledVehicleClass);
        add_wheeled_vehicle_methods(vm, g_motorcycleClass);
        add_tracked_vehicle_methods(vm, g_trackedVehicleClass);

        vm.addNativeMethod(g_motorcycleClass, "enableLeanController", motorcycle_enable_lean_controller);
        vm.addNativeMethod(g_motorcycleClass, "isLeanControllerEnabled", motorcycle_is_lean_controller_enabled);
        vm.addNativeMethod(g_motorcycleClass, "enableLeanSteeringLimit", motorcycle_enable_lean_steering_limit);
        vm.addNativeMethod(g_motorcycleClass, "isLeanSteeringLimitEnabled", motorcycle_is_lean_steering_limit_enabled);
        vm.addNativeMethod(g_motorcycleClass, "setLeanSpringConstant", motorcycle_set_lean_spring_constant);
        vm.addNativeMethod(g_motorcycleClass, "getLeanSpringConstant", motorcycle_get_lean_spring_constant);

        vm.addNativeMethod(g_worldClass, "createWheeledVehicle", world_create_wheeled_vehicle);
        vm.addNativeMethod(g_worldClass, "createMotorcycle", world_create_motorcycle);
        vm.addNativeMethod(g_worldClass, "createTrackedVehicle", world_create_tracked_vehicle);
    }
}
