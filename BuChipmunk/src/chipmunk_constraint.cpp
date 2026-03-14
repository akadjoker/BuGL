#include "chipmunk_common.hpp"

namespace ChipmunkBindings
{
    typedef cpBool (*ConstraintCheckFn)(const cpConstraint *constraint);

    static cpConstraint *require_constraint_kind(Interpreter *vm, const Value &value, const char *fn, int argIndex, const char *kindName, ConstraintCheckFn check)
    {
        cpConstraint *constraint = require_constraint_arg(vm, value, fn, argIndex);
        if (!constraint)
            return nullptr;

        if (check && !check(constraint))
        {
            Error("%s arg %d expects %s", fn, argIndex, kindName);
            return nullptr;
        }

        return constraint;
    }

    static int push_space_or_nil(Interpreter *vm, cpSpace *space)
    {
        if (!space)
        {
            vm->pushNil();
            return 1;
        }

        return push_space_handle(vm, space) ? 1 : 0;
    }

    static int push_body_or_nil(Interpreter *vm, cpBody *body)
    {
        if (!body)
        {
            vm->pushNil();
            return 1;
        }

        return push_body_handle(vm, body) ? 1 : 0;
    }

#define DEFINE_CONSTRAINT_IS_FN(nativeName, checkFn)                                                       \
    static int native_##nativeName(Interpreter *vm, int argc, Value *args)                                \
    {                                                                                                      \
        if (argc != 1)                                                                                     \
        {                                                                                                  \
            Error(#nativeName " expects (cpConstraint)");                                                  \
            return 0;                                                                                      \
        }                                                                                                  \
                                                                                                           \
        cpConstraint *constraint = require_constraint_arg(vm, args[0], #nativeName, 1);                   \
        if (!constraint)                                                                                   \
            return 0;                                                                                      \
                                                                                                           \
        vm->pushBool(checkFn(constraint) != 0);                                                            \
        return 1;                                                                                          \
    }

#define DEFINE_CONSTRAINT_VECT_GETTER(nativeName, kindName, checkFn, getterFn)                            \
    static int native_##nativeName(Interpreter *vm, int argc, Value *args)                                \
    {                                                                                                      \
        if (argc != 1)                                                                                     \
        {                                                                                                  \
            Error(#nativeName " expects (cpConstraint)");                                                  \
            return 0;                                                                                      \
        }                                                                                                  \
                                                                                                           \
        cpConstraint *constraint = require_constraint_kind(vm, args[0], #nativeName, 1, kindName, checkFn); \
        if (!constraint)                                                                                   \
            return 0;                                                                                      \
                                                                                                           \
        push_cpvect_components(vm, getterFn(constraint));                                                  \
        return 2;                                                                                          \
    }

#define DEFINE_CONSTRAINT_VECT_SETTER(nativeName, kindName, checkFn, setterFn)                            \
    static int native_##nativeName(Interpreter *vm, int argc, Value *args)                                \
    {                                                                                                      \
        if (argc != 2)                                                                                     \
        {                                                                                                  \
            Error(#nativeName " expects (cpConstraint, Vector2)");                                         \
            return 0;                                                                                      \
        }                                                                                                  \
                                                                                                           \
        cpConstraint *constraint = require_constraint_kind(vm, args[0], #nativeName, 1, kindName, checkFn); \
        cpVect value;                                                                                      \
        if (!constraint || !read_cpvect_arg(args[1], &value, #nativeName, 2))                             \
            return 0;                                                                                      \
                                                                                                           \
        setterFn(constraint, value);                                                                       \
        return 0;                                                                                          \
    }

#define DEFINE_CONSTRAINT_NUMBER_GETTER(nativeName, kindName, checkFn, getterFn)                          \
    static int native_##nativeName(Interpreter *vm, int argc, Value *args)                                \
    {                                                                                                      \
        if (argc != 1)                                                                                     \
        {                                                                                                  \
            Error(#nativeName " expects (cpConstraint)");                                                  \
            return 0;                                                                                      \
        }                                                                                                  \
                                                                                                           \
        cpConstraint *constraint = require_constraint_kind(vm, args[0], #nativeName, 1, kindName, checkFn); \
        if (!constraint)                                                                                   \
            return 0;                                                                                      \
                                                                                                           \
        vm->pushDouble((double)getterFn(constraint));                                                      \
        return 1;                                                                                          \
    }

#define DEFINE_CONSTRAINT_NUMBER_SETTER(nativeName, kindName, checkFn, setterFn, valueLabel)             \
    static int native_##nativeName(Interpreter *vm, int argc, Value *args)                                \
    {                                                                                                      \
        if (argc != 2)                                                                                     \
        {                                                                                                  \
            Error(#nativeName " expects (cpConstraint, " valueLabel ")");                                  \
            return 0;                                                                                      \
        }                                                                                                  \
                                                                                                           \
        cpConstraint *constraint = require_constraint_kind(vm, args[0], #nativeName, 1, kindName, checkFn); \
        double value = 0.0;                                                                                \
        if (!constraint || !read_number_arg(args[1], &value, #nativeName, 2))                             \
            return 0;                                                                                      \
                                                                                                           \
        setterFn(constraint, (cpFloat)value);                                                              \
        return 0;                                                                                          \
    }

    static int native_cpConstraintFree(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpConstraintFree expects (cpConstraint)");
            return 0;
        }

        cpConstraint *constraint = require_constraint_arg(vm, args[0], "cpConstraintFree", 1);
        if (!constraint)
            return 0;
        if (cpConstraintGetSpace(constraint) != nullptr)
        {
            Error("cpConstraintFree expects constraint removed from cpSpace first");
            return 0;
        }

        cpConstraintFree(constraint);
        invalidate_native_handle(args[0]);
        return 0;
    }

    static int native_cpConstraintGetSpace(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpConstraintGetSpace expects (cpConstraint)");
            return 0;
        }

        cpConstraint *constraint = require_constraint_arg(vm, args[0], "cpConstraintGetSpace", 1);
        if (!constraint)
            return 0;

        return push_space_or_nil(vm, cpConstraintGetSpace(constraint));
    }

    static int native_cpConstraintGetBodyA(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpConstraintGetBodyA expects (cpConstraint)");
            return 0;
        }

        cpConstraint *constraint = require_constraint_arg(vm, args[0], "cpConstraintGetBodyA", 1);
        if (!constraint)
            return 0;

        return push_body_or_nil(vm, cpConstraintGetBodyA(constraint));
    }

    static int native_cpConstraintGetBodyB(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpConstraintGetBodyB expects (cpConstraint)");
            return 0;
        }

        cpConstraint *constraint = require_constraint_arg(vm, args[0], "cpConstraintGetBodyB", 1);
        if (!constraint)
            return 0;

        return push_body_or_nil(vm, cpConstraintGetBodyB(constraint));
    }

    DEFINE_CONSTRAINT_NUMBER_GETTER(cpConstraintGetMaxForce, "cpConstraint", nullptr, cpConstraintGetMaxForce)
    DEFINE_CONSTRAINT_NUMBER_SETTER(cpConstraintSetMaxForce, "cpConstraint", nullptr, cpConstraintSetMaxForce, "maxForce")
    DEFINE_CONSTRAINT_NUMBER_GETTER(cpConstraintGetErrorBias, "cpConstraint", nullptr, cpConstraintGetErrorBias)
    DEFINE_CONSTRAINT_NUMBER_SETTER(cpConstraintSetErrorBias, "cpConstraint", nullptr, cpConstraintSetErrorBias, "errorBias")
    DEFINE_CONSTRAINT_NUMBER_GETTER(cpConstraintGetMaxBias, "cpConstraint", nullptr, cpConstraintGetMaxBias)
    DEFINE_CONSTRAINT_NUMBER_SETTER(cpConstraintSetMaxBias, "cpConstraint", nullptr, cpConstraintSetMaxBias, "maxBias")
    DEFINE_CONSTRAINT_NUMBER_GETTER(cpConstraintGetImpulse, "cpConstraint", nullptr, cpConstraintGetImpulse)

    static int native_cpConstraintGetCollideBodies(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpConstraintGetCollideBodies expects (cpConstraint)");
            return 0;
        }

        cpConstraint *constraint = require_constraint_arg(vm, args[0], "cpConstraintGetCollideBodies", 1);
        if (!constraint)
            return 0;

        vm->pushBool(cpConstraintGetCollideBodies(constraint) != 0);
        return 1;
    }

    static int native_cpConstraintSetCollideBodies(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2 || !args[1].isBool())
        {
            Error("cpConstraintSetCollideBodies expects (cpConstraint, bool)");
            return 0;
        }

        cpConstraint *constraint = require_constraint_arg(vm, args[0], "cpConstraintSetCollideBodies", 1);
        if (!constraint)
            return 0;

        cpConstraintSetCollideBodies(constraint, args[1].asBool() ? cpTrue : cpFalse);
        return 0;
    }

    DEFINE_CONSTRAINT_IS_FN(cpConstraintIsPinJoint, cpConstraintIsPinJoint)
    DEFINE_CONSTRAINT_IS_FN(cpConstraintIsSlideJoint, cpConstraintIsSlideJoint)
    DEFINE_CONSTRAINT_IS_FN(cpConstraintIsPivotJoint, cpConstraintIsPivotJoint)
    DEFINE_CONSTRAINT_IS_FN(cpConstraintIsGrooveJoint, cpConstraintIsGrooveJoint)
    DEFINE_CONSTRAINT_IS_FN(cpConstraintIsDampedSpring, cpConstraintIsDampedSpring)
    DEFINE_CONSTRAINT_IS_FN(cpConstraintIsDampedRotarySpring, cpConstraintIsDampedRotarySpring)
    DEFINE_CONSTRAINT_IS_FN(cpConstraintIsRotaryLimitJoint, cpConstraintIsRotaryLimitJoint)
    DEFINE_CONSTRAINT_IS_FN(cpConstraintIsRatchetJoint, cpConstraintIsRatchetJoint)
    DEFINE_CONSTRAINT_IS_FN(cpConstraintIsGearJoint, cpConstraintIsGearJoint)
    DEFINE_CONSTRAINT_IS_FN(cpConstraintIsSimpleMotor, cpConstraintIsSimpleMotor)

    static int native_cpPinJointNew(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("cpPinJointNew expects (cpBody, cpBody, anchorA, anchorB)");
            return 0;
        }

        cpBody *bodyA = require_body_arg(vm, args[0], "cpPinJointNew", 1);
        cpBody *bodyB = require_body_arg(vm, args[1], "cpPinJointNew", 2);
        cpVect anchorA;
        cpVect anchorB;
        if (!bodyA || !bodyB ||
            !read_cpvect_arg(args[2], &anchorA, "cpPinJointNew", 3) ||
            !read_cpvect_arg(args[3], &anchorB, "cpPinJointNew", 4))
            return 0;

        return push_constraint_handle(vm, cpPinJointNew(bodyA, bodyB, anchorA, anchorB)) ? 1 : 0;
    }

    DEFINE_CONSTRAINT_VECT_GETTER(cpPinJointGetAnchorA, "cpPinJoint", cpConstraintIsPinJoint, cpPinJointGetAnchorA)
    DEFINE_CONSTRAINT_VECT_SETTER(cpPinJointSetAnchorA, "cpPinJoint", cpConstraintIsPinJoint, cpPinJointSetAnchorA)
    DEFINE_CONSTRAINT_VECT_GETTER(cpPinJointGetAnchorB, "cpPinJoint", cpConstraintIsPinJoint, cpPinJointGetAnchorB)
    DEFINE_CONSTRAINT_VECT_SETTER(cpPinJointSetAnchorB, "cpPinJoint", cpConstraintIsPinJoint, cpPinJointSetAnchorB)
    DEFINE_CONSTRAINT_NUMBER_GETTER(cpPinJointGetDist, "cpPinJoint", cpConstraintIsPinJoint, cpPinJointGetDist)
    DEFINE_CONSTRAINT_NUMBER_SETTER(cpPinJointSetDist, "cpPinJoint", cpConstraintIsPinJoint, cpPinJointSetDist, "dist")

    static int native_cpSlideJointNew(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 6)
        {
            Error("cpSlideJointNew expects (cpBody, cpBody, anchorA, anchorB, min, max)");
            return 0;
        }

        cpBody *bodyA = require_body_arg(vm, args[0], "cpSlideJointNew", 1);
        cpBody *bodyB = require_body_arg(vm, args[1], "cpSlideJointNew", 2);
        cpVect anchorA;
        cpVect anchorB;
        double minValue = 0.0;
        double maxValue = 0.0;
        if (!bodyA || !bodyB ||
            !read_cpvect_arg(args[2], &anchorA, "cpSlideJointNew", 3) ||
            !read_cpvect_arg(args[3], &anchorB, "cpSlideJointNew", 4) ||
            !read_number_arg(args[4], &minValue, "cpSlideJointNew", 5) ||
            !read_number_arg(args[5], &maxValue, "cpSlideJointNew", 6))
            return 0;

        return push_constraint_handle(vm, cpSlideJointNew(bodyA, bodyB, anchorA, anchorB, (cpFloat)minValue, (cpFloat)maxValue)) ? 1 : 0;
    }

    DEFINE_CONSTRAINT_VECT_GETTER(cpSlideJointGetAnchorA, "cpSlideJoint", cpConstraintIsSlideJoint, cpSlideJointGetAnchorA)
    DEFINE_CONSTRAINT_VECT_SETTER(cpSlideJointSetAnchorA, "cpSlideJoint", cpConstraintIsSlideJoint, cpSlideJointSetAnchorA)
    DEFINE_CONSTRAINT_VECT_GETTER(cpSlideJointGetAnchorB, "cpSlideJoint", cpConstraintIsSlideJoint, cpSlideJointGetAnchorB)
    DEFINE_CONSTRAINT_VECT_SETTER(cpSlideJointSetAnchorB, "cpSlideJoint", cpConstraintIsSlideJoint, cpSlideJointSetAnchorB)
    DEFINE_CONSTRAINT_NUMBER_GETTER(cpSlideJointGetMin, "cpSlideJoint", cpConstraintIsSlideJoint, cpSlideJointGetMin)
    DEFINE_CONSTRAINT_NUMBER_SETTER(cpSlideJointSetMin, "cpSlideJoint", cpConstraintIsSlideJoint, cpSlideJointSetMin, "min")
    DEFINE_CONSTRAINT_NUMBER_GETTER(cpSlideJointGetMax, "cpSlideJoint", cpConstraintIsSlideJoint, cpSlideJointGetMax)
    DEFINE_CONSTRAINT_NUMBER_SETTER(cpSlideJointSetMax, "cpSlideJoint", cpConstraintIsSlideJoint, cpSlideJointSetMax, "max")

    static int native_cpPivotJointNew(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("cpPivotJointNew expects (cpBody, cpBody, pivot)");
            return 0;
        }

        cpBody *bodyA = require_body_arg(vm, args[0], "cpPivotJointNew", 1);
        cpBody *bodyB = require_body_arg(vm, args[1], "cpPivotJointNew", 2);
        cpVect pivot;
        if (!bodyA || !bodyB || !read_cpvect_arg(args[2], &pivot, "cpPivotJointNew", 3))
            return 0;

        return push_constraint_handle(vm, cpPivotJointNew(bodyA, bodyB, pivot)) ? 1 : 0;
    }

    static int native_cpPivotJointNew2(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("cpPivotJointNew2 expects (cpBody, cpBody, anchorA, anchorB)");
            return 0;
        }

        cpBody *bodyA = require_body_arg(vm, args[0], "cpPivotJointNew2", 1);
        cpBody *bodyB = require_body_arg(vm, args[1], "cpPivotJointNew2", 2);
        cpVect anchorA;
        cpVect anchorB;
        if (!bodyA || !bodyB ||
            !read_cpvect_arg(args[2], &anchorA, "cpPivotJointNew2", 3) ||
            !read_cpvect_arg(args[3], &anchorB, "cpPivotJointNew2", 4))
            return 0;

        return push_constraint_handle(vm, cpPivotJointNew2(bodyA, bodyB, anchorA, anchorB)) ? 1 : 0;
    }

    DEFINE_CONSTRAINT_VECT_GETTER(cpPivotJointGetAnchorA, "cpPivotJoint", cpConstraintIsPivotJoint, cpPivotJointGetAnchorA)
    DEFINE_CONSTRAINT_VECT_SETTER(cpPivotJointSetAnchorA, "cpPivotJoint", cpConstraintIsPivotJoint, cpPivotJointSetAnchorA)
    DEFINE_CONSTRAINT_VECT_GETTER(cpPivotJointGetAnchorB, "cpPivotJoint", cpConstraintIsPivotJoint, cpPivotJointGetAnchorB)
    DEFINE_CONSTRAINT_VECT_SETTER(cpPivotJointSetAnchorB, "cpPivotJoint", cpConstraintIsPivotJoint, cpPivotJointSetAnchorB)

    static int native_cpGrooveJointNew(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5)
        {
            Error("cpGrooveJointNew expects (cpBody, cpBody, grooveA, grooveB, anchorB)");
            return 0;
        }

        cpBody *bodyA = require_body_arg(vm, args[0], "cpGrooveJointNew", 1);
        cpBody *bodyB = require_body_arg(vm, args[1], "cpGrooveJointNew", 2);
        cpVect grooveA;
        cpVect grooveB;
        cpVect anchorB;
        if (!bodyA || !bodyB ||
            !read_cpvect_arg(args[2], &grooveA, "cpGrooveJointNew", 3) ||
            !read_cpvect_arg(args[3], &grooveB, "cpGrooveJointNew", 4) ||
            !read_cpvect_arg(args[4], &anchorB, "cpGrooveJointNew", 5))
            return 0;

        return push_constraint_handle(vm, cpGrooveJointNew(bodyA, bodyB, grooveA, grooveB, anchorB)) ? 1 : 0;
    }

    DEFINE_CONSTRAINT_VECT_GETTER(cpGrooveJointGetGrooveA, "cpGrooveJoint", cpConstraintIsGrooveJoint, cpGrooveJointGetGrooveA)
    DEFINE_CONSTRAINT_VECT_SETTER(cpGrooveJointSetGrooveA, "cpGrooveJoint", cpConstraintIsGrooveJoint, cpGrooveJointSetGrooveA)
    DEFINE_CONSTRAINT_VECT_GETTER(cpGrooveJointGetGrooveB, "cpGrooveJoint", cpConstraintIsGrooveJoint, cpGrooveJointGetGrooveB)
    DEFINE_CONSTRAINT_VECT_SETTER(cpGrooveJointSetGrooveB, "cpGrooveJoint", cpConstraintIsGrooveJoint, cpGrooveJointSetGrooveB)
    DEFINE_CONSTRAINT_VECT_GETTER(cpGrooveJointGetAnchorB, "cpGrooveJoint", cpConstraintIsGrooveJoint, cpGrooveJointGetAnchorB)
    DEFINE_CONSTRAINT_VECT_SETTER(cpGrooveJointSetAnchorB, "cpGrooveJoint", cpConstraintIsGrooveJoint, cpGrooveJointSetAnchorB)

    static int native_cpDampedSpringNew(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 7)
        {
            Error("cpDampedSpringNew expects (cpBody, cpBody, anchorA, anchorB, restLength, stiffness, damping)");
            return 0;
        }

        cpBody *bodyA = require_body_arg(vm, args[0], "cpDampedSpringNew", 1);
        cpBody *bodyB = require_body_arg(vm, args[1], "cpDampedSpringNew", 2);
        cpVect anchorA;
        cpVect anchorB;
        double restLength = 0.0;
        double stiffness = 0.0;
        double damping = 0.0;
        if (!bodyA || !bodyB ||
            !read_cpvect_arg(args[2], &anchorA, "cpDampedSpringNew", 3) ||
            !read_cpvect_arg(args[3], &anchorB, "cpDampedSpringNew", 4) ||
            !read_number_arg(args[4], &restLength, "cpDampedSpringNew", 5) ||
            !read_number_arg(args[5], &stiffness, "cpDampedSpringNew", 6) ||
            !read_number_arg(args[6], &damping, "cpDampedSpringNew", 7))
            return 0;

        return push_constraint_handle(vm, cpDampedSpringNew(bodyA, bodyB, anchorA, anchorB, (cpFloat)restLength, (cpFloat)stiffness, (cpFloat)damping)) ? 1 : 0;
    }

    DEFINE_CONSTRAINT_VECT_GETTER(cpDampedSpringGetAnchorA, "cpDampedSpring", cpConstraintIsDampedSpring, cpDampedSpringGetAnchorA)
    DEFINE_CONSTRAINT_VECT_SETTER(cpDampedSpringSetAnchorA, "cpDampedSpring", cpConstraintIsDampedSpring, cpDampedSpringSetAnchorA)
    DEFINE_CONSTRAINT_VECT_GETTER(cpDampedSpringGetAnchorB, "cpDampedSpring", cpConstraintIsDampedSpring, cpDampedSpringGetAnchorB)
    DEFINE_CONSTRAINT_VECT_SETTER(cpDampedSpringSetAnchorB, "cpDampedSpring", cpConstraintIsDampedSpring, cpDampedSpringSetAnchorB)
    DEFINE_CONSTRAINT_NUMBER_GETTER(cpDampedSpringGetRestLength, "cpDampedSpring", cpConstraintIsDampedSpring, cpDampedSpringGetRestLength)
    DEFINE_CONSTRAINT_NUMBER_SETTER(cpDampedSpringSetRestLength, "cpDampedSpring", cpConstraintIsDampedSpring, cpDampedSpringSetRestLength, "restLength")
    DEFINE_CONSTRAINT_NUMBER_GETTER(cpDampedSpringGetStiffness, "cpDampedSpring", cpConstraintIsDampedSpring, cpDampedSpringGetStiffness)
    DEFINE_CONSTRAINT_NUMBER_SETTER(cpDampedSpringSetStiffness, "cpDampedSpring", cpConstraintIsDampedSpring, cpDampedSpringSetStiffness, "stiffness")
    DEFINE_CONSTRAINT_NUMBER_GETTER(cpDampedSpringGetDamping, "cpDampedSpring", cpConstraintIsDampedSpring, cpDampedSpringGetDamping)
    DEFINE_CONSTRAINT_NUMBER_SETTER(cpDampedSpringSetDamping, "cpDampedSpring", cpConstraintIsDampedSpring, cpDampedSpringSetDamping, "damping")

    static int native_cpDampedRotarySpringNew(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5)
        {
            Error("cpDampedRotarySpringNew expects (cpBody, cpBody, restAngle, stiffness, damping)");
            return 0;
        }

        cpBody *bodyA = require_body_arg(vm, args[0], "cpDampedRotarySpringNew", 1);
        cpBody *bodyB = require_body_arg(vm, args[1], "cpDampedRotarySpringNew", 2);
        double restAngle = 0.0;
        double stiffness = 0.0;
        double damping = 0.0;
        if (!bodyA || !bodyB ||
            !read_number_arg(args[2], &restAngle, "cpDampedRotarySpringNew", 3) ||
            !read_number_arg(args[3], &stiffness, "cpDampedRotarySpringNew", 4) ||
            !read_number_arg(args[4], &damping, "cpDampedRotarySpringNew", 5))
            return 0;

        return push_constraint_handle(vm, cpDampedRotarySpringNew(bodyA, bodyB, (cpFloat)restAngle, (cpFloat)stiffness, (cpFloat)damping)) ? 1 : 0;
    }

    DEFINE_CONSTRAINT_NUMBER_GETTER(cpDampedRotarySpringGetRestAngle, "cpDampedRotarySpring", cpConstraintIsDampedRotarySpring, cpDampedRotarySpringGetRestAngle)
    DEFINE_CONSTRAINT_NUMBER_SETTER(cpDampedRotarySpringSetRestAngle, "cpDampedRotarySpring", cpConstraintIsDampedRotarySpring, cpDampedRotarySpringSetRestAngle, "restAngle")
    DEFINE_CONSTRAINT_NUMBER_GETTER(cpDampedRotarySpringGetStiffness, "cpDampedRotarySpring", cpConstraintIsDampedRotarySpring, cpDampedRotarySpringGetStiffness)
    DEFINE_CONSTRAINT_NUMBER_SETTER(cpDampedRotarySpringSetStiffness, "cpDampedRotarySpring", cpConstraintIsDampedRotarySpring, cpDampedRotarySpringSetStiffness, "stiffness")
    DEFINE_CONSTRAINT_NUMBER_GETTER(cpDampedRotarySpringGetDamping, "cpDampedRotarySpring", cpConstraintIsDampedRotarySpring, cpDampedRotarySpringGetDamping)
    DEFINE_CONSTRAINT_NUMBER_SETTER(cpDampedRotarySpringSetDamping, "cpDampedRotarySpring", cpConstraintIsDampedRotarySpring, cpDampedRotarySpringSetDamping, "damping")

    static int native_cpRotaryLimitJointNew(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("cpRotaryLimitJointNew expects (cpBody, cpBody, min, max)");
            return 0;
        }

        cpBody *bodyA = require_body_arg(vm, args[0], "cpRotaryLimitJointNew", 1);
        cpBody *bodyB = require_body_arg(vm, args[1], "cpRotaryLimitJointNew", 2);
        double minValue = 0.0;
        double maxValue = 0.0;
        if (!bodyA || !bodyB ||
            !read_number_arg(args[2], &minValue, "cpRotaryLimitJointNew", 3) ||
            !read_number_arg(args[3], &maxValue, "cpRotaryLimitJointNew", 4))
            return 0;

        return push_constraint_handle(vm, cpRotaryLimitJointNew(bodyA, bodyB, (cpFloat)minValue, (cpFloat)maxValue)) ? 1 : 0;
    }

