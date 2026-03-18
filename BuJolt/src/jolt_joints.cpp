#include "jolt_joints.hpp"
#include "jolt_core.hpp"

#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <Jolt/Physics/Constraints/TwoBodyConstraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Constraints/ConeConstraint.h>

namespace JoltBindings
{
    static NativeClassDef *g_pointConstraintClass = nullptr;
    static NativeClassDef *g_distanceConstraintClass = nullptr;
    static NativeClassDef *g_hingeConstraintClass = nullptr;
    static NativeClassDef *g_sliderConstraintClass = nullptr;
    static NativeClassDef *g_fixedConstraintClass = nullptr;
    static NativeClassDef *g_coneConstraintClass = nullptr;

    static JoltBodyHandle *require_body_value_arg(Interpreter *vm, const Value &value, const char *fn, int argIndex)
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

    static JoltConstraintHandle *require_constraint_handle(void *instance, EConstraintSubType expectedSubType, const char *fn)
    {
        JoltConstraintHandle *constraint = (JoltConstraintHandle *)instance;
        if (!constraint || !constraint->valid || !constraint->world || !constraint->world->valid || !constraint->constraint)
        {
            Error("%s on invalid constraint", fn);
            return nullptr;
        }

        if (constraint->subType != expectedSubType)
        {
            Error("%s on wrong constraint type", fn);
            return nullptr;
        }

        return constraint;
    }

    static bool push_constraint_handle(Interpreter *vm, NativeClassDef *klass, JoltConstraintHandle *constraint)
    {
        if (!vm || !klass || !constraint)
            return false;

        Value value = vm->makeNativeClassInstance(false);
        NativeClassInstance *inst = value.asNativeClassInstance();
        if (!inst)
            return false;

        inst->klass = klass;
        inst->userData = constraint;
        constraint->wrapperAlive = true;
        vm->push(value);
        return true;
    }

    static bool build_two_body_constraint(Interpreter *vm,
                                          JoltWorldHandle *world,
                                          JoltBodyHandle *bodyA,
                                          JoltBodyHandle *bodyB,
                                          const char *fn,
                                          Constraint **outConstraint,
                                          EConstraintSubType *outSubType,
                                          PointConstraintSettings *pointSettings,
                                          DistanceConstraintSettings *distanceSettings,
                                          HingeConstraintSettings *hingeSettings,
                                          SliderConstraintSettings *sliderSettings)
    {
        if (!vm || !world || !bodyA || !bodyB || !outConstraint || !outSubType)
            return false;

        BodyID ids[2] = { bodyA->id, bodyB->id };
        BodyLockMultiWrite lock(world->physicsSystem.GetBodyLockInterface(), ids, 2);
        Body *joltBodyA = lock.GetBody(0);
        Body *joltBodyB = lock.GetBody(1);
        if (!joltBodyA || !joltBodyB)
        {
            Error("%s could not lock bodies", fn);
            return false;
        }

        if (pointSettings != nullptr)
        {
            *outConstraint = new PointConstraint(*joltBodyA, *joltBodyB, *pointSettings);
            *outSubType = EConstraintSubType::Point;
            return true;
        }

        if (distanceSettings != nullptr)
        {
            *outConstraint = new DistanceConstraint(*joltBodyA, *joltBodyB, *distanceSettings);
            *outSubType = EConstraintSubType::Distance;
            return true;
        }

        if (hingeSettings != nullptr)
        {
            *outConstraint = new HingeConstraint(*joltBodyA, *joltBodyB, *hingeSettings);
            *outSubType = EConstraintSubType::Hinge;
            return true;
        }

        if (sliderSettings != nullptr)
        {
            *outConstraint = new SliderConstraint(*joltBodyA, *joltBodyB, *sliderSettings);
            *outSubType = EConstraintSubType::Slider;
            return true;
        }

        Error("%s internal error: no settings supplied", fn);
        return false;
    }

    static bool finalize_constraint_creation(Interpreter *vm, JoltWorldHandle *world, Constraint *constraint, EConstraintSubType subType, NativeClassDef *klass)
    {
        if (!vm || !world || !constraint || !klass)
            return false;

        JoltConstraintHandle *handle = create_constraint_handle(world, constraint, subType);
        if (!handle)
            return false;

        if (push_constraint_handle(vm, klass, handle))
            return true;

        destroy_constraint_runtime(handle);
        delete handle;
        return false;
    }

    static int constraint_destroy(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("Constraint.destroy() expects 0 arguments");
            return 0;
        }