    DEFINE_CONSTRAINT_NUMBER_GETTER(cpRotaryLimitJointGetMin, "cpRotaryLimitJoint", cpConstraintIsRotaryLimitJoint, cpRotaryLimitJointGetMin)
    DEFINE_CONSTRAINT_NUMBER_SETTER(cpRotaryLimitJointSetMin, "cpRotaryLimitJoint", cpConstraintIsRotaryLimitJoint, cpRotaryLimitJointSetMin, "min")
    DEFINE_CONSTRAINT_NUMBER_GETTER(cpRotaryLimitJointGetMax, "cpRotaryLimitJoint", cpConstraintIsRotaryLimitJoint, cpRotaryLimitJointGetMax)
    DEFINE_CONSTRAINT_NUMBER_SETTER(cpRotaryLimitJointSetMax, "cpRotaryLimitJoint", cpConstraintIsRotaryLimitJoint, cpRotaryLimitJointSetMax, "max")

    static int native_cpRatchetJointNew(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("cpRatchetJointNew expects (cpBody, cpBody, phase, ratchet)");
            return 0;
        }

        cpBody *bodyA = require_body_arg(vm, args[0], "cpRatchetJointNew", 1);
        cpBody *bodyB = require_body_arg(vm, args[1], "cpRatchetJointNew", 2);
        double phase = 0.0;
        double ratchet = 0.0;
        if (!bodyA || !bodyB ||
            !read_number_arg(args[2], &phase, "cpRatchetJointNew", 3) ||
            !read_number_arg(args[3], &ratchet, "cpRatchetJointNew", 4))
            return 0;

        return push_constraint_handle(vm, cpRatchetJointNew(bodyA, bodyB, (cpFloat)phase, (cpFloat)ratchet)) ? 1 : 0;
    }

    DEFINE_CONSTRAINT_NUMBER_GETTER(cpRatchetJointGetAngle, "cpRatchetJoint", cpConstraintIsRatchetJoint, cpRatchetJointGetAngle)
    DEFINE_CONSTRAINT_NUMBER_SETTER(cpRatchetJointSetAngle, "cpRatchetJoint", cpConstraintIsRatchetJoint, cpRatchetJointSetAngle, "angle")
    DEFINE_CONSTRAINT_NUMBER_GETTER(cpRatchetJointGetPhase, "cpRatchetJoint", cpConstraintIsRatchetJoint, cpRatchetJointGetPhase)
    DEFINE_CONSTRAINT_NUMBER_SETTER(cpRatchetJointSetPhase, "cpRatchetJoint", cpConstraintIsRatchetJoint, cpRatchetJointSetPhase, "phase")
    DEFINE_CONSTRAINT_NUMBER_GETTER(cpRatchetJointGetRatchet, "cpRatchetJoint", cpConstraintIsRatchetJoint, cpRatchetJointGetRatchet)
    DEFINE_CONSTRAINT_NUMBER_SETTER(cpRatchetJointSetRatchet, "cpRatchetJoint", cpConstraintIsRatchetJoint, cpRatchetJointSetRatchet, "ratchet")

    static int native_cpGearJointNew(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("cpGearJointNew expects (cpBody, cpBody, phase, ratio)");
            return 0;
        }

        cpBody *bodyA = require_body_arg(vm, args[0], "cpGearJointNew", 1);
        cpBody *bodyB = require_body_arg(vm, args[1], "cpGearJointNew", 2);
        double phase = 0.0;
        double ratio = 0.0;
        if (!bodyA || !bodyB ||
            !read_number_arg(args[2], &phase, "cpGearJointNew", 3) ||
            !read_number_arg(args[3], &ratio, "cpGearJointNew", 4))
            return 0;

        return push_constraint_handle(vm, cpGearJointNew(bodyA, bodyB, (cpFloat)phase, (cpFloat)ratio)) ? 1 : 0;
    }

    DEFINE_CONSTRAINT_NUMBER_GETTER(cpGearJointGetPhase, "cpGearJoint", cpConstraintIsGearJoint, cpGearJointGetPhase)
    DEFINE_CONSTRAINT_NUMBER_SETTER(cpGearJointSetPhase, "cpGearJoint", cpConstraintIsGearJoint, cpGearJointSetPhase, "phase")
    DEFINE_CONSTRAINT_NUMBER_GETTER(cpGearJointGetRatio, "cpGearJoint", cpConstraintIsGearJoint, cpGearJointGetRatio)
    DEFINE_CONSTRAINT_NUMBER_SETTER(cpGearJointSetRatio, "cpGearJoint", cpConstraintIsGearJoint, cpGearJointSetRatio, "ratio")

    static int native_cpSimpleMotorNew(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("cpSimpleMotorNew expects (cpBody, cpBody, rate)");
            return 0;
        }

        cpBody *bodyA = require_body_arg(vm, args[0], "cpSimpleMotorNew", 1);
        cpBody *bodyB = require_body_arg(vm, args[1], "cpSimpleMotorNew", 2);
        double rate = 0.0;
        if (!bodyA || !bodyB || !read_number_arg(args[2], &rate, "cpSimpleMotorNew", 3))
            return 0;

        return push_constraint_handle(vm, cpSimpleMotorNew(bodyA, bodyB, (cpFloat)rate)) ? 1 : 0;
    }

    DEFINE_CONSTRAINT_NUMBER_GETTER(cpSimpleMotorGetRate, "cpSimpleMotor", cpConstraintIsSimpleMotor, cpSimpleMotorGetRate)
    DEFINE_CONSTRAINT_NUMBER_SETTER(cpSimpleMotorSetRate, "cpSimpleMotor", cpConstraintIsSimpleMotor, cpSimpleMotorSetRate, "rate")