        destroy_constraint_runtime((JoltConstraintHandle *)instance);
        return 0;
    }

    static int constraint_is_valid(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("Constraint.isValid() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *constraint = (JoltConstraintHandle *)instance;
        vm->pushBool(constraint != nullptr && constraint->valid && constraint->constraint != nullptr && constraint->world != nullptr && constraint->world->valid);
        return 1;
    }

    static int constraint_get_enabled(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("Constraint.getEnabled() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *constraint = (JoltConstraintHandle *)instance;
        if (!constraint || !constraint->valid || !constraint->constraint)
            return push_nil1(vm);

        vm->pushBool(constraint->constraint->GetEnabled());
        return 1;
    }

    static int constraint_set_enabled(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1 || !args[0].isBool())
        {
            Error("Constraint.setEnabled() expects (bool)");
            return 0;
        }

        JoltConstraintHandle *constraint = (JoltConstraintHandle *)instance;
        if (!constraint || !constraint->valid || !constraint->constraint)
            return 0;

        constraint->constraint->SetEnabled(args[0].asBool());
        return 0;
    }

    static int constraint_get_user_data(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("Constraint.getUserData() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *constraint = (JoltConstraintHandle *)instance;
        if (!constraint || !constraint->valid || !constraint->constraint)
            return push_nil1(vm);

        vm->pushDouble((double)constraint->constraint->GetUserData());
        return 1;
    }

    static int constraint_set_user_data(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("Constraint.setUserData() expects (number)");
            return 0;
        }

        JoltConstraintHandle *constraint = (JoltConstraintHandle *)instance;
        if (!constraint || !constraint->valid || !constraint->constraint)
            return 0;

        constraint->constraint->SetUserData((uint64)args[0].asInt());
        return 0;
    }

    static int constraint_get_sub_type(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("Constraint.getSubType() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *constraint = (JoltConstraintHandle *)instance;
        if (!constraint)
            return push_nil1(vm);

        vm->pushInt((int)constraint->subType);
        return 1;
    }

    static int point_constraint_set_point1(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltPointConstraint.setPoint1() expects (Vector3)");
            return 0;
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Point, "JoltPointConstraint.setPoint1()");
        if (!constraint)
            return 0;

        Vector3 point;
        if (!read_vector3_arg(args[0], &point, "JoltPointConstraint.setPoint1()", 1))
            return 0;

        ((PointConstraint *)constraint->constraint)->SetPoint1(EConstraintSpace::WorldSpace, to_jolt_rvec3(point));
        return 0;
    }

    static int point_constraint_set_point2(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltPointConstraint.setPoint2() expects (Vector3)");
            return 0;
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Point, "JoltPointConstraint.setPoint2()");
        if (!constraint)
            return 0;

        Vector3 point;
        if (!read_vector3_arg(args[0], &point, "JoltPointConstraint.setPoint2()", 1))
            return 0;

        ((PointConstraint *)constraint->constraint)->SetPoint2(EConstraintSpace::WorldSpace, to_jolt_rvec3(point));
        return 0;
    }

    static int distance_constraint_set_distance(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("JoltDistanceConstraint.setDistance() expects (min, max)");
            return 0;
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Distance, "JoltDistanceConstraint.setDistance()");
        if (!constraint)
            return 0;

        double minDistance = 0.0;
        double maxDistance = 0.0;
        if (!read_number_arg(args[0], &minDistance, "JoltDistanceConstraint.setDistance()", 1) ||
            !read_number_arg(args[1], &maxDistance, "JoltDistanceConstraint.setDistance()", 2))
            return 0;

        ((DistanceConstraint *)constraint->constraint)->SetDistance((float)minDistance, (float)maxDistance);
        return 0;
    }

    static int distance_constraint_get_min_distance(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltDistanceConstraint.getMinDistance() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Distance, "JoltDistanceConstraint.getMinDistance()");
        if (!constraint)
            return push_nil1(vm);

        vm->pushDouble((double)((DistanceConstraint *)constraint->constraint)->GetMinDistance());
        return 1;
    }

    static int distance_constraint_get_max_distance(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltDistanceConstraint.getMaxDistance() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Distance, "JoltDistanceConstraint.getMaxDistance()");
        if (!constraint)
            return push_nil1(vm);

        vm->pushDouble((double)((DistanceConstraint *)constraint->constraint)->GetMaxDistance());
        return 1;
    }

    static int hinge_constraint_get_current_angle(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltHingeConstraint.getCurrentAngle() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Hinge, "JoltHingeConstraint.getCurrentAngle()");
        if (!constraint)
            return push_nil1(vm);

        vm->pushDouble((double)((HingeConstraint *)constraint->constraint)->GetCurrentAngle());
        return 1;
    }

    static int hinge_constraint_set_limits(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("JoltHingeConstraint.setLimits() expects (min, max)");
            return 0;
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Hinge, "JoltHingeConstraint.setLimits()");
        if (!constraint)
            return 0;

        double minAngle = 0.0;
        double maxAngle = 0.0;
        if (!read_number_arg(args[0], &minAngle, "JoltHingeConstraint.setLimits()", 1) ||
            !read_number_arg(args[1], &maxAngle, "JoltHingeConstraint.setLimits()", 2))
            return 0;

        ((HingeConstraint *)constraint->constraint)->SetLimits((float)minAngle, (float)maxAngle);
        return 0;
    }

    static int hinge_constraint_get_limits_min(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltHingeConstraint.getLimitsMin() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Hinge, "JoltHingeConstraint.getLimitsMin()");
        if (!constraint)
            return push_nil1(vm);

        vm->pushDouble((double)((HingeConstraint *)constraint->constraint)->GetLimitsMin());
        return 1;
    }

    static int hinge_constraint_get_limits_max(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltHingeConstraint.getLimitsMax() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Hinge, "JoltHingeConstraint.getLimitsMax()");
        if (!constraint)
            return push_nil1(vm);

        vm->pushDouble((double)((HingeConstraint *)constraint->constraint)->GetLimitsMax());
        return 1;
    }

    static int hinge_constraint_set_max_friction_torque(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltHingeConstraint.setMaxFrictionTorque() expects (number)");
            return 0;
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Hinge, "JoltHingeConstraint.setMaxFrictionTorque()");
        if (!constraint)
            return 0;

        double torque = 0.0;
        if (!read_number_arg(args[0], &torque, "JoltHingeConstraint.setMaxFrictionTorque()", 1))
            return 0;

        ((HingeConstraint *)constraint->constraint)->SetMaxFrictionTorque((float)torque);
        return 0;
    }

    static int hinge_constraint_get_max_friction_torque(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltHingeConstraint.getMaxFrictionTorque() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Hinge, "JoltHingeConstraint.getMaxFrictionTorque()");
        if (!constraint)
            return push_nil1(vm);

        vm->pushDouble((double)((HingeConstraint *)constraint->constraint)->GetMaxFrictionTorque());
        return 1;
    }

    static int hinge_constraint_set_motor_state(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("JoltHingeConstraint.setMotorState() expects (int)");
            return 0;
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Hinge, "JoltHingeConstraint.setMotorState()");
        if (!constraint)
            return 0;

        ((HingeConstraint *)constraint->constraint)->SetMotorState((EMotorState)args[0].asInt());
        return 0;
    }

    static int hinge_constraint_get_motor_state(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltHingeConstraint.getMotorState() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Hinge, "JoltHingeConstraint.getMotorState()");
        if (!constraint)
            return push_nil1(vm);

        vm->pushInt((int)((HingeConstraint *)constraint->constraint)->GetMotorState());
        return 1;
    }

    static int hinge_constraint_set_target_angular_velocity(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltHingeConstraint.setTargetAngularVelocity() expects (number)");
            return 0;
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Hinge, "JoltHingeConstraint.setTargetAngularVelocity()");
        if (!constraint)
            return 0;

        double velocity = 0.0;
        if (!read_number_arg(args[0], &velocity, "JoltHingeConstraint.setTargetAngularVelocity()", 1))
            return 0;

        ((HingeConstraint *)constraint->constraint)->SetTargetAngularVelocity((float)velocity);
        return 0;
    }

    static int hinge_constraint_get_target_angular_velocity(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltHingeConstraint.getTargetAngularVelocity() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Hinge, "JoltHingeConstraint.getTargetAngularVelocity()");
        if (!constraint)
            return push_nil1(vm);

        vm->pushDouble((double)((HingeConstraint *)constraint->constraint)->GetTargetAngularVelocity());
        return 1;
    }

    static int hinge_constraint_set_target_angle(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltHingeConstraint.setTargetAngle() expects (number)");
            return 0;
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Hinge, "JoltHingeConstraint.setTargetAngle()");
        if (!constraint)
            return 0;

        double angle = 0.0;
        if (!read_number_arg(args[0], &angle, "JoltHingeConstraint.setTargetAngle()", 1))
            return 0;

        ((HingeConstraint *)constraint->constraint)->SetTargetAngle((float)angle);
        return 0;
    }

    static int hinge_constraint_get_target_angle(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltHingeConstraint.getTargetAngle() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Hinge, "JoltHingeConstraint.getTargetAngle()");
        if (!constraint)
            return push_nil1(vm);

        vm->pushDouble((double)((HingeConstraint *)constraint->constraint)->GetTargetAngle());
        return 1;
    }

    static int hinge_constraint_set_motor_torque_limit(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltHingeConstraint.setMotorTorqueLimit() expects (number)");
            return 0;
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Hinge, "JoltHingeConstraint.setMotorTorqueLimit()");
        if (!constraint)
            return 0;

        double limit = 0.0;
        if (!read_number_arg(args[0], &limit, "JoltHingeConstraint.setMotorTorqueLimit()", 1))
            return 0;

        ((HingeConstraint *)constraint->constraint)->GetMotorSettings().SetTorqueLimit((float)limit);
        return 0;
    }

    static int hinge_constraint_get_motor_torque_limit(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltHingeConstraint.getMotorTorqueLimit() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Hinge, "JoltHingeConstraint.getMotorTorqueLimit()");
        if (!constraint)
            return push_nil1(vm);

        vm->pushDouble((double)((HingeConstraint *)constraint->constraint)->GetMotorSettings().mMaxTorqueLimit);
        return 1;
    }

    static int slider_constraint_get_current_position(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltSliderConstraint.getCurrentPosition() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Slider, "JoltSliderConstraint.getCurrentPosition()");
        if (!constraint)
            return push_nil1(vm);

        vm->pushDouble((double)((SliderConstraint *)constraint->constraint)->GetCurrentPosition());
        return 1;
    }

    static int slider_constraint_set_limits(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("JoltSliderConstraint.setLimits() expects (min, max)");
            return 0;
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Slider, "JoltSliderConstraint.setLimits()");
        if (!constraint)
            return 0;

        double minValue = 0.0;
        double maxValue = 0.0;
        if (!read_number_arg(args[0], &minValue, "JoltSliderConstraint.setLimits()", 1) ||
            !read_number_arg(args[1], &maxValue, "JoltSliderConstraint.setLimits()", 2))
            return 0;

        ((SliderConstraint *)constraint->constraint)->SetLimits((float)minValue, (float)maxValue);
        return 0;
    }

    static int slider_constraint_get_limits_min(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltSliderConstraint.getLimitsMin() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Slider, "JoltSliderConstraint.getLimitsMin()");
        if (!constraint)
            return push_nil1(vm);

        vm->pushDouble((double)((SliderConstraint *)constraint->constraint)->GetLimitsMin());
        return 1;
    }

    static int slider_constraint_get_limits_max(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltSliderConstraint.getLimitsMax() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Slider, "JoltSliderConstraint.getLimitsMax()");
        if (!constraint)
            return push_nil1(vm);

        vm->pushDouble((double)((SliderConstraint *)constraint->constraint)->GetLimitsMax());
        return 1;
    }

    static int slider_constraint_set_max_friction_force(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltSliderConstraint.setMaxFrictionForce() expects (number)");
            return 0;
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Slider, "JoltSliderConstraint.setMaxFrictionForce()");
        if (!constraint)
            return 0;

        double force = 0.0;
        if (!read_number_arg(args[0], &force, "JoltSliderConstraint.setMaxFrictionForce()", 1))
            return 0;

        ((SliderConstraint *)constraint->constraint)->SetMaxFrictionForce((float)force);
        return 0;
    }

    static int slider_constraint_get_max_friction_force(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltSliderConstraint.getMaxFrictionForce() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Slider, "JoltSliderConstraint.getMaxFrictionForce()");
        if (!constraint)
            return push_nil1(vm);

        vm->pushDouble((double)((SliderConstraint *)constraint->constraint)->GetMaxFrictionForce());
        return 1;
    }

    static int slider_constraint_set_motor_state(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("JoltSliderConstraint.setMotorState() expects (int)");
            return 0;
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Slider, "JoltSliderConstraint.setMotorState()");
        if (!constraint)
            return 0;

        ((SliderConstraint *)constraint->constraint)->SetMotorState((EMotorState)args[0].asInt());
        return 0;
    }

    static int slider_constraint_get_motor_state(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltSliderConstraint.getMotorState() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Slider, "JoltSliderConstraint.getMotorState()");
        if (!constraint)
            return push_nil1(vm);

        vm->pushInt((int)((SliderConstraint *)constraint->constraint)->GetMotorState());
        return 1;
    }

    static int slider_constraint_set_target_velocity(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltSliderConstraint.setTargetVelocity() expects (number)");
            return 0;
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Slider, "JoltSliderConstraint.setTargetVelocity()");
        if (!constraint)
            return 0;

        double value = 0.0;
        if (!read_number_arg(args[0], &value, "JoltSliderConstraint.setTargetVelocity()", 1))
            return 0;

        ((SliderConstraint *)constraint->constraint)->SetTargetVelocity((float)value);
        return 0;
    }

    static int slider_constraint_get_target_velocity(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltSliderConstraint.getTargetVelocity() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Slider, "JoltSliderConstraint.getTargetVelocity()");
        if (!constraint)
            return push_nil1(vm);

        vm->pushDouble((double)((SliderConstraint *)constraint->constraint)->GetTargetVelocity());
        return 1;
    }

    static int slider_constraint_set_target_position(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltSliderConstraint.setTargetPosition() expects (number)");
            return 0;
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Slider, "JoltSliderConstraint.setTargetPosition()");
        if (!constraint)
            return 0;

        double value = 0.0;
        if (!read_number_arg(args[0], &value, "JoltSliderConstraint.setTargetPosition()", 1))
            return 0;

        ((SliderConstraint *)constraint->constraint)->SetTargetPosition((float)value);
        return 0;
    }

    static int slider_constraint_get_target_position(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltSliderConstraint.getTargetPosition() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Slider, "JoltSliderConstraint.getTargetPosition()");
        if (!constraint)
            return push_nil1(vm);

        vm->pushDouble((double)((SliderConstraint *)constraint->constraint)->GetTargetPosition());
        return 1;
    }

    static int slider_constraint_set_motor_force_limit(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltSliderConstraint.setMotorForceLimit() expects (number)");
            return 0;
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Slider, "JoltSliderConstraint.setMotorForceLimit()");
        if (!constraint)
            return 0;

        double value = 0.0;
        if (!read_number_arg(args[0], &value, "JoltSliderConstraint.setMotorForceLimit()", 1))
            return 0;

        ((SliderConstraint *)constraint->constraint)->GetMotorSettings().SetForceLimit((float)value);
        return 0;
    }

    static int slider_constraint_get_motor_force_limit(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltSliderConstraint.getMotorForceLimit() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Slider, "JoltSliderConstraint.getMotorForceLimit()");
        if (!constraint)
            return push_nil1(vm);

        vm->pushDouble((double)((SliderConstraint *)constraint->constraint)->GetMotorSettings().mMaxForceLimit);
        return 1;
    }

    static int world_create_point_constraint(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 3 && argCount != 4)
        {
            Error("JoltWorld.createPointConstraint() expects (bodyA, bodyB, point[, point2])");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.createPointConstraint()");
        if (!world)
            return push_nil1(vm);

        JoltBodyHandle *bodyA = require_body_value_arg(vm, args[0], "JoltWorld.createPointConstraint()", 1);
        JoltBodyHandle *bodyB = require_body_value_arg(vm, args[1], "JoltWorld.createPointConstraint()", 2);
        if (!bodyA || !bodyB)
            return push_nil1(vm);

        Vector3 point1;
        Vector3 point2;
        if (!read_vector3_arg(args[2], &point1, "JoltWorld.createPointConstraint()", 3))
            return push_nil1(vm);
        point2 = point1;
        if (argCount == 4 && !read_vector3_arg(args[3], &point2, "JoltWorld.createPointConstraint()", 4))
            return push_nil1(vm);

        PointConstraintSettings settings;
        settings.mSpace = EConstraintSpace::WorldSpace;
        settings.mPoint1 = to_jolt_rvec3(point1);
        settings.mPoint2 = to_jolt_rvec3(point2);

        Constraint *constraint = nullptr;
        EConstraintSubType subType = EConstraintSubType::Point;
        if (!build_two_body_constraint(vm, world, bodyA, bodyB, "JoltWorld.createPointConstraint()", &constraint, &subType, &settings, nullptr, nullptr, nullptr))
            return push_nil1(vm);

        return finalize_constraint_creation(vm, world, constraint, subType, g_pointConstraintClass) ? 1 : push_nil1(vm);
    }

    static int world_create_distance_constraint(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 6)
        {
            Error("JoltWorld.createDistanceConstraint() expects (bodyA, bodyB, point1, point2, minDistance, maxDistance)");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.createDistanceConstraint()");
        if (!world)
            return push_nil1(vm);

        JoltBodyHandle *bodyA = require_body_value_arg(vm, args[0], "JoltWorld.createDistanceConstraint()", 1);
        JoltBodyHandle *bodyB = require_body_value_arg(vm, args[1], "JoltWorld.createDistanceConstraint()", 2);
        if (!bodyA || !bodyB)
            return push_nil1(vm);

        Vector3 point1;
        Vector3 point2;
        double minDistance = 0.0;
        double maxDistance = 0.0;
        if (!read_vector3_arg(args[2], &point1, "JoltWorld.createDistanceConstraint()", 3) ||
            !read_vector3_arg(args[3], &point2, "JoltWorld.createDistanceConstraint()", 4) ||
            !read_number_arg(args[4], &minDistance, "JoltWorld.createDistanceConstraint()", 5) ||
            !read_number_arg(args[5], &maxDistance, "JoltWorld.createDistanceConstraint()", 6))
            return push_nil1(vm);

        DistanceConstraintSettings settings;
        settings.mSpace = EConstraintSpace::WorldSpace;
        settings.mPoint1 = to_jolt_rvec3(point1);
        settings.mPoint2 = to_jolt_rvec3(point2);
        settings.mMinDistance = (float)minDistance;
        settings.mMaxDistance = (float)maxDistance;

        Constraint *constraint = nullptr;
        EConstraintSubType subType = EConstraintSubType::Distance;
        if (!build_two_body_constraint(vm, world, bodyA, bodyB, "JoltWorld.createDistanceConstraint()", &constraint, &subType, nullptr, &settings, nullptr, nullptr))
            return push_nil1(vm);

        return finalize_constraint_creation(vm, world, constraint, subType, g_distanceConstraintClass) ? 1 : push_nil1(vm);
    }

    static int world_create_hinge_constraint(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 5 && argCount != 7)
        {
            Error("JoltWorld.createHingeConstraint() expects (bodyA, bodyB, anchor, axis, normal[, minAngle, maxAngle])");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.createHingeConstraint()");
        if (!world)
            return push_nil1(vm);

        JoltBodyHandle *bodyA = require_body_value_arg(vm, args[0], "JoltWorld.createHingeConstraint()", 1);
        JoltBodyHandle *bodyB = require_body_value_arg(vm, args[1], "JoltWorld.createHingeConstraint()", 2);
        if (!bodyA || !bodyB)
            return push_nil1(vm);

        Vector3 anchor;
        Vector3 axis;
        Vector3 normal;
        if (!read_vector3_arg(args[2], &anchor, "JoltWorld.createHingeConstraint()", 3) ||
            !read_vector3_arg(args[3], &axis, "JoltWorld.createHingeConstraint()", 4) ||
            !read_vector3_arg(args[4], &normal, "JoltWorld.createHingeConstraint()", 5))
            return push_nil1(vm);

        HingeConstraintSettings settings;
        settings.mSpace = EConstraintSpace::WorldSpace;
        settings.mPoint1 = to_jolt_rvec3(anchor);
        settings.mPoint2 = to_jolt_rvec3(anchor);
        settings.mHingeAxis1 = to_jolt_vec3(axis).Normalized();
        settings.mHingeAxis2 = settings.mHingeAxis1;
        settings.mNormalAxis1 = to_jolt_vec3(normal).Normalized();
        settings.mNormalAxis2 = settings.mNormalAxis1;

        if (argCount == 7)
        {
            double minAngle = 0.0;
            double maxAngle = 0.0;
            if (!read_number_arg(args[5], &minAngle, "JoltWorld.createHingeConstraint()", 6) ||
                !read_number_arg(args[6], &maxAngle, "JoltWorld.createHingeConstraint()", 7))
                return push_nil1(vm);

            settings.mLimitsMin = (float)minAngle;
            settings.mLimitsMax = (float)maxAngle;
        }

        Constraint *constraint = nullptr;
        EConstraintSubType subType = EConstraintSubType::Hinge;
        if (!build_two_body_constraint(vm, world, bodyA, bodyB, "JoltWorld.createHingeConstraint()", &constraint, &subType, nullptr, nullptr, &settings, nullptr))
            return push_nil1(vm);

        return finalize_constraint_creation(vm, world, constraint, subType, g_hingeConstraintClass) ? 1 : push_nil1(vm);
    }

    static int world_create_slider_constraint(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 5 && argCount != 7)
        {
            Error("JoltWorld.createSliderConstraint() expects (bodyA, bodyB, anchor, axis, normal[, minValue, maxValue])");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.createSliderConstraint()");
        if (!world)
            return push_nil1(vm);

        JoltBodyHandle *bodyA = require_body_value_arg(vm, args[0], "JoltWorld.createSliderConstraint()", 1);
        JoltBodyHandle *bodyB = require_body_value_arg(vm, args[1], "JoltWorld.createSliderConstraint()", 2);
        if (!bodyA || !bodyB)
            return push_nil1(vm);

        Vector3 anchor;
        Vector3 axis;
        Vector3 normal;
        if (!read_vector3_arg(args[2], &anchor, "JoltWorld.createSliderConstraint()", 3) ||
            !read_vector3_arg(args[3], &axis, "JoltWorld.createSliderConstraint()", 4) ||
            !read_vector3_arg(args[4], &normal, "JoltWorld.createSliderConstraint()", 5))
            return push_nil1(vm);

        SliderConstraintSettings settings;
        settings.mSpace = EConstraintSpace::WorldSpace;
        settings.mPoint1 = to_jolt_rvec3(anchor);
        settings.mPoint2 = to_jolt_rvec3(anchor);
        settings.mSliderAxis1 = to_jolt_vec3(axis).Normalized();
        settings.mSliderAxis2 = settings.mSliderAxis1;
        settings.mNormalAxis1 = to_jolt_vec3(normal).Normalized();
        settings.mNormalAxis2 = settings.mNormalAxis1;

        if (argCount == 7)
        {
            double minValue = 0.0;
            double maxValue = 0.0;
            if (!read_number_arg(args[5], &minValue, "JoltWorld.createSliderConstraint()", 6) ||
                !read_number_arg(args[6], &maxValue, "JoltWorld.createSliderConstraint()", 7))
                return push_nil1(vm);

            settings.mLimitsMin = (float)minValue;
            settings.mLimitsMax = (float)maxValue;
        }

        Constraint *constraint = nullptr;
        EConstraintSubType subType = EConstraintSubType::Slider;
        if (!build_two_body_constraint(vm, world, bodyA, bodyB, "JoltWorld.createSliderConstraint()", &constraint, &subType, nullptr, nullptr, nullptr, &settings))
            return push_nil1(vm);

        return finalize_constraint_creation(vm, world, constraint, subType, g_sliderConstraintClass) ? 1 : push_nil1(vm);
    }

    // ─── Fixed Constraint ─────────────────────────────────────────────────

    static int world_create_fixed_constraint(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("JoltWorld.createFixedConstraint() expects (bodyA, bodyB)");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.createFixedConstraint()");
        if (!world)
            return push_nil1(vm);

        JoltBodyHandle *bodyA = require_body_value_arg(vm, args[0], "JoltWorld.createFixedConstraint()", 1);
        JoltBodyHandle *bodyB = require_body_value_arg(vm, args[1], "JoltWorld.createFixedConstraint()", 2);
        if (!bodyA || !bodyB)
            return push_nil1(vm);

        FixedConstraintSettings settings;
        settings.mAutoDetectPoint = true;

        BodyID ids[2] = { bodyA->id, bodyB->id };
        BodyLockMultiWrite lock(world->physicsSystem.GetBodyLockInterface(), ids, 2);
        Body *joltBodyA = lock.GetBody(0);
        Body *joltBodyB = lock.GetBody(1);
        if (!joltBodyA || !joltBodyB)
        {
            Error("JoltWorld.createFixedConstraint() could not lock bodies");
            return push_nil1(vm);
        }

        Constraint *constraint = settings.Create(*joltBodyA, *joltBodyB);
        if (!constraint)
        {
            Error("JoltWorld.createFixedConstraint() failed to create constraint");
            return push_nil1(vm);
        }

        return finalize_constraint_creation(vm, world, constraint, EConstraintSubType::Fixed, g_fixedConstraintClass) ? 1 : push_nil1(vm);
    }

    // ─── Cone Constraint ──────────────────────────────────────────────────

    static int world_create_cone_constraint(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 4 && argCount != 5)
        {
            Error("JoltWorld.createConeConstraint() expects (bodyA, bodyB, anchor, axis[, halfConeAngle])");
            return push_nil1(vm);
        }

        JoltWorldHandle *world = require_world(instance, "JoltWorld.createConeConstraint()");
        if (!world)
            return push_nil1(vm);

        JoltBodyHandle *bodyA = require_body_value_arg(vm, args[0], "JoltWorld.createConeConstraint()", 1);
        JoltBodyHandle *bodyB = require_body_value_arg(vm, args[1], "JoltWorld.createConeConstraint()", 2);
        if (!bodyA || !bodyB)
            return push_nil1(vm);

        Vector3 anchor;
        Vector3 axis;
        if (!read_vector3_arg(args[2], &anchor, "JoltWorld.createConeConstraint()", 3) ||
            !read_vector3_arg(args[3], &axis, "JoltWorld.createConeConstraint()", 4))
            return push_nil1(vm);

        double halfConeAngle = JPH_PI;
        if (argCount == 5 && !read_number_arg(args[4], &halfConeAngle, "JoltWorld.createConeConstraint()", 5))
            return push_nil1(vm);

        ConeConstraintSettings settings;
        settings.mSpace = EConstraintSpace::WorldSpace;
        settings.mPoint1 = to_jolt_rvec3(anchor);
        settings.mPoint2 = to_jolt_rvec3(anchor);
        settings.mTwistAxis1 = to_jolt_vec3(axis).Normalized();
        settings.mTwistAxis2 = settings.mTwistAxis1;
        settings.mHalfConeAngle = (float)halfConeAngle;

        BodyID ids[2] = { bodyA->id, bodyB->id };
        BodyLockMultiWrite lock(world->physicsSystem.GetBodyLockInterface(), ids, 2);
        Body *joltBodyA = lock.GetBody(0);
        Body *joltBodyB = lock.GetBody(1);
        if (!joltBodyA || !joltBodyB)
        {
            Error("JoltWorld.createConeConstraint() could not lock bodies");
            return push_nil1(vm);
        }

        Constraint *constraint = settings.Create(*joltBodyA, *joltBodyB);
        if (!constraint)
        {
            Error("JoltWorld.createConeConstraint() failed to create constraint");
            return push_nil1(vm);
        }

        return finalize_constraint_creation(vm, world, constraint, EConstraintSubType::Cone, g_coneConstraintClass) ? 1 : push_nil1(vm);
    }

    // ─── Cone constraint specific methods ─────────────────────────────────

    static int cone_constraint_set_half_cone_angle(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltConeConstraint.setHalfConeAngle() expects (number)");
            return 0;
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Cone, "JoltConeConstraint.setHalfConeAngle()");
        if (!constraint)
            return 0;

        double angle = 0.0;
        if (!read_number_arg(args[0], &angle, "JoltConeConstraint.setHalfConeAngle()", 1))
            return 0;

        ((ConeConstraint *)constraint->constraint)->SetHalfConeAngle((float)angle);
        return 0;
    }

    static int cone_constraint_get_cos_half_cone_angle(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltConeConstraint.getCosHalfConeAngle() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltConstraintHandle *constraint = require_constraint_handle(instance, EConstraintSubType::Cone, "JoltConeConstraint.getCosHalfConeAngle()");
        if (!constraint)
            return push_nil1(vm);

        vm->pushDouble((double)((ConeConstraint *)constraint->constraint)->GetCosHalfConeAngle());
        return 1;
    }

    static void *constraint_ctor_error(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)argCount;
        (void)args;
        Error("Jolt constraints cannot be constructed directly; use JoltWorld.create*Constraint()");
        return nullptr;
    }

    static void constraint_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        JoltConstraintHandle *constraint = (JoltConstraintHandle *)instance;
        if (!constraint)
            return;

        constraint->wrapperAlive = false;

        if (constraint->world == nullptr || !constraint->valid)
            delete constraint;
    }

    static void add_common_constraint_methods(Interpreter &vm, NativeClassDef *klass)
    {
        vm.addNativeMethod(klass, "destroy", constraint_destroy);
        vm.addNativeMethod(klass, "isValid", constraint_is_valid);
        vm.addNativeMethod(klass, "getEnabled", constraint_get_enabled);
        vm.addNativeMethod(klass, "setEnabled", constraint_set_enabled);
        vm.addNativeMethod(klass, "getUserData", constraint_get_user_data);
        vm.addNativeMethod(klass, "setUserData", constraint_set_user_data);
        vm.addNativeMethod(klass, "getSubType", constraint_get_sub_type);
    }

    void register_jolt_joints(Interpreter &vm)
    {
        g_pointConstraintClass = vm.registerNativeClass("JoltPointConstraint", constraint_ctor_error, constraint_dtor, 0, false);
        g_distanceConstraintClass = vm.registerNativeClass("JoltDistanceConstraint", constraint_ctor_error, constraint_dtor, 0, false);
        g_hingeConstraintClass = vm.registerNativeClass("JoltHingeConstraint", constraint_ctor_error, constraint_dtor, 0, false);
        g_sliderConstraintClass = vm.registerNativeClass("JoltSliderConstraint", constraint_ctor_error, constraint_dtor, 0, false);
        g_fixedConstraintClass = vm.registerNativeClass("JoltFixedConstraint", constraint_ctor_error, constraint_dtor, 0, false);
        g_coneConstraintClass = vm.registerNativeClass("JoltConeConstraint", constraint_ctor_error, constraint_dtor, 0, false);

        add_common_constraint_methods(vm, g_pointConstraintClass);
        add_common_constraint_methods(vm, g_distanceConstraintClass);
        add_common_constraint_methods(vm, g_hingeConstraintClass);
        add_common_constraint_methods(vm, g_sliderConstraintClass);
        add_common_constraint_methods(vm, g_fixedConstraintClass);
        add_common_constraint_methods(vm, g_coneConstraintClass);

        vm.addNativeMethod(g_pointConstraintClass, "setPoint1", point_constraint_set_point1);
        vm.addNativeMethod(g_pointConstraintClass, "setPoint2", point_constraint_set_point2);

        vm.addNativeMethod(g_distanceConstraintClass, "setDistance", distance_constraint_set_distance);
        vm.addNativeMethod(g_distanceConstraintClass, "getMinDistance", distance_constraint_get_min_distance);
        vm.addNativeMethod(g_distanceConstraintClass, "getMaxDistance", distance_constraint_get_max_distance);

        vm.addNativeMethod(g_hingeConstraintClass, "getCurrentAngle", hinge_constraint_get_current_angle);
        vm.addNativeMethod(g_hingeConstraintClass, "setLimits", hinge_constraint_set_limits);
        vm.addNativeMethod(g_hingeConstraintClass, "getLimitsMin", hinge_constraint_get_limits_min);
        vm.addNativeMethod(g_hingeConstraintClass, "getLimitsMax", hinge_constraint_get_limits_max);
        vm.addNativeMethod(g_hingeConstraintClass, "setMaxFrictionTorque", hinge_constraint_set_max_friction_torque);
        vm.addNativeMethod(g_hingeConstraintClass, "getMaxFrictionTorque", hinge_constraint_get_max_friction_torque);
        vm.addNativeMethod(g_hingeConstraintClass, "setMotorState", hinge_constraint_set_motor_state);
        vm.addNativeMethod(g_hingeConstraintClass, "getMotorState", hinge_constraint_get_motor_state);
        vm.addNativeMethod(g_hingeConstraintClass, "setTargetAngularVelocity", hinge_constraint_set_target_angular_velocity);
        vm.addNativeMethod(g_hingeConstraintClass, "getTargetAngularVelocity", hinge_constraint_get_target_angular_velocity);
        vm.addNativeMethod(g_hingeConstraintClass, "setTargetAngle", hinge_constraint_set_target_angle);
        vm.addNativeMethod(g_hingeConstraintClass, "getTargetAngle", hinge_constraint_get_target_angle);
        vm.addNativeMethod(g_hingeConstraintClass, "setMotorTorqueLimit", hinge_constraint_set_motor_torque_limit);
        vm.addNativeMethod(g_hingeConstraintClass, "getMotorTorqueLimit", hinge_constraint_get_motor_torque_limit);

        vm.addNativeMethod(g_sliderConstraintClass, "getCurrentPosition", slider_constraint_get_current_position);
        vm.addNativeMethod(g_sliderConstraintClass, "setLimits", slider_constraint_set_limits);
        vm.addNativeMethod(g_sliderConstraintClass, "getLimitsMin", slider_constraint_get_limits_min);
        vm.addNativeMethod(g_sliderConstraintClass, "getLimitsMax", slider_constraint_get_limits_max);
        vm.addNativeMethod(g_sliderConstraintClass, "setMaxFrictionForce", slider_constraint_set_max_friction_force);
        vm.addNativeMethod(g_sliderConstraintClass, "getMaxFrictionForce", slider_constraint_get_max_friction_force);
        vm.addNativeMethod(g_sliderConstraintClass, "setMotorState", slider_constraint_set_motor_state);
        vm.addNativeMethod(g_sliderConstraintClass, "getMotorState", slider_constraint_get_motor_state);
        vm.addNativeMethod(g_sliderConstraintClass, "setTargetVelocity", slider_constraint_set_target_velocity);
        vm.addNativeMethod(g_sliderConstraintClass, "getTargetVelocity", slider_constraint_get_target_velocity);
        vm.addNativeMethod(g_sliderConstraintClass, "setTargetPosition", slider_constraint_set_target_position);
        vm.addNativeMethod(g_sliderConstraintClass, "getTargetPosition", slider_constraint_get_target_position);
        vm.addNativeMethod(g_sliderConstraintClass, "setMotorForceLimit", slider_constraint_set_motor_force_limit);
        vm.addNativeMethod(g_sliderConstraintClass, "getMotorForceLimit", slider_constraint_get_motor_force_limit);

        vm.addNativeMethod(g_worldClass, "createPointConstraint", world_create_point_constraint);
        vm.addNativeMethod(g_worldClass, "createDistanceConstraint", world_create_distance_constraint);
        vm.addNativeMethod(g_worldClass, "createHingeConstraint", world_create_hinge_constraint);
        vm.addNativeMethod(g_worldClass, "createSliderConstraint", world_create_slider_constraint);
        vm.addNativeMethod(g_worldClass, "createFixedConstraint", world_create_fixed_constraint);
        vm.addNativeMethod(g_worldClass, "createConeConstraint", world_create_cone_constraint);

        vm.addNativeMethod(g_coneConstraintClass, "setHalfConeAngle", cone_constraint_set_half_cone_angle);
        vm.addNativeMethod(g_coneConstraintClass, "getCosHalfConeAngle", cone_constraint_get_cos_half_cone_angle);
    }
}