#undef DEFINE_CONSTRAINT_IS_FN
#undef DEFINE_CONSTRAINT_VECT_GETTER
#undef DEFINE_CONSTRAINT_VECT_SETTER
#undef DEFINE_CONSTRAINT_NUMBER_GETTER
#undef DEFINE_CONSTRAINT_NUMBER_SETTER

    void register_constraint(ModuleBuilder &module, Interpreter &vm)
    {
        (void)vm;
        module.addFunction("cpConstraintFree", native_cpConstraintFree, 1)
            .addFunction("cpConstraintGetSpace", native_cpConstraintGetSpace, 1)
            .addFunction("cpConstraintGetBodyA", native_cpConstraintGetBodyA, 1)
            .addFunction("cpConstraintGetBodyB", native_cpConstraintGetBodyB, 1)
            .addFunction("cpConstraintGetMaxForce", native_cpConstraintGetMaxForce, 1)
            .addFunction("cpConstraintSetMaxForce", native_cpConstraintSetMaxForce, 2)
            .addFunction("cpConstraintGetErrorBias", native_cpConstraintGetErrorBias, 1)
            .addFunction("cpConstraintSetErrorBias", native_cpConstraintSetErrorBias, 2)
            .addFunction("cpConstraintGetMaxBias", native_cpConstraintGetMaxBias, 1)
            .addFunction("cpConstraintSetMaxBias", native_cpConstraintSetMaxBias, 2)
            .addFunction("cpConstraintGetCollideBodies", native_cpConstraintGetCollideBodies, 1)
            .addFunction("cpConstraintSetCollideBodies", native_cpConstraintSetCollideBodies, 2)
            .addFunction("cpConstraintGetImpulse", native_cpConstraintGetImpulse, 1)
            .addFunction("cpConstraintIsPinJoint", native_cpConstraintIsPinJoint, 1)
            .addFunction("cpConstraintIsSlideJoint", native_cpConstraintIsSlideJoint, 1)
            .addFunction("cpConstraintIsPivotJoint", native_cpConstraintIsPivotJoint, 1)
            .addFunction("cpConstraintIsGrooveJoint", native_cpConstraintIsGrooveJoint, 1)
            .addFunction("cpConstraintIsDampedSpring", native_cpConstraintIsDampedSpring, 1)
            .addFunction("cpConstraintIsDampedRotarySpring", native_cpConstraintIsDampedRotarySpring, 1)
            .addFunction("cpConstraintIsRotaryLimitJoint", native_cpConstraintIsRotaryLimitJoint, 1)
            .addFunction("cpConstraintIsRatchetJoint", native_cpConstraintIsRatchetJoint, 1)
            .addFunction("cpConstraintIsGearJoint", native_cpConstraintIsGearJoint, 1)
            .addFunction("cpConstraintIsSimpleMotor", native_cpConstraintIsSimpleMotor, 1)
            .addFunction("cpPinJointNew", native_cpPinJointNew, 4)
            .addFunction("cpPinJointGetAnchorA", native_cpPinJointGetAnchorA, 1)
            .addFunction("cpPinJointSetAnchorA", native_cpPinJointSetAnchorA, 2)
            .addFunction("cpPinJointGetAnchorB", native_cpPinJointGetAnchorB, 1)
            .addFunction("cpPinJointSetAnchorB", native_cpPinJointSetAnchorB, 2)
            .addFunction("cpPinJointGetDist", native_cpPinJointGetDist, 1)
            .addFunction("cpPinJointSetDist", native_cpPinJointSetDist, 2)
            .addFunction("cpSlideJointNew", native_cpSlideJointNew, 6)
            .addFunction("cpSlideJointGetAnchorA", native_cpSlideJointGetAnchorA, 1)
            .addFunction("cpSlideJointSetAnchorA", native_cpSlideJointSetAnchorA, 2)
            .addFunction("cpSlideJointGetAnchorB", native_cpSlideJointGetAnchorB, 1)
            .addFunction("cpSlideJointSetAnchorB", native_cpSlideJointSetAnchorB, 2)
            .addFunction("cpSlideJointGetMin", native_cpSlideJointGetMin, 1)
            .addFunction("cpSlideJointSetMin", native_cpSlideJointSetMin, 2)
            .addFunction("cpSlideJointGetMax", native_cpSlideJointGetMax, 1)
            .addFunction("cpSlideJointSetMax", native_cpSlideJointSetMax, 2)
            .addFunction("cpPivotJointNew", native_cpPivotJointNew, 3)
            .addFunction("cpPivotJointNew2", native_cpPivotJointNew2, 4)
            .addFunction("cpPivotJointGetAnchorA", native_cpPivotJointGetAnchorA, 1)
            .addFunction("cpPivotJointSetAnchorA", native_cpPivotJointSetAnchorA, 2)
            .addFunction("cpPivotJointGetAnchorB", native_cpPivotJointGetAnchorB, 1)
            .addFunction("cpPivotJointSetAnchorB", native_cpPivotJointSetAnchorB, 2)
            .addFunction("cpGrooveJointNew", native_cpGrooveJointNew, 5)
            .addFunction("cpGrooveJointGetGrooveA", native_cpGrooveJointGetGrooveA, 1)
            .addFunction("cpGrooveJointSetGrooveA", native_cpGrooveJointSetGrooveA, 2)
            .addFunction("cpGrooveJointGetGrooveB", native_cpGrooveJointGetGrooveB, 1)
            .addFunction("cpGrooveJointSetGrooveB", native_cpGrooveJointSetGrooveB, 2)
            .addFunction("cpGrooveJointGetAnchorB", native_cpGrooveJointGetAnchorB, 1)
            .addFunction("cpGrooveJointSetAnchorB", native_cpGrooveJointSetAnchorB, 2)
            .addFunction("cpDampedSpringNew", native_cpDampedSpringNew, 7)
            .addFunction("cpDampedSpringGetAnchorA", native_cpDampedSpringGetAnchorA, 1)
            .addFunction("cpDampedSpringSetAnchorA", native_cpDampedSpringSetAnchorA, 2)
            .addFunction("cpDampedSpringGetAnchorB", native_cpDampedSpringGetAnchorB, 1)
            .addFunction("cpDampedSpringSetAnchorB", native_cpDampedSpringSetAnchorB, 2)
            .addFunction("cpDampedSpringGetRestLength", native_cpDampedSpringGetRestLength, 1)
            .addFunction("cpDampedSpringSetRestLength", native_cpDampedSpringSetRestLength, 2)
            .addFunction("cpDampedSpringGetStiffness", native_cpDampedSpringGetStiffness, 1)
            .addFunction("cpDampedSpringSetStiffness", native_cpDampedSpringSetStiffness, 2)
            .addFunction("cpDampedSpringGetDamping", native_cpDampedSpringGetDamping, 1)
            .addFunction("cpDampedSpringSetDamping", native_cpDampedSpringSetDamping, 2)
            .addFunction("cpDampedRotarySpringNew", native_cpDampedRotarySpringNew, 5)
            .addFunction("cpDampedRotarySpringGetRestAngle", native_cpDampedRotarySpringGetRestAngle, 1)
            .addFunction("cpDampedRotarySpringSetRestAngle", native_cpDampedRotarySpringSetRestAngle, 2)
            .addFunction("cpDampedRotarySpringGetStiffness", native_cpDampedRotarySpringGetStiffness, 1)
            .addFunction("cpDampedRotarySpringSetStiffness", native_cpDampedRotarySpringSetStiffness, 2)
            .addFunction("cpDampedRotarySpringGetDamping", native_cpDampedRotarySpringGetDamping, 1)
            .addFunction("cpDampedRotarySpringSetDamping", native_cpDampedRotarySpringSetDamping, 2)
            .addFunction("cpRotaryLimitJointNew", native_cpRotaryLimitJointNew, 4)
            .addFunction("cpRotaryLimitJointGetMin", native_cpRotaryLimitJointGetMin, 1)
            .addFunction("cpRotaryLimitJointSetMin", native_cpRotaryLimitJointSetMin, 2)
            .addFunction("cpRotaryLimitJointGetMax", native_cpRotaryLimitJointGetMax, 1)
            .addFunction("cpRotaryLimitJointSetMax", native_cpRotaryLimitJointSetMax, 2)
            .addFunction("cpRatchetJointNew", native_cpRatchetJointNew, 4)
            .addFunction("cpRatchetJointGetAngle", native_cpRatchetJointGetAngle, 1)
            .addFunction("cpRatchetJointSetAngle", native_cpRatchetJointSetAngle, 2)
            .addFunction("cpRatchetJointGetPhase", native_cpRatchetJointGetPhase, 1)
            .addFunction("cpRatchetJointSetPhase", native_cpRatchetJointSetPhase, 2)
            .addFunction("cpRatchetJointGetRatchet", native_cpRatchetJointGetRatchet, 1)
            .addFunction("cpRatchetJointSetRatchet", native_cpRatchetJointSetRatchet, 2)
            .addFunction("cpGearJointNew", native_cpGearJointNew, 4)
            .addFunction("cpGearJointGetPhase", native_cpGearJointGetPhase, 1)
            .addFunction("cpGearJointSetPhase", native_cpGearJointSetPhase, 2)
            .addFunction("cpGearJointGetRatio", native_cpGearJointGetRatio, 1)
            .addFunction("cpGearJointSetRatio", native_cpGearJointSetRatio, 2)
            .addFunction("cpSimpleMotorNew", native_cpSimpleMotorNew, 3)
            .addFunction("cpSimpleMotorGetRate", native_cpSimpleMotorGetRate, 1)
            .addFunction("cpSimpleMotorSetRate", native_cpSimpleMotorSetRate, 2);
    }
}
