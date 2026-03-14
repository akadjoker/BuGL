#include "box2d_joints.hpp"
#include "bindings.hpp"
#include <box2d/box2d.h>
#include <box2d/b2_rope.h>

namespace BOX2DBindings
{
    
    static constexpr const char *kBodyClass = "B2Body";

    static constexpr const char *kMouseJointClass = "MouseJoint";
    static constexpr const char *kMouseJointDefClass = "MouseJointDef";

    static constexpr const char *kRevoluteJointClass = "RevoluteJoint";
    static constexpr const char *kRevoluteJointDefClass = "RevoluteJointDef";

    static constexpr const char *kWheelJointClass = "WheelJoint";
    static constexpr const char *kWheelJointDefClass = "WheelJointDef";

    static constexpr const char *kDistanceJointClass = "DistanceJoint";
    static constexpr const char *kDistanceJointDefClass = "DistanceJointDef";

    static constexpr const char *kPrismaticJointClass = "PrismaticJoint";
    static constexpr const char *kPrismaticJointDefClass = "PrismaticJointDef";

    static constexpr const char *kMotorJointClass = "MotorJoint";
    static constexpr const char *kMotorJointDefClass = "MotorJointDef";

    static constexpr const char *kPulleyJointClass = "PulleyJoint";
    static constexpr const char *kPulleyJointDefClass = "PulleyJointDef";

    static constexpr const char *kFrictionJointClass = "FrictionJoint";
    static constexpr const char *kFrictionJointDefClass = "FrictionJointDef";

    static constexpr const char *kGearJointClass = "GearJoint";
    static constexpr const char *kGearJointDefClass = "GearJointDef";

 
    static constexpr const char *kB2RopeClass = "b2Rope";
    static constexpr const char *kB2RopeDefClass = "b2RopeDef";
    static constexpr const char *kB2RopeTuningClass = "b2RopeTuning";

   
    static bool value_to_bool(const Value &v, bool *out)
    {
        if (!out)
            return false;
        if (v.isBool())
        {
            *out = v.asBool();
            return true;
        }
        if (v.isNumber())
        {
            *out = (v.asNumber() != 0.0);
            return true;
        }
        return false;
    }

    static NativeClassInstance *require_native_instance(Interpreter *vm, const Value &value, const char *className, const char *funcName)
    {
        if (!value.isNativeClassInstance())
        {
            Error("%s expects %s instance", funcName, className);
            return nullptr;
        }

        NativeClassDef *klass = nullptr;
        if (!vm->tryGetNativeClassDef(className, &klass) || !klass)
        {
            Error("%s could not resolve %s class", funcName, className);
            return nullptr;
        }

        NativeClassInstance *instance = value.asNativeClassInstance();
        if (!instance || instance->klass != klass || !instance->userData)
        {
            Error("%s expects valid %s instance", funcName, className);
            return nullptr;
        }
        return instance;
    }

    static b2Body *require_body_arg(Interpreter *vm, const Value &value, const char *funcName)
    {
        NativeClassInstance *instance = require_native_instance(vm, value, kBodyClass, funcName);
        if (!instance)
            return nullptr;
        return static_cast<b2Body *>(instance->userData);
    }

    static b2Joint *require_gear_input_joint_arg(Interpreter *vm, const Value &value, const char *funcName)
    {
        if (!value.isNativeClassInstance())
        {
            Error("%s expects RevoluteJoint or PrismaticJoint", funcName);
            return nullptr;
        }

        NativeClassDef *revoluteKlass = nullptr;
        NativeClassDef *prismaticKlass = nullptr;
        if (!vm->tryGetNativeClassDef(kRevoluteJointClass, &revoluteKlass) || !revoluteKlass ||
            !vm->tryGetNativeClassDef(kPrismaticJointClass, &prismaticKlass) || !prismaticKlass)
        {
            Error("%s could not resolve RevoluteJoint/PrismaticJoint class", funcName);
            return nullptr;
        }

        NativeClassInstance *instance = value.asNativeClassInstance();
        if (!instance || !instance->userData || (instance->klass != revoluteKlass && instance->klass != prismaticKlass))
        {
            Error("%s expects RevoluteJoint or PrismaticJoint", funcName);
            return nullptr;
        }
        return static_cast<b2Joint *>(instance->userData);
    }

    template <typename TJoint>
    static TJoint *as_joint(void *data, const char *funcName, const char *className)
    {
        TJoint *joint = static_cast<TJoint *>(data);
        if (!joint)
        {
            Error("%s invalid %s", funcName, className);
            return nullptr;
        }
        return joint;
    }

    static b2World *resolve_world_from_joint(b2Joint *joint)
    {
        if (!joint)
            return nullptr;
        b2Body *bodyA = joint->GetBodyA();
        if (bodyA && bodyA->GetWorld())
            return bodyA->GetWorld();
        b2Body *bodyB = joint->GetBodyB();
        if (bodyB && bodyB->GetWorld())
            return bodyB->GetWorld();
        return nullptr;
    }

    static b2World *resolve_world_from_bodies(b2Body *bodyA, b2Body *bodyB, const char *funcName)
    {
        if (!bodyA || !bodyB)
        {
            Error("%s requires bodyA and bodyB", funcName);
            return nullptr;
        }

        b2World *worldA = bodyA->GetWorld();
        b2World *worldB = bodyB->GetWorld();
        if (!worldA || !worldB)
        {
            Error("%s requires bodies attached to a world", funcName);
            return nullptr;
        }
        if (worldA != worldB)
        {
            Error("%s requires bodyA and bodyB in same world", funcName);
            return nullptr;
        }
        return worldA;
    }

    static void destroy_joint_now(void *data)
    {
        b2Joint *joint = static_cast<b2Joint *>(data);
        if (!joint)
            return;
        b2World *world = resolve_world_from_joint(joint);
        if (!world || world->IsLocked())
            return;
        world->DestroyJoint(joint);
    }

    static bool joint_data_exists(void *data)
    {
        return resolve_world_from_joint(static_cast<b2Joint *>(data)) != nullptr;
    }

    static b2RopeTuning *as_b2_rope_tuning(void *data, const char *funcName)
    {
        b2RopeTuning *tuning = static_cast<b2RopeTuning *>(data);
        if (!tuning)
        {
            Error("%s invalid b2RopeTuning", funcName);
            return nullptr;
        }
        return tuning;
    }

    static b2RopeDef *as_b2_rope_def(void *data, const char *funcName)
    {
        b2RopeDef *def = static_cast<b2RopeDef *>(data);
        if (!def)
        {
            Error("%s invalid b2RopeDef", funcName);
            return nullptr;
        }
        return def;
    }

    static b2Rope *as_b2_rope(void *data, const char *funcName)
    {
        b2Rope *rope = static_cast<b2Rope *>(data);
        if (!rope)
        {
            Error("%s invalid b2Rope", funcName);
            return nullptr;
        }
        return rope;
    }

    static b2RopeTuning *require_b2_rope_tuning_arg(Interpreter *vm, const Value &value, const char *funcName)
    {
        NativeClassInstance *instance = require_native_instance(vm, value, kB2RopeTuningClass, funcName);
        if (!instance)
            return nullptr;
        return static_cast<b2RopeTuning *>(instance->userData);
    }

    static b2RopeDef *require_b2_rope_def_arg(Interpreter *vm, const Value &value, const char *funcName)
    {
        NativeClassInstance *instance = require_native_instance(vm, value, kB2RopeDefClass, funcName);
        if (!instance)
            return nullptr;
        return static_cast<b2RopeDef *>(instance->userData);
    }

    // -------------------------------------------------------------------------
    // MouseJointDef / MouseJoint
    // -------------------------------------------------------------------------

    void *ctor_native_mouse_joint_def(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("MouseJointDef expects no args");
            return nullptr;
        }
        return new b2MouseJointDef();
    }

    void dtor_native_mouse_joint_def(Interpreter *vm, void *data)
    {
        (void)vm;
        delete static_cast<b2MouseJointDef *>(data);
    }

    int native_mouse_joint_def_set_body_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("set_body_a expects 1 arg (Body)");
            return 0;
        }
        b2Body *body = require_body_arg(vm, args[0], "MouseJointDef.set_body_a");
        if (!body)
            return 0;
        static_cast<b2MouseJointDef *>(data)->bodyA = body;
        return 0;
    }

    int native_mouse_joint_def_set_body_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("set_body_b expects 1 arg (Body)");
            return 0;
        }
        b2Body *body = require_body_arg(vm, args[0], "MouseJointDef.set_body_b");
        if (!body)
            return 0;
        static_cast<b2MouseJointDef *>(data)->bodyB = body;
        return 0;
    }

    int native_mouse_joint_def_initialize(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 4 || !args[2].isNumber() || !args[3].isNumber())
        {
            Error("initialize expects 4 args (bodyA, bodyB, x, y)");
            return 0;
        }
        b2Body *bodyA = require_body_arg(vm, args[0], "MouseJointDef.initialize");
        if (!bodyA)
            return 0;
        b2Body *bodyB = require_body_arg(vm, args[1], "MouseJointDef.initialize");
        if (!bodyB)
            return 0;
        b2MouseJointDef *def = static_cast<b2MouseJointDef *>(data);
        def->bodyA = bodyA;
        def->bodyB = bodyB;
        def->target.Set(((float)args[2].asNumber()), ((float)args[3].asNumber()));
        return 0;
    }

    int native_mouse_joint_def_set_target(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_target expects 2 args (x, y)");
            return 0;
        }
        b2MouseJointDef *def = static_cast<b2MouseJointDef *>(data);
        def->target.Set(((float)args[0].asNumber()), ((float)args[1].asNumber()));
        return 0;
    }

    int native_mouse_joint_def_set_max_force(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_max_force expects 1 arg (force)");
            return 0;
        }
        static_cast<b2MouseJointDef *>(data)->maxForce = (float)args[0].asNumber();
        return 0;
    }

    int native_mouse_joint_def_set_stiffness(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_stiffness expects 1 arg (stiffness)");
            return 0;
        }
        static_cast<b2MouseJointDef *>(data)->stiffness = (float)args[0].asNumber();
        return 0;
    }

    int native_mouse_joint_def_set_damping(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_damping expects 1 arg (damping)");
            return 0;
        }
        static_cast<b2MouseJointDef *>(data)->damping = (float)args[0].asNumber();
        return 0;
    }

    int native_mouse_joint_def_set_collide_connected(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("set_collide_connected expects 1 bool arg");
            return 0;
        }
        static_cast<b2MouseJointDef *>(data)->collideConnected = enabled;
        return 0;
    }

    void *ctor_native_mouse_joint(Interpreter *vm, int argCount, Value *args)
    {
        if (argCount != 1 && argCount != 2)
        {
            Error("MouseJoint expects (MouseJointDef) or (Body, MouseJointDef)");
            return nullptr;
        }

        int defArgIndex = (argCount == 2) ? 1 : 0;
        NativeClassInstance *defInstance = require_native_instance(vm, args[defArgIndex], kMouseJointDefClass, "MouseJoint");
        if (!defInstance)
            return nullptr;

        b2MouseJointDef def = *static_cast<b2MouseJointDef *>(defInstance->userData);
        if (argCount == 2)
        {
            b2Body *body = require_body_arg(vm, args[0], "MouseJoint");
            if (!body)
                return nullptr;
            def.bodyB = body;
        }
        if (!def.bodyA || !def.bodyB)
        {
            Error("MouseJointDef needs bodyA and bodyB");
            return nullptr;
        }

        b2World *world = resolve_world_from_bodies(def.bodyA, def.bodyB, "MouseJoint");
        if (!world)
            return nullptr;
        if (world->IsLocked())
        {
            Error("MouseJoint cannot be created while world is locked");
            return nullptr;
        }

        b2Joint *joint = world->CreateJoint(&def);
        if (!joint)
        {
            Error("MouseJoint failed to create joint");
            return nullptr;
        }

        b2MouseJoint *mouseJoint = static_cast<b2MouseJoint *>(joint);
        if (mouseJoint->GetBodyB())
            mouseJoint->GetBodyB()->SetAwake(true);
        return mouseJoint;
    }

    void dtor_native_mouse_joint(Interpreter *vm, void *data)
    {
        (void)vm;
        (void)data;
    }

    int native_mouse_joint_set_target(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_target expects 2 args (x, y)");
            return 0;
        }
        b2MouseJoint *joint = as_joint<b2MouseJoint>(data, "set_target", kMouseJointClass);
        if (!joint)
            return 0;
        joint->SetTarget(b2Vec2(((float)args[0].asNumber()), ((float)args[1].asNumber())));
        if (joint->GetBodyB())
            joint->GetBodyB()->SetAwake(true);
        return 0;
    }

    int native_mouse_joint_set_max_force(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_max_force expects 1 arg (force)");
            return 0;
        }
        b2MouseJoint *joint = as_joint<b2MouseJoint>(data, "set_max_force", kMouseJointClass);
        if (!joint)
            return 0;
        joint->SetMaxForce((float)args[0].asNumber());
        return 0;
    }

    int native_mouse_joint_set_stiffness(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_stiffness expects 1 arg (stiffness)");
            return 0;
        }
        b2MouseJoint *joint = as_joint<b2MouseJoint>(data, "set_stiffness", kMouseJointClass);
        if (!joint)
            return 0;
        joint->SetStiffness((float)args[0].asNumber());
        return 0;
    }

    int native_mouse_joint_set_damping(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_damping expects 1 arg (damping)");
            return 0;
        }
        b2MouseJoint *joint = as_joint<b2MouseJoint>(data, "set_damping", kMouseJointClass);
        if (!joint)
            return 0;
        joint->SetDamping((float)args[0].asNumber());
        return 0;
    }

    int native_mouse_joint_destroy(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("destroy expects no args");
            return 0;
        }
        destroy_joint_now(data);
        return 0;
    }

    int native_mouse_joint_exists(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("exists expects no args");
            vm->pushBool(false);
            return 1;
        }
        vm->pushBool(joint_data_exists(data));
        return 1;
    }

    int native_mouse_joint_get_target(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_target expects no args");
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2MouseJoint *joint = as_joint<b2MouseJoint>(data, "get_target", kMouseJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2Vec2 t = joint->GetTarget();
        vm->pushDouble((t.x));
        vm->pushDouble((t.y));
        return 2;
    }

    // -------------------------------------------------------------------------
    // RevoluteJointDef / RevoluteJoint
    // -------------------------------------------------------------------------

    void *ctor_native_revolute_joint_def(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("RevoluteJointDef expects no args");
            return nullptr;
        }
        return new b2RevoluteJointDef();
    }

    void dtor_native_revolute_joint_def(Interpreter *vm, void *data)
    {
        (void)vm;
        delete static_cast<b2RevoluteJointDef *>(data);
    }

    int native_revolute_joint_def_set_body_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("set_body_a expects 1 arg (Body)");
            return 0;
        }
        b2Body *body = require_body_arg(vm, args[0], "RevoluteJointDef.set_body_a");
        if (!body)
            return 0;
        static_cast<b2RevoluteJointDef *>(data)->bodyA = body;
        return 0;
    }

    int native_revolute_joint_def_set_body_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("set_body_b expects 1 arg (Body)");
            return 0;
        }
        b2Body *body = require_body_arg(vm, args[0], "RevoluteJointDef.set_body_b");
        if (!body)
            return 0;
        static_cast<b2RevoluteJointDef *>(data)->bodyB = body;
        return 0;
    }

    int native_revolute_joint_def_initialize(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 4 || !args[2].isNumber() || !args[3].isNumber())
        {
            Error("initialize expects 4 args (bodyA, bodyB, x, y)");
            return 0;
        }
        b2Body *bodyA = require_body_arg(vm, args[0], "RevoluteJointDef.initialize");
        if (!bodyA)
            return 0;
        b2Body *bodyB = require_body_arg(vm, args[1], "RevoluteJointDef.initialize");
        if (!bodyB)
            return 0;

        b2RevoluteJointDef *def = static_cast<b2RevoluteJointDef *>(data);
        def->Initialize(bodyA, bodyB, b2Vec2(((float)args[2].asNumber()), ((float)args[3].asNumber())));
        return 0;
    }

    int native_revolute_joint_def_set_local_anchor_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_local_anchor_a expects 2 args (x, y)");
            return 0;
        }
        b2RevoluteJointDef *def = static_cast<b2RevoluteJointDef *>(data);
        def->localAnchorA.Set(((float)args[0].asNumber()), ((float)args[1].asNumber()));
        return 0;
    }

    int native_revolute_joint_def_set_local_anchor_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_local_anchor_b expects 2 args (x, y)");
            return 0;
        }
        b2RevoluteJointDef *def = static_cast<b2RevoluteJointDef *>(data);
        def->localAnchorB.Set(((float)args[0].asNumber()), ((float)args[1].asNumber()));
        return 0;
    }

    int native_revolute_joint_def_set_reference_angle(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_reference_angle expects 1 arg (radians)");
            return 0;
        }
        static_cast<b2RevoluteJointDef *>(data)->referenceAngle = ((float)args[0].asNumber());
        return 0;
    }

    int native_revolute_joint_def_set_enable_limit(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("set_enable_limit expects 1 bool arg");
            return 0;
        }
        static_cast<b2RevoluteJointDef *>(data)->enableLimit = enabled;
        return 0;
    }

    int native_revolute_joint_def_set_limits(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_limits expects 2 args (lowerRadians, upperRadians)");
            return 0;
        }
        b2RevoluteJointDef *def = static_cast<b2RevoluteJointDef *>(data);
        def->lowerAngle = ((float)args[0].asNumber());
        def->upperAngle = ((float)args[1].asNumber());
        return 0;
    }

    int native_revolute_joint_def_set_enable_motor(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("set_enable_motor expects 1 bool arg");
            return 0;
        }
        static_cast<b2RevoluteJointDef *>(data)->enableMotor = enabled;
        return 0;
    }

    int native_revolute_joint_def_set_motor_speed(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_motor_speed expects 1 arg (radians/s)");
            return 0;
        }
        static_cast<b2RevoluteJointDef *>(data)->motorSpeed = ((float)args[0].asNumber());
        return 0;
    }

    int native_revolute_joint_def_set_max_motor_torque(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_max_motor_torque expects 1 arg");
            return 0;
        }
        static_cast<b2RevoluteJointDef *>(data)->maxMotorTorque = (float)args[0].asNumber();
        return 0;
    }

    int native_revolute_joint_def_set_collide_connected(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("set_collide_connected expects 1 bool arg");
            return 0;
        }
        static_cast<b2RevoluteJointDef *>(data)->collideConnected = enabled;
        return 0;
    }

    void *ctor_native_revolute_joint(Interpreter *vm, int argCount, Value *args)
    {
        if (argCount != 1 && argCount != 3)
        {
            Error("RevoluteJoint expects (RevoluteJointDef) or (Body, Body, RevoluteJointDef)");
            return nullptr;
        }

        int defArgIndex = (argCount == 3) ? 2 : 0;
        NativeClassInstance *defInstance = require_native_instance(vm, args[defArgIndex], kRevoluteJointDefClass, "RevoluteJoint");
        if (!defInstance)
            return nullptr;

        b2RevoluteJointDef def = *static_cast<b2RevoluteJointDef *>(defInstance->userData);
        if (argCount == 3)
        {
            b2Body *bodyA = require_body_arg(vm, args[0], "RevoluteJoint");
            if (!bodyA)
                return nullptr;
            b2Body *bodyB = require_body_arg(vm, args[1], "RevoluteJoint");
            if (!bodyB)
                return nullptr;
            def.bodyA = bodyA;
            def.bodyB = bodyB;
        }

        if (!def.bodyA || !def.bodyB)
        {
            Error("RevoluteJointDef needs bodyA and bodyB");
            return nullptr;
        }

        b2World *world = resolve_world_from_bodies(def.bodyA, def.bodyB, "RevoluteJoint");
        if (!world)
            return nullptr;
        if (world->IsLocked())
        {
            Error("RevoluteJoint cannot be created while world is locked");
            return nullptr;
        }

        b2Joint *joint = world->CreateJoint(&def);
        if (!joint)
        {
            Error("RevoluteJoint failed to create joint");
            return nullptr;
        }

        return static_cast<b2RevoluteJoint *>(joint);
    }

    void dtor_native_revolute_joint(Interpreter *vm, void *data)
    {
        (void)vm;
        (void)data;
    }

    int native_revolute_joint_enable_limit(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("enable_limit expects 1 bool arg");
            return 0;
        }
        b2RevoluteJoint *joint = as_joint<b2RevoluteJoint>(data, "enable_limit", kRevoluteJointClass);
        if (!joint)
            return 0;
        joint->EnableLimit(enabled);
        return 0;
    }

    int native_revolute_joint_set_limits(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_limits expects 2 args (lowerRadians, upperRadians)");
            return 0;
        }
        b2RevoluteJoint *joint = as_joint<b2RevoluteJoint>(data, "set_limits", kRevoluteJointClass);
        if (!joint)
            return 0;
        joint->SetLimits(((float)args[0].asNumber()), ((float)args[1].asNumber()));
        return 0;
    }

    int native_revolute_joint_enable_motor(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("enable_motor expects 1 bool arg");
            return 0;
        }
        b2RevoluteJoint *joint = as_joint<b2RevoluteJoint>(data, "enable_motor", kRevoluteJointClass);
        if (!joint)
            return 0;
        joint->EnableMotor(enabled);
        return 0;
    }

    int native_revolute_joint_set_motor_speed(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_motor_speed expects 1 arg (radians/s)");
            return 0;
        }
        b2RevoluteJoint *joint = as_joint<b2RevoluteJoint>(data, "set_motor_speed", kRevoluteJointClass);
        if (!joint)
            return 0;
        joint->SetMotorSpeed(((float)args[0].asNumber()));
        return 0;
    }

    int native_revolute_joint_set_max_motor_torque(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_max_motor_torque expects 1 arg");
            return 0;
        }
        b2RevoluteJoint *joint = as_joint<b2RevoluteJoint>(data, "set_max_motor_torque", kRevoluteJointClass);
        if (!joint)
            return 0;
        joint->SetMaxMotorTorque((float)args[0].asNumber());
        return 0;
    }

    int native_revolute_joint_get_joint_angle(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_joint_angle expects no args");
            vm->pushDouble(0);
            return 1;
        }
        b2RevoluteJoint *joint = as_joint<b2RevoluteJoint>(data, "get_joint_angle", kRevoluteJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble((joint->GetJointAngle()));
        return 1;
    }

    int native_revolute_joint_get_joint_speed(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_joint_speed expects no args");
            vm->pushDouble(0);
            return 1;
        }
        b2RevoluteJoint *joint = as_joint<b2RevoluteJoint>(data, "get_joint_speed", kRevoluteJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble((joint->GetJointSpeed()));
        return 1;
    }

    int native_revolute_joint_get_motor_torque(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("get_motor_torque expects 1 arg (inv_dt)");
            vm->pushDouble(0);
            return 1;
        }
        b2RevoluteJoint *joint = as_joint<b2RevoluteJoint>(data, "get_motor_torque", kRevoluteJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble(joint->GetMotorTorque((float)args[0].asNumber()));
        return 1;
    }

    int native_revolute_joint_get_anchor_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_anchor_a expects no args");
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2RevoluteJoint *joint = as_joint<b2RevoluteJoint>(data, "get_anchor_a", kRevoluteJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2Vec2 a = joint->GetAnchorA();
        vm->pushDouble((a.x));
        vm->pushDouble((a.y));
        return 2;
    }

    int native_revolute_joint_get_anchor_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_anchor_b expects no args");
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2RevoluteJoint *joint = as_joint<b2RevoluteJoint>(data, "get_anchor_b", kRevoluteJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2Vec2 a = joint->GetAnchorB();
        vm->pushDouble((a.x));
        vm->pushDouble((a.y));
        return 2;
    }

    int native_revolute_joint_destroy(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("destroy expects no args");
            return 0;
        }
        destroy_joint_now(data);
        return 0;
    }

    int native_revolute_joint_exists(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("exists expects no args");
            vm->pushBool(false);
            return 1;
        }
        vm->pushBool(joint_data_exists(data));
        return 1;
    }

    // -------------------------------------------------------------------------
    // WheelJointDef / WheelJoint
    // -------------------------------------------------------------------------

    void *ctor_native_wheel_joint_def(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("WheelJointDef expects no args");
            return nullptr;
        }
        return new b2WheelJointDef();
    }

    void dtor_native_wheel_joint_def(Interpreter *vm, void *data)
    {
        (void)vm;
        delete static_cast<b2WheelJointDef *>(data);
    }

    int native_wheel_joint_def_set_body_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("set_body_a expects 1 arg (Body)");
            return 0;
        }
        b2Body *body = require_body_arg(vm, args[0], "WheelJointDef.set_body_a");
        if (!body)
            return 0;
        static_cast<b2WheelJointDef *>(data)->bodyA = body;
        return 0;
    }

    int native_wheel_joint_def_set_body_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("set_body_b expects 1 arg (Body)");
            return 0;
        }
        b2Body *body = require_body_arg(vm, args[0], "WheelJointDef.set_body_b");
        if (!body)
            return 0;
        static_cast<b2WheelJointDef *>(data)->bodyB = body;
        return 0;
    }

    int native_wheel_joint_def_initialize(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 6 || !args[2].isNumber() || !args[3].isNumber() || !args[4].isNumber() || !args[5].isNumber())
        {
            Error("initialize expects 6 args (bodyA, bodyB, anchorX, anchorY, axisX, axisY)");
            return 0;
        }
        b2Body *bodyA = require_body_arg(vm, args[0], "WheelJointDef.initialize");
        if (!bodyA)
            return 0;
        b2Body *bodyB = require_body_arg(vm, args[1], "WheelJointDef.initialize");
        if (!bodyB)
            return 0;

        b2WheelJointDef *def = static_cast<b2WheelJointDef *>(data);
        def->Initialize(
            bodyA,
            bodyB,
            b2Vec2(((float)args[2].asNumber()), ((float)args[3].asNumber())),
            b2Vec2((float)args[4].asNumber(), (float)args[5].asNumber()));
        return 0;
    }

    int native_wheel_joint_def_set_local_anchor_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_local_anchor_a expects 2 args (x, y)");
            return 0;
        }
        b2WheelJointDef *def = static_cast<b2WheelJointDef *>(data);
        def->localAnchorA.Set(((float)args[0].asNumber()), ((float)args[1].asNumber()));
        return 0;
    }

    int native_wheel_joint_def_set_local_anchor_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_local_anchor_b expects 2 args (x, y)");
            return 0;
        }
        b2WheelJointDef *def = static_cast<b2WheelJointDef *>(data);
        def->localAnchorB.Set(((float)args[0].asNumber()), ((float)args[1].asNumber()));
        return 0;
    }

    int native_wheel_joint_def_set_local_axis_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_local_axis_a expects 2 args (x, y)");
            return 0;
        }
        b2WheelJointDef *def = static_cast<b2WheelJointDef *>(data);
        def->localAxisA.Set((float)args[0].asNumber(), (float)args[1].asNumber());
        return 0;
    }

    int native_wheel_joint_def_set_enable_motor(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("set_enable_motor expects 1 bool arg");
            return 0;
        }
        static_cast<b2WheelJointDef *>(data)->enableMotor = enabled;
        return 0;
    }

    int native_wheel_joint_def_set_max_motor_torque(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_max_motor_torque expects 1 arg");
            return 0;
        }
        static_cast<b2WheelJointDef *>(data)->maxMotorTorque = (float)args[0].asNumber();
        return 0;
    }

    int native_wheel_joint_def_set_motor_speed(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_motor_speed expects 1 arg (radians/s)");
            return 0;
        }
        static_cast<b2WheelJointDef *>(data)->motorSpeed = ((float)args[0].asNumber());
        return 0;
    }

    int native_wheel_joint_def_set_stiffness(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_stiffness expects 1 arg");
            return 0;
        }
        static_cast<b2WheelJointDef *>(data)->stiffness = (float)args[0].asNumber();
        return 0;
    }

    int native_wheel_joint_def_set_damping(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_damping expects 1 arg");
            return 0;
        }
        static_cast<b2WheelJointDef *>(data)->damping = (float)args[0].asNumber();
        return 0;
    }

    int native_wheel_joint_def_set_collide_connected(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("set_collide_connected expects 1 bool arg");
            return 0;
        }
        static_cast<b2WheelJointDef *>(data)->collideConnected = enabled;
        return 0;
    }

    void *ctor_native_wheel_joint(Interpreter *vm, int argCount, Value *args)
    {
        if (argCount != 1 && argCount != 3)
        {
            Error("WheelJoint expects (WheelJointDef) or (Body, Body, WheelJointDef)");
            return nullptr;
        }

        int defArgIndex = (argCount == 3) ? 2 : 0;
        NativeClassInstance *defInstance = require_native_instance(vm, args[defArgIndex], kWheelJointDefClass, "WheelJoint");
        if (!defInstance)
            return nullptr;

        b2WheelJointDef def = *static_cast<b2WheelJointDef *>(defInstance->userData);
        if (argCount == 3)
        {
            b2Body *bodyA = require_body_arg(vm, args[0], "WheelJoint");
            if (!bodyA)
                return nullptr;
            b2Body *bodyB = require_body_arg(vm, args[1], "WheelJoint");
            if (!bodyB)
                return nullptr;
            def.bodyA = bodyA;
            def.bodyB = bodyB;
        }

        if (!def.bodyA || !def.bodyB)
        {
            Error("WheelJointDef needs bodyA and bodyB");
            return nullptr;
        }

        b2World *world = resolve_world_from_bodies(def.bodyA, def.bodyB, "WheelJoint");
        if (!world)
            return nullptr;
        if (world->IsLocked())
        {
            Error("WheelJoint cannot be created while world is locked");
            return nullptr;
        }

        b2Joint *joint = world->CreateJoint(&def);
        if (!joint)
        {
            Error("WheelJoint failed to create joint");
            return nullptr;
        }

        return static_cast<b2WheelJoint *>(joint);
    }

    void dtor_native_wheel_joint(Interpreter *vm, void *data)
    {
        (void)vm;
        (void)data;
    }

    int native_wheel_joint_enable_motor(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("enable_motor expects 1 bool arg");
            return 0;
        }
        b2WheelJoint *joint = as_joint<b2WheelJoint>(data, "enable_motor", kWheelJointClass);
        if (!joint)
            return 0;
        joint->EnableMotor(enabled);
        return 0;
    }

    int native_wheel_joint_set_max_motor_torque(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_max_motor_torque expects 1 arg");
            return 0;
        }
        b2WheelJoint *joint = as_joint<b2WheelJoint>(data, "set_max_motor_torque", kWheelJointClass);
        if (!joint)
            return 0;
        joint->SetMaxMotorTorque((float)args[0].asNumber());
        return 0;
    }

    int native_wheel_joint_set_motor_speed(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_motor_speed expects 1 arg (radians/s)");
            return 0;
        }
        b2WheelJoint *joint = as_joint<b2WheelJoint>(data, "set_motor_speed", kWheelJointClass);
        if (!joint)
            return 0;
        joint->SetMotorSpeed(((float)args[0].asNumber()));
        return 0;
    }

    int native_wheel_joint_set_stiffness(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_stiffness expects 1 arg");
            return 0;
        }
        b2WheelJoint *joint = as_joint<b2WheelJoint>(data, "set_stiffness", kWheelJointClass);
        if (!joint)
            return 0;
        joint->SetStiffness((float)args[0].asNumber());
        return 0;
    }

    int native_wheel_joint_set_damping(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_damping expects 1 arg");
            return 0;
        }
        b2WheelJoint *joint = as_joint<b2WheelJoint>(data, "set_damping", kWheelJointClass);
        if (!joint)
            return 0;
        joint->SetDamping((float)args[0].asNumber());
        return 0;
    }

    int native_wheel_joint_get_motor_speed(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_motor_speed expects no args");
            vm->pushDouble(0);
            return 1;
        }
        b2WheelJoint *joint = as_joint<b2WheelJoint>(data, "get_motor_speed", kWheelJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble((joint->GetMotorSpeed()));
        return 1;
    }

    int native_wheel_joint_get_joint_translation(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_joint_translation expects no args");
            vm->pushDouble(0);
            return 1;
        }
        b2WheelJoint *joint = as_joint<b2WheelJoint>(data, "get_joint_translation", kWheelJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble((joint->GetJointTranslation()));
        return 1;
    }

    int native_wheel_joint_get_joint_linear_speed(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_joint_linear_speed expects no args");
            vm->pushDouble(0);
            return 1;
        }
        b2WheelJoint *joint = as_joint<b2WheelJoint>(data, "get_joint_linear_speed", kWheelJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble((joint->GetJointLinearSpeed()));
        return 1;
    }

    int native_wheel_joint_get_motor_torque(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("get_motor_torque expects 1 arg (inv_dt)");
            vm->pushDouble(0);
            return 1;
        }
        b2WheelJoint *joint = as_joint<b2WheelJoint>(data, "get_motor_torque", kWheelJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble(joint->GetMotorTorque((float)args[0].asNumber()));
        return 1;
    }

    int native_wheel_joint_get_anchor_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_anchor_a expects no args");
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2WheelJoint *joint = as_joint<b2WheelJoint>(data, "get_anchor_a", kWheelJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2Vec2 a = joint->GetAnchorA();
        vm->pushDouble((a.x));
        vm->pushDouble((a.y));
        return 2;
    }

    int native_wheel_joint_get_anchor_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_anchor_b expects no args");
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2WheelJoint *joint = as_joint<b2WheelJoint>(data, "get_anchor_b", kWheelJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2Vec2 a = joint->GetAnchorB();
        vm->pushDouble((a.x));
        vm->pushDouble((a.y));
        return 2;
    }

    int native_wheel_joint_destroy(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("destroy expects no args");
            return 0;
        }
        destroy_joint_now(data);
        return 0;
    }

    int native_wheel_joint_exists(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("exists expects no args");
            vm->pushBool(false);
            return 1;
        }
        vm->pushBool(joint_data_exists(data));
        return 1;
    }

    // -------------------------------------------------------------------------
    // DistanceJointDef / DistanceJoint
    // -------------------------------------------------------------------------

    void *ctor_native_distance_joint_def(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("DistanceJointDef expects no args");
            return nullptr;
        }
        return new b2DistanceJointDef();
    }

    void dtor_native_distance_joint_def(Interpreter *vm, void *data)
    {
        (void)vm;
        delete static_cast<b2DistanceJointDef *>(data);
    }

    int native_distance_joint_def_set_body_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("set_body_a expects 1 arg (Body)");
            return 0;
        }
        b2Body *body = require_body_arg(vm, args[0], "DistanceJointDef.set_body_a");
        if (!body)
            return 0;
        static_cast<b2DistanceJointDef *>(data)->bodyA = body;
        return 0;
    }

    int native_distance_joint_def_set_body_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("set_body_b expects 1 arg (Body)");
            return 0;
        }
        b2Body *body = require_body_arg(vm, args[0], "DistanceJointDef.set_body_b");
        if (!body)
            return 0;
        static_cast<b2DistanceJointDef *>(data)->bodyB = body;
        return 0;
    }

    int native_distance_joint_def_initialize(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 6 || !args[2].isNumber() || !args[3].isNumber() || !args[4].isNumber() || !args[5].isNumber())
        {
            Error("initialize expects 6 args (bodyA, bodyB, ax, ay, bx, by)");
            return 0;
        }
        b2Body *bodyA = require_body_arg(vm, args[0], "DistanceJointDef.initialize");
        if (!bodyA)
            return 0;
        b2Body *bodyB = require_body_arg(vm, args[1], "DistanceJointDef.initialize");
        if (!bodyB)
            return 0;

        b2DistanceJointDef *def = static_cast<b2DistanceJointDef *>(data);
        def->Initialize(
            bodyA,
            bodyB,
            b2Vec2(((float)args[2].asNumber()), ((float)args[3].asNumber())),
            b2Vec2(((float)args[4].asNumber()), ((float)args[5].asNumber())));
        return 0;
    }

    int native_distance_joint_def_set_local_anchor_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_local_anchor_a expects 2 args (x, y)");
            return 0;
        }
        b2DistanceJointDef *def = static_cast<b2DistanceJointDef *>(data);
        def->localAnchorA.Set(((float)args[0].asNumber()), ((float)args[1].asNumber()));
        return 0;
    }

    int native_distance_joint_def_set_local_anchor_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_local_anchor_b expects 2 args (x, y)");
            return 0;
        }
        b2DistanceJointDef *def = static_cast<b2DistanceJointDef *>(data);
        def->localAnchorB.Set(((float)args[0].asNumber()), ((float)args[1].asNumber()));
        return 0;
    }

    int native_distance_joint_def_set_length(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_length expects 1 arg (meters)");
            return 0;
        }
        static_cast<b2DistanceJointDef *>(data)->length = ((float)args[0].asNumber());
        return 0;
    }

    int native_distance_joint_def_set_min_length(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_min_length expects 1 arg (meters)");
            return 0;
        }
        static_cast<b2DistanceJointDef *>(data)->minLength = ((float)args[0].asNumber());
        return 0;
    }

    int native_distance_joint_def_set_max_length(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_max_length expects 1 arg (meters)");
            return 0;
        }
        static_cast<b2DistanceJointDef *>(data)->maxLength = ((float)args[0].asNumber());
        return 0;
    }

    int native_distance_joint_def_set_stiffness(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_stiffness expects 1 arg");
            return 0;
        }
        static_cast<b2DistanceJointDef *>(data)->stiffness = (float)args[0].asNumber();
        return 0;
    }

    int native_distance_joint_def_set_damping(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_damping expects 1 arg");
            return 0;
        }
        static_cast<b2DistanceJointDef *>(data)->damping = (float)args[0].asNumber();
        return 0;
    }

    int native_distance_joint_def_set_collide_connected(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("set_collide_connected expects 1 bool arg");
            return 0;
        }
        static_cast<b2DistanceJointDef *>(data)->collideConnected = enabled;
        return 0;
    }

    void *ctor_native_distance_joint(Interpreter *vm, int argCount, Value *args)
    {
        if (argCount != 1 && argCount != 3)
        {
            Error("DistanceJoint expects (DistanceJointDef) or (Body, Body, DistanceJointDef)");
            return nullptr;
        }

        int defArgIndex = (argCount == 3) ? 2 : 0;
        NativeClassInstance *defInstance = require_native_instance(vm, args[defArgIndex], kDistanceJointDefClass, "DistanceJoint");
        if (!defInstance)
            return nullptr;

        b2DistanceJointDef def = *static_cast<b2DistanceJointDef *>(defInstance->userData);
        if (argCount == 3)
        {
            b2Body *bodyA = require_body_arg(vm, args[0], "DistanceJoint");
            if (!bodyA)
                return nullptr;
            b2Body *bodyB = require_body_arg(vm, args[1], "DistanceJoint");
            if (!bodyB)
                return nullptr;
            def.bodyA = bodyA;
            def.bodyB = bodyB;
        }

        if (!def.bodyA || !def.bodyB)
        {
            Error("DistanceJointDef needs bodyA and bodyB");
            return nullptr;
        }

        b2World *world = resolve_world_from_bodies(def.bodyA, def.bodyB, "DistanceJoint");
        if (!world)
            return nullptr;
        if (world->IsLocked())
        {
            Error("DistanceJoint cannot be created while world is locked");
            return nullptr;
        }

        b2Joint *joint = world->CreateJoint(&def);
        if (!joint)
        {
            Error("DistanceJoint failed to create joint");
            return nullptr;
        }
        return static_cast<b2DistanceJoint *>(joint);
    }

    void dtor_native_distance_joint(Interpreter *vm, void *data)
    {
        (void)vm;
        (void)data;
    }

    int native_distance_joint_set_length(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_length expects 1 arg (meters)");
            return 0;
        }
        b2DistanceJoint *joint = as_joint<b2DistanceJoint>(data, "set_length", kDistanceJointClass);
        if (!joint)
            return 0;
        joint->SetLength(((float)args[0].asNumber()));
        return 0;
    }

    int native_distance_joint_set_min_length(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_min_length expects 1 arg (meters)");
            return 0;
        }
        b2DistanceJoint *joint = as_joint<b2DistanceJoint>(data, "set_min_length", kDistanceJointClass);
        if (!joint)
            return 0;
        joint->SetMinLength(((float)args[0].asNumber()));
        return 0;
    }

    int native_distance_joint_set_max_length(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_max_length expects 1 arg (meters)");
            return 0;
        }
        b2DistanceJoint *joint = as_joint<b2DistanceJoint>(data, "set_max_length", kDistanceJointClass);
        if (!joint)
            return 0;
        joint->SetMaxLength(((float)args[0].asNumber()));
        return 0;
    }

    int native_distance_joint_set_stiffness(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_stiffness expects 1 arg");
            return 0;
        }
        b2DistanceJoint *joint = as_joint<b2DistanceJoint>(data, "set_stiffness", kDistanceJointClass);
        if (!joint)
            return 0;
        joint->SetStiffness((float)args[0].asNumber());
        return 0;
    }

    int native_distance_joint_set_damping(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_damping expects 1 arg");
            return 0;
        }
        b2DistanceJoint *joint = as_joint<b2DistanceJoint>(data, "set_damping", kDistanceJointClass);
        if (!joint)
            return 0;
        joint->SetDamping((float)args[0].asNumber());
        return 0;
    }

    int native_distance_joint_get_length(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_length expects no args");
            vm->pushDouble(0);
            return 1;
        }
        b2DistanceJoint *joint = as_joint<b2DistanceJoint>(data, "get_length", kDistanceJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble((joint->GetLength()));
        return 1;
    }

    int native_distance_joint_get_current_length(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_current_length expects no args");
            vm->pushDouble(0);
            return 1;
        }
        b2DistanceJoint *joint = as_joint<b2DistanceJoint>(data, "get_current_length", kDistanceJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble((joint->GetCurrentLength()));
        return 1;
    }

    int native_distance_joint_get_anchor_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_anchor_a expects no args");
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2DistanceJoint *joint = as_joint<b2DistanceJoint>(data, "get_anchor_a", kDistanceJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2Vec2 a = joint->GetAnchorA();
        vm->pushDouble((a.x));
        vm->pushDouble((a.y));
        return 2;
    }

    int native_distance_joint_get_anchor_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_anchor_b expects no args");
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2DistanceJoint *joint = as_joint<b2DistanceJoint>(data, "get_anchor_b", kDistanceJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2Vec2 a = joint->GetAnchorB();
        vm->pushDouble((a.x));
        vm->pushDouble((a.y));
        return 2;
    }

    int native_distance_joint_destroy(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("destroy expects no args");
            return 0;
        }
        destroy_joint_now(data);
        return 0;
    }

    int native_distance_joint_exists(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("exists expects no args");
            vm->pushBool(false);
            return 1;
        }
        vm->pushBool(joint_data_exists(data));
        return 1;
    }

    // -------------------------------------------------------------------------
    // PrismaticJointDef / PrismaticJoint
    // -------------------------------------------------------------------------

    void *ctor_native_prismatic_joint_def(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("PrismaticJointDef expects no args");
            return nullptr;
        }
        return new b2PrismaticJointDef();
    }

    void dtor_native_prismatic_joint_def(Interpreter *vm, void *data)
    {
        (void)vm;
        delete static_cast<b2PrismaticJointDef *>(data);
    }

    int native_prismatic_joint_def_set_body_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("set_body_a expects 1 arg (Body)");
            return 0;
        }
        b2Body *body = require_body_arg(vm, args[0], "PrismaticJointDef.set_body_a");
        if (!body)
            return 0;
        static_cast<b2PrismaticJointDef *>(data)->bodyA = body;
        return 0;
    }

    int native_prismatic_joint_def_set_body_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("set_body_b expects 1 arg (Body)");
            return 0;
        }
        b2Body *body = require_body_arg(vm, args[0], "PrismaticJointDef.set_body_b");
        if (!body)
            return 0;
        static_cast<b2PrismaticJointDef *>(data)->bodyB = body;
        return 0;
    }

    int native_prismatic_joint_def_initialize(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 6 || !args[2].isNumber() || !args[3].isNumber() || !args[4].isNumber() || !args[5].isNumber())
        {
            Error("initialize expects 6 args (bodyA, bodyB, anchorX, anchorY, axisX, axisY)");
            return 0;
        }
        b2Body *bodyA = require_body_arg(vm, args[0], "PrismaticJointDef.initialize");
        if (!bodyA)
            return 0;
        b2Body *bodyB = require_body_arg(vm, args[1], "PrismaticJointDef.initialize");
        if (!bodyB)
            return 0;

        b2PrismaticJointDef *def = static_cast<b2PrismaticJointDef *>(data);
        def->Initialize(
            bodyA,
            bodyB,
            b2Vec2(((float)args[2].asNumber()), ((float)args[3].asNumber())),
            b2Vec2((float)args[4].asNumber(), (float)args[5].asNumber()));
        return 0;
    }

    int native_prismatic_joint_def_set_local_anchor_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_local_anchor_a expects 2 args (x, y)");
            return 0;
        }
        b2PrismaticJointDef *def = static_cast<b2PrismaticJointDef *>(data);
        def->localAnchorA.Set(((float)args[0].asNumber()), ((float)args[1].asNumber()));
        return 0;
    }

    int native_prismatic_joint_def_set_local_anchor_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_local_anchor_b expects 2 args (x, y)");
            return 0;
        }
        b2PrismaticJointDef *def = static_cast<b2PrismaticJointDef *>(data);
        def->localAnchorB.Set(((float)args[0].asNumber()), ((float)args[1].asNumber()));
        return 0;
    }

    int native_prismatic_joint_def_set_local_axis_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_local_axis_a expects 2 args (x, y)");
            return 0;
        }
        b2PrismaticJointDef *def = static_cast<b2PrismaticJointDef *>(data);
        def->localAxisA.Set((float)args[0].asNumber(), (float)args[1].asNumber());
        return 0;
    }

    int native_prismatic_joint_def_set_reference_angle(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_reference_angle expects 1 arg (radians)");
            return 0;
        }
        static_cast<b2PrismaticJointDef *>(data)->referenceAngle = ((float)args[0].asNumber());
        return 0;
    }

    int native_prismatic_joint_def_set_enable_limit(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("set_enable_limit expects 1 bool arg");
            return 0;
        }
        static_cast<b2PrismaticJointDef *>(data)->enableLimit = enabled;
        return 0;
    }

    int native_prismatic_joint_def_set_limits(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_limits expects 2 args (lowerPixels, upperPixels)");
            return 0;
        }
        b2PrismaticJointDef *def = static_cast<b2PrismaticJointDef *>(data);
        def->lowerTranslation = ((float)args[0].asNumber());
        def->upperTranslation = ((float)args[1].asNumber());
        return 0;
    }

    int native_prismatic_joint_def_set_enable_motor(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("set_enable_motor expects 1 bool arg");
            return 0;
        }
        static_cast<b2PrismaticJointDef *>(data)->enableMotor = enabled;
        return 0;
    }

    int native_prismatic_joint_def_set_motor_speed(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_motor_speed expects 1 arg (meters/s)");
            return 0;
        }
        static_cast<b2PrismaticJointDef *>(data)->motorSpeed = ((float)args[0].asNumber());
        return 0;
    }

    int native_prismatic_joint_def_set_max_motor_force(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_max_motor_force expects 1 arg");
            return 0;
        }
        static_cast<b2PrismaticJointDef *>(data)->maxMotorForce = (float)args[0].asNumber();
        return 0;
    }

    int native_prismatic_joint_def_set_collide_connected(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("set_collide_connected expects 1 bool arg");
            return 0;
        }
        static_cast<b2PrismaticJointDef *>(data)->collideConnected = enabled;
        return 0;
    }

    void *ctor_native_prismatic_joint(Interpreter *vm, int argCount, Value *args)
    {
        if (argCount != 1 && argCount != 3)
        {
            Error("PrismaticJoint expects (PrismaticJointDef) or (Body, Body, PrismaticJointDef)");
            return nullptr;
        }

        int defArgIndex = (argCount == 3) ? 2 : 0;
        NativeClassInstance *defInstance = require_native_instance(vm, args[defArgIndex], kPrismaticJointDefClass, "PrismaticJoint");
        if (!defInstance)
            return nullptr;

        b2PrismaticJointDef def = *static_cast<b2PrismaticJointDef *>(defInstance->userData);
        if (argCount == 3)
        {
            b2Body *bodyA = require_body_arg(vm, args[0], "PrismaticJoint");
            if (!bodyA)
                return nullptr;
            b2Body *bodyB = require_body_arg(vm, args[1], "PrismaticJoint");
            if (!bodyB)
                return nullptr;
            def.bodyA = bodyA;
            def.bodyB = bodyB;
        }

        if (!def.bodyA || !def.bodyB)
        {
            Error("PrismaticJointDef needs bodyA and bodyB");
            return nullptr;
        }

        b2World *world = resolve_world_from_bodies(def.bodyA, def.bodyB, "PrismaticJoint");
        if (!world)
            return nullptr;
        if (world->IsLocked())
        {
            Error("PrismaticJoint cannot be created while world is locked");
            return nullptr;
        }

        b2Joint *joint = world->CreateJoint(&def);
        if (!joint)
        {
            Error("PrismaticJoint failed to create joint");
            return nullptr;
        }
        return static_cast<b2PrismaticJoint *>(joint);
    }

    void dtor_native_prismatic_joint(Interpreter *vm, void *data)
    {
        (void)vm;
        (void)data;
    }

    int native_prismatic_joint_enable_limit(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("enable_limit expects 1 bool arg");
            return 0;
        }
        b2PrismaticJoint *joint = as_joint<b2PrismaticJoint>(data, "enable_limit", kPrismaticJointClass);
        if (!joint)
            return 0;
        joint->EnableLimit(enabled);
        return 0;
    }

    int native_prismatic_joint_set_limits(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_limits expects 2 args (lowerPixels, upperPixels)");
            return 0;
        }
        b2PrismaticJoint *joint = as_joint<b2PrismaticJoint>(data, "set_limits", kPrismaticJointClass);
        if (!joint)
            return 0;
        joint->SetLimits(((float)args[0].asNumber()), ((float)args[1].asNumber()));
        return 0;
    }

    int native_prismatic_joint_enable_motor(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("enable_motor expects 1 bool arg");
            return 0;
        }
        b2PrismaticJoint *joint = as_joint<b2PrismaticJoint>(data, "enable_motor", kPrismaticJointClass);
        if (!joint)
            return 0;
        joint->EnableMotor(enabled);
        return 0;
    }

    int native_prismatic_joint_set_motor_speed(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_motor_speed expects 1 arg (meters/s)");
            return 0;
        }
        b2PrismaticJoint *joint = as_joint<b2PrismaticJoint>(data, "set_motor_speed", kPrismaticJointClass);
        if (!joint)
            return 0;
        joint->SetMotorSpeed(((float)args[0].asNumber()));
        return 0;
    }

    int native_prismatic_joint_set_max_motor_force(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_max_motor_force expects 1 arg");
            return 0;
        }
        b2PrismaticJoint *joint = as_joint<b2PrismaticJoint>(data, "set_max_motor_force", kPrismaticJointClass);
        if (!joint)
            return 0;
        joint->SetMaxMotorForce((float)args[0].asNumber());
        return 0;
    }

    int native_prismatic_joint_get_joint_translation(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_joint_translation expects no args");
            vm->pushDouble(0);
            return 1;
        }
        b2PrismaticJoint *joint = as_joint<b2PrismaticJoint>(data, "get_joint_translation", kPrismaticJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble((joint->GetJointTranslation()));
        return 1;
    }

    int native_prismatic_joint_get_joint_speed(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_joint_speed expects no args");
            vm->pushDouble(0);
            return 1;
        }
        b2PrismaticJoint *joint = as_joint<b2PrismaticJoint>(data, "get_joint_speed", kPrismaticJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble((joint->GetJointSpeed()));
        return 1;
    }

    int native_prismatic_joint_get_motor_force(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("get_motor_force expects 1 arg (inv_dt)");
            vm->pushDouble(0);
            return 1;
        }
        b2PrismaticJoint *joint = as_joint<b2PrismaticJoint>(data, "get_motor_force", kPrismaticJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble(joint->GetMotorForce((float)args[0].asNumber()));
        return 1;
    }

    int native_prismatic_joint_get_anchor_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_anchor_a expects no args");
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2PrismaticJoint *joint = as_joint<b2PrismaticJoint>(data, "get_anchor_a", kPrismaticJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2Vec2 a = joint->GetAnchorA();
        vm->pushDouble((a.x));
        vm->pushDouble((a.y));
        return 2;
    }

    int native_prismatic_joint_get_anchor_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_anchor_b expects no args");
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2PrismaticJoint *joint = as_joint<b2PrismaticJoint>(data, "get_anchor_b", kPrismaticJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2Vec2 a = joint->GetAnchorB();
        vm->pushDouble((a.x));
        vm->pushDouble((a.y));
        return 2;
    }

    int native_prismatic_joint_destroy(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("destroy expects no args");
            return 0;
        }
        destroy_joint_now(data);
        return 0;
    }

    int native_prismatic_joint_exists(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("exists expects no args");
            vm->pushBool(false);
            return 1;
        }
        vm->pushBool(joint_data_exists(data));
        return 1;
    }

    // -------------------------------------------------------------------------
    // PulleyJointDef / PulleyJoint
    // -------------------------------------------------------------------------

    void *ctor_native_pulley_joint_def(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("PulleyJointDef expects no args");
            return nullptr;
        }
        return new b2PulleyJointDef();
    }

    void dtor_native_pulley_joint_def(Interpreter *vm, void *data)
    {
        (void)vm;
        delete static_cast<b2PulleyJointDef *>(data);
    }

    int native_pulley_joint_def_set_body_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("set_body_a expects 1 arg (Body)");
            return 0;
        }
        b2Body *body = require_body_arg(vm, args[0], "PulleyJointDef.set_body_a");
        if (!body)
            return 0;
        static_cast<b2PulleyJointDef *>(data)->bodyA = body;
        return 0;
    }

    int native_pulley_joint_def_set_body_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("set_body_b expects 1 arg (Body)");
            return 0;
        }
        b2Body *body = require_body_arg(vm, args[0], "PulleyJointDef.set_body_b");
        if (!body)
            return 0;
        static_cast<b2PulleyJointDef *>(data)->bodyB = body;
        return 0;
    }

    int native_pulley_joint_def_initialize(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 11)
        {
            Error("initialize expects 11 args (bodyA, bodyB, gax, gay, gbx, gby, ax, ay, bx, by, ratio)");
            return 0;
        }
        for (int i = 2; i <= 10; ++i)
        {
            if (!args[i].isNumber())
            {
                Error("initialize expects numeric anchor/ratio args");
                return 0;
            }
        }
        b2Body *bodyA = require_body_arg(vm, args[0], "PulleyJointDef.initialize");
        if (!bodyA)
            return 0;
        b2Body *bodyB = require_body_arg(vm, args[1], "PulleyJointDef.initialize");
        if (!bodyB)
            return 0;
        const float ratio = (float)args[10].asNumber();
        if (ratio <= 0.0f)
        {
            Error("initialize ratio must be > 0");
            return 0;
        }

        b2PulleyJointDef *def = static_cast<b2PulleyJointDef *>(data);
        def->Initialize(
            bodyA,
            bodyB,
            b2Vec2(((float)args[2].asNumber()), ((float)args[3].asNumber())),
            b2Vec2(((float)args[4].asNumber()), ((float)args[5].asNumber())),
            b2Vec2(((float)args[6].asNumber()), ((float)args[7].asNumber())),
            b2Vec2(((float)args[8].asNumber()), ((float)args[9].asNumber())),
            ratio);
        return 0;
    }

    int native_pulley_joint_def_set_ground_anchor_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_ground_anchor_a expects 2 args (x, y)");
            return 0;
        }
        b2PulleyJointDef *def = static_cast<b2PulleyJointDef *>(data);
        def->groundAnchorA.Set(((float)args[0].asNumber()), ((float)args[1].asNumber()));
        return 0;
    }

    int native_pulley_joint_def_set_ground_anchor_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_ground_anchor_b expects 2 args (x, y)");
            return 0;
        }
        b2PulleyJointDef *def = static_cast<b2PulleyJointDef *>(data);
        def->groundAnchorB.Set(((float)args[0].asNumber()), ((float)args[1].asNumber()));
        return 0;
    }

    int native_pulley_joint_def_set_local_anchor_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_local_anchor_a expects 2 args (x, y)");
            return 0;
        }
        b2PulleyJointDef *def = static_cast<b2PulleyJointDef *>(data);
        def->localAnchorA.Set(((float)args[0].asNumber()), ((float)args[1].asNumber()));
        return 0;
    }

    int native_pulley_joint_def_set_local_anchor_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_local_anchor_b expects 2 args (x, y)");
            return 0;
        }
        b2PulleyJointDef *def = static_cast<b2PulleyJointDef *>(data);
        def->localAnchorB.Set(((float)args[0].asNumber()), ((float)args[1].asNumber()));
        return 0;
    }

    int native_pulley_joint_def_set_length_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_length_a expects 1 arg (meters)");
            return 0;
        }
        static_cast<b2PulleyJointDef *>(data)->lengthA = ((float)args[0].asNumber());
        return 0;
    }

    int native_pulley_joint_def_set_length_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_length_b expects 1 arg (meters)");
            return 0;
        }
        static_cast<b2PulleyJointDef *>(data)->lengthB = ((float)args[0].asNumber());
        return 0;
    }

    int native_pulley_joint_def_set_ratio(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_ratio expects 1 arg");
            return 0;
        }
        const float ratio = (float)args[0].asNumber();
        if (ratio <= 0.0f)
        {
            Error("set_ratio expects value > 0");
            return 0;
        }
        static_cast<b2PulleyJointDef *>(data)->ratio = ratio;
        return 0;
    }

    int native_pulley_joint_def_set_collide_connected(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("set_collide_connected expects 1 bool arg");
            return 0;
        }
        static_cast<b2PulleyJointDef *>(data)->collideConnected = enabled;
        return 0;
    }

    void *ctor_native_pulley_joint(Interpreter *vm, int argCount, Value *args)
    {
        if (argCount != 1 && argCount != 3)
        {
            Error("PulleyJoint expects (PulleyJointDef) or (Body, Body, PulleyJointDef)");
            return nullptr;
        }

        int defArgIndex = (argCount == 3) ? 2 : 0;
        NativeClassInstance *defInstance = require_native_instance(vm, args[defArgIndex], kPulleyJointDefClass, "PulleyJoint");
        if (!defInstance)
            return nullptr;

        b2PulleyJointDef def = *static_cast<b2PulleyJointDef *>(defInstance->userData);
        if (argCount == 3)
        {
            b2Body *bodyA = require_body_arg(vm, args[0], "PulleyJoint");
            if (!bodyA)
                return nullptr;
            b2Body *bodyB = require_body_arg(vm, args[1], "PulleyJoint");
            if (!bodyB)
                return nullptr;
            def.bodyA = bodyA;
            def.bodyB = bodyB;
        }
        if (!def.bodyA || !def.bodyB)
        {
            Error("PulleyJointDef needs bodyA and bodyB");
            return nullptr;
        }
        if (def.ratio <= 0.0f)
        {
            Error("PulleyJointDef ratio must be > 0");
            return nullptr;
        }

        b2World *world = resolve_world_from_bodies(def.bodyA, def.bodyB, "PulleyJoint");
        if (!world)
            return nullptr;
        if (world->IsLocked())
        {
            Error("PulleyJoint cannot be created while world is locked");
            return nullptr;
        }

        b2Joint *joint = world->CreateJoint(&def);
        if (!joint)
        {
            Error("PulleyJoint failed to create joint");
            return nullptr;
        }
        return static_cast<b2PulleyJoint *>(joint);
    }

    void dtor_native_pulley_joint(Interpreter *vm, void *data)
    {
        (void)vm;
        (void)data;
    }

    int native_pulley_joint_get_ratio(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_ratio expects no args");
            vm->pushDouble(0);
            return 1;
        }
        b2PulleyJoint *joint = as_joint<b2PulleyJoint>(data, "get_ratio", kPulleyJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble(joint->GetRatio());
        return 1;
    }

    int native_pulley_joint_get_length_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_length_a expects no args");
            vm->pushDouble(0);
            return 1;
        }
        b2PulleyJoint *joint = as_joint<b2PulleyJoint>(data, "get_length_a", kPulleyJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble((joint->GetLengthA()));
        return 1;
    }

    int native_pulley_joint_get_length_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_length_b expects no args");
            vm->pushDouble(0);
            return 1;
        }
        b2PulleyJoint *joint = as_joint<b2PulleyJoint>(data, "get_length_b", kPulleyJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble((joint->GetLengthB()));
        return 1;
    }

    int native_pulley_joint_get_current_length_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_current_length_a expects no args");
            vm->pushDouble(0);
            return 1;
        }
        b2PulleyJoint *joint = as_joint<b2PulleyJoint>(data, "get_current_length_a", kPulleyJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble((joint->GetCurrentLengthA()));
        return 1;
    }

    int native_pulley_joint_get_current_length_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_current_length_b expects no args");
            vm->pushDouble(0);
            return 1;
        }
        b2PulleyJoint *joint = as_joint<b2PulleyJoint>(data, "get_current_length_b", kPulleyJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble((joint->GetCurrentLengthB()));
        return 1;
    }

    int native_pulley_joint_get_anchor_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_anchor_a expects no args");
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2PulleyJoint *joint = as_joint<b2PulleyJoint>(data, "get_anchor_a", kPulleyJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2Vec2 a = joint->GetAnchorA();
        vm->pushDouble((a.x));
        vm->pushDouble((a.y));
        return 2;
    }

    int native_pulley_joint_get_anchor_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_anchor_b expects no args");
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2PulleyJoint *joint = as_joint<b2PulleyJoint>(data, "get_anchor_b", kPulleyJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2Vec2 a = joint->GetAnchorB();
        vm->pushDouble((a.x));
        vm->pushDouble((a.y));
        return 2;
    }

    int native_pulley_joint_get_ground_anchor_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_ground_anchor_a expects no args");
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2PulleyJoint *joint = as_joint<b2PulleyJoint>(data, "get_ground_anchor_a", kPulleyJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2Vec2 a = joint->GetGroundAnchorA();
        vm->pushDouble((a.x));
        vm->pushDouble((a.y));
        return 2;
    }

    int native_pulley_joint_get_ground_anchor_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_ground_anchor_b expects no args");
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2PulleyJoint *joint = as_joint<b2PulleyJoint>(data, "get_ground_anchor_b", kPulleyJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2Vec2 a = joint->GetGroundAnchorB();
        vm->pushDouble((a.x));
        vm->pushDouble((a.y));
        return 2;
    }

    int native_pulley_joint_destroy(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("destroy expects no args");
            return 0;
        }
        destroy_joint_now(data);
        return 0;
    }

    int native_pulley_joint_exists(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("exists expects no args");
            vm->pushBool(false);
            return 1;
        }
        vm->pushBool(joint_data_exists(data));
        return 1;
    }

    // -------------------------------------------------------------------------
    // FrictionJointDef / FrictionJoint
    // -------------------------------------------------------------------------

    void *ctor_native_friction_joint_def(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("FrictionJointDef expects no args");
            return nullptr;
        }
        return new b2FrictionJointDef();
    }

    void dtor_native_friction_joint_def(Interpreter *vm, void *data)
    {
        (void)vm;
        delete static_cast<b2FrictionJointDef *>(data);
    }

    int native_friction_joint_def_set_body_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("set_body_a expects 1 arg (Body)");
            return 0;
        }
        b2Body *body = require_body_arg(vm, args[0], "FrictionJointDef.set_body_a");
        if (!body)
            return 0;
        static_cast<b2FrictionJointDef *>(data)->bodyA = body;
        return 0;
    }

    int native_friction_joint_def_set_body_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("set_body_b expects 1 arg (Body)");
            return 0;
        }
        b2Body *body = require_body_arg(vm, args[0], "FrictionJointDef.set_body_b");
        if (!body)
            return 0;
        static_cast<b2FrictionJointDef *>(data)->bodyB = body;
        return 0;
    }

    int native_friction_joint_def_initialize(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 4 || !args[2].isNumber() || !args[3].isNumber())
        {
            Error("initialize expects 4 args (bodyA, bodyB, x, y)");
            return 0;
        }
        b2Body *bodyA = require_body_arg(vm, args[0], "FrictionJointDef.initialize");
        if (!bodyA)
            return 0;
        b2Body *bodyB = require_body_arg(vm, args[1], "FrictionJointDef.initialize");
        if (!bodyB)
            return 0;
        static_cast<b2FrictionJointDef *>(data)->Initialize(
            bodyA,
            bodyB,
            b2Vec2(((float)args[2].asNumber()), ((float)args[3].asNumber())));
        return 0;
    }

    int native_friction_joint_def_set_local_anchor_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_local_anchor_a expects 2 args (x, y)");
            return 0;
        }
        b2FrictionJointDef *def = static_cast<b2FrictionJointDef *>(data);
        def->localAnchorA.Set(((float)args[0].asNumber()), ((float)args[1].asNumber()));
        return 0;
    }

    int native_friction_joint_def_set_local_anchor_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_local_anchor_b expects 2 args (x, y)");
            return 0;
        }
        b2FrictionJointDef *def = static_cast<b2FrictionJointDef *>(data);
        def->localAnchorB.Set(((float)args[0].asNumber()), ((float)args[1].asNumber()));
        return 0;
    }

    int native_friction_joint_def_set_max_force(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_max_force expects 1 arg");
            return 0;
        }
        static_cast<b2FrictionJointDef *>(data)->maxForce = (float)args[0].asNumber();
        return 0;
    }

    int native_friction_joint_def_set_max_torque(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_max_torque expects 1 arg");
            return 0;
        }
        static_cast<b2FrictionJointDef *>(data)->maxTorque = (float)args[0].asNumber();
        return 0;
    }

    int native_friction_joint_def_set_collide_connected(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("set_collide_connected expects 1 bool arg");
            return 0;
        }
        static_cast<b2FrictionJointDef *>(data)->collideConnected = enabled;
        return 0;
    }

    void *ctor_native_friction_joint(Interpreter *vm, int argCount, Value *args)
    {
        if (argCount != 1 && argCount != 3)
        {
            Error("FrictionJoint expects (FrictionJointDef) or (Body, Body, FrictionJointDef)");
            return nullptr;
        }

        int defArgIndex = (argCount == 3) ? 2 : 0;
        NativeClassInstance *defInstance = require_native_instance(vm, args[defArgIndex], kFrictionJointDefClass, "FrictionJoint");
        if (!defInstance)
            return nullptr;

        b2FrictionJointDef def = *static_cast<b2FrictionJointDef *>(defInstance->userData);
        if (argCount == 3)
        {
            b2Body *bodyA = require_body_arg(vm, args[0], "FrictionJoint");
            if (!bodyA)
                return nullptr;
            b2Body *bodyB = require_body_arg(vm, args[1], "FrictionJoint");
            if (!bodyB)
                return nullptr;
            def.bodyA = bodyA;
            def.bodyB = bodyB;
        }
        if (!def.bodyA || !def.bodyB)
        {
            Error("FrictionJointDef needs bodyA and bodyB");
            return nullptr;
        }

        b2World *world = resolve_world_from_bodies(def.bodyA, def.bodyB, "FrictionJoint");
        if (!world)
            return nullptr;
        if (world->IsLocked())
        {
            Error("FrictionJoint cannot be created while world is locked");
            return nullptr;
        }

        b2Joint *joint = world->CreateJoint(&def);
        if (!joint)
        {
            Error("FrictionJoint failed to create joint");
            return nullptr;
        }
        return static_cast<b2FrictionJoint *>(joint);
    }

    void dtor_native_friction_joint(Interpreter *vm, void *data)
    {
        (void)vm;
        (void)data;
    }

    int native_friction_joint_set_max_force(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_max_force expects 1 arg");
            return 0;
        }
        b2FrictionJoint *joint = as_joint<b2FrictionJoint>(data, "set_max_force", kFrictionJointClass);
        if (!joint)
            return 0;
        joint->SetMaxForce((float)args[0].asNumber());
        return 0;
    }

    int native_friction_joint_get_max_force(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_max_force expects no args");
            vm->pushDouble(0);
            return 1;
        }
        b2FrictionJoint *joint = as_joint<b2FrictionJoint>(data, "get_max_force", kFrictionJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble(joint->GetMaxForce());
        return 1;
    }

    int native_friction_joint_set_max_torque(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_max_torque expects 1 arg");
            return 0;
        }
        b2FrictionJoint *joint = as_joint<b2FrictionJoint>(data, "set_max_torque", kFrictionJointClass);
        if (!joint)
            return 0;
        joint->SetMaxTorque((float)args[0].asNumber());
        return 0;
    }

    int native_friction_joint_get_max_torque(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_max_torque expects no args");
            vm->pushDouble(0);
            return 1;
        }
        b2FrictionJoint *joint = as_joint<b2FrictionJoint>(data, "get_max_torque", kFrictionJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble(joint->GetMaxTorque());
        return 1;
    }

    int native_friction_joint_get_anchor_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_anchor_a expects no args");
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2FrictionJoint *joint = as_joint<b2FrictionJoint>(data, "get_anchor_a", kFrictionJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2Vec2 a = joint->GetAnchorA();
        vm->pushDouble((a.x));
        vm->pushDouble((a.y));
        return 2;
    }

    int native_friction_joint_get_anchor_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_anchor_b expects no args");
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2FrictionJoint *joint = as_joint<b2FrictionJoint>(data, "get_anchor_b", kFrictionJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2Vec2 a = joint->GetAnchorB();
        vm->pushDouble((a.x));
        vm->pushDouble((a.y));
        return 2;
    }

    int native_friction_joint_destroy(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("destroy expects no args");
            return 0;
        }
        destroy_joint_now(data);
        return 0;
    }

    int native_friction_joint_exists(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("exists expects no args");
            vm->pushBool(false);
            return 1;
        }
        vm->pushBool(joint_data_exists(data));
        return 1;
    }

    // -------------------------------------------------------------------------
    // GearJointDef / GearJoint
    // -------------------------------------------------------------------------

    void *ctor_native_gear_joint_def(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("GearJointDef expects no args");
            return nullptr;
        }
        return new b2GearJointDef();
    }

    void dtor_native_gear_joint_def(Interpreter *vm, void *data)
    {
        (void)vm;
        delete static_cast<b2GearJointDef *>(data);
    }

    int native_gear_joint_def_set_body_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("set_body_a expects 1 arg (Body)");
            return 0;
        }
        b2Body *body = require_body_arg(vm, args[0], "GearJointDef.set_body_a");
        if (!body)
            return 0;
        static_cast<b2GearJointDef *>(data)->bodyA = body;
        return 0;
    }

    int native_gear_joint_def_set_body_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("set_body_b expects 1 arg (Body)");
            return 0;
        }
        b2Body *body = require_body_arg(vm, args[0], "GearJointDef.set_body_b");
        if (!body)
            return 0;
        static_cast<b2GearJointDef *>(data)->bodyB = body;
        return 0;
    }

    int native_gear_joint_def_set_joint1(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("set_joint1 expects 1 arg (RevoluteJoint|PrismaticJoint)");
            return 0;
        }
        b2Joint *joint = require_gear_input_joint_arg(vm, args[0], "GearJointDef.set_joint1");
        if (!joint)
            return 0;
        static_cast<b2GearJointDef *>(data)->joint1 = joint;
        return 0;
    }

    int native_gear_joint_def_set_joint2(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("set_joint2 expects 1 arg (RevoluteJoint|PrismaticJoint)");
            return 0;
        }
        b2Joint *joint = require_gear_input_joint_arg(vm, args[0], "GearJointDef.set_joint2");
        if (!joint)
            return 0;
        static_cast<b2GearJointDef *>(data)->joint2 = joint;
        return 0;
    }

    int native_gear_joint_def_set_ratio(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_ratio expects 1 arg");
            return 0;
        }
        static_cast<b2GearJointDef *>(data)->ratio = (float)args[0].asNumber();
        return 0;
    }

    int native_gear_joint_def_set_collide_connected(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("set_collide_connected expects 1 bool arg");
            return 0;
        }
        static_cast<b2GearJointDef *>(data)->collideConnected = enabled;
        return 0;
    }

    void *ctor_native_gear_joint(Interpreter *vm, int argCount, Value *args)
    {
        if (argCount != 1 && argCount != 3)
        {
            Error("GearJoint expects (GearJointDef) or (Body, Body, GearJointDef)");
            return nullptr;
        }

        int defArgIndex = (argCount == 3) ? 2 : 0;
        NativeClassInstance *defInstance = require_native_instance(vm, args[defArgIndex], kGearJointDefClass, "GearJoint");
        if (!defInstance)
            return nullptr;

        b2GearJointDef def = *static_cast<b2GearJointDef *>(defInstance->userData);
        if (argCount == 3)
        {
            b2Body *bodyA = require_body_arg(vm, args[0], "GearJoint");
            if (!bodyA)
                return nullptr;
            b2Body *bodyB = require_body_arg(vm, args[1], "GearJoint");
            if (!bodyB)
                return nullptr;
            def.bodyA = bodyA;
            def.bodyB = bodyB;
        }
        if (!def.bodyA || !def.bodyB)
        {
            Error("GearJointDef needs bodyA and bodyB");
            return nullptr;
        }
        if (!def.joint1 || !def.joint2)
        {
            Error("GearJointDef needs joint1 and joint2");
            return nullptr;
        }

        b2World *world = resolve_world_from_bodies(def.bodyA, def.bodyB, "GearJoint");
        if (!world)
            return nullptr;
        if (world->IsLocked())
        {
            Error("GearJoint cannot be created while world is locked");
            return nullptr;
        }

        b2Joint *joint = world->CreateJoint(&def);
        if (!joint)
        {
            Error("GearJoint failed to create joint");
            return nullptr;
        }
        return static_cast<b2GearJoint *>(joint);
    }

    void dtor_native_gear_joint(Interpreter *vm, void *data)
    {
        (void)vm;
        (void)data;
    }

    int native_gear_joint_set_ratio(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_ratio expects 1 arg");
            return 0;
        }
        b2GearJoint *joint = as_joint<b2GearJoint>(data, "set_ratio", kGearJointClass);
        if (!joint)
            return 0;
        joint->SetRatio((float)args[0].asNumber());
        return 0;
    }

    int native_gear_joint_get_ratio(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_ratio expects no args");
            vm->pushDouble(0);
            return 1;
        }
        b2GearJoint *joint = as_joint<b2GearJoint>(data, "get_ratio", kGearJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble(joint->GetRatio());
        return 1;
    }

    int native_gear_joint_get_anchor_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_anchor_a expects no args");
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2GearJoint *joint = as_joint<b2GearJoint>(data, "get_anchor_a", kGearJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2Vec2 a = joint->GetAnchorA();
        vm->pushDouble((a.x));
        vm->pushDouble((a.y));
        return 2;
    }

    int native_gear_joint_get_anchor_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_anchor_b expects no args");
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2GearJoint *joint = as_joint<b2GearJoint>(data, "get_anchor_b", kGearJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2Vec2 a = joint->GetAnchorB();
        vm->pushDouble((a.x));
        vm->pushDouble((a.y));
        return 2;
    }

    int native_gear_joint_destroy(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("destroy expects no args");
            return 0;
        }
        destroy_joint_now(data);
        return 0;
    }

    int native_gear_joint_exists(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("exists expects no args");
            vm->pushBool(false);
            return 1;
        }
        vm->pushBool(joint_data_exists(data));
        return 1;
    }

    // -------------------------------------------------------------------------
    // MotorJointDef / MotorJoint
    // -------------------------------------------------------------------------

    void *ctor_native_motor_joint_def(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("MotorJointDef expects no args");
            return nullptr;
        }
        return new b2MotorJointDef();
    }

    void dtor_native_motor_joint_def(Interpreter *vm, void *data)
    {
        (void)vm;
        delete static_cast<b2MotorJointDef *>(data);
    }

    int native_motor_joint_def_set_body_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("set_body_a expects 1 arg (Body)");
            return 0;
        }
        b2Body *body = require_body_arg(vm, args[0], "MotorJointDef.set_body_a");
        if (!body)
            return 0;
        static_cast<b2MotorJointDef *>(data)->bodyA = body;
        return 0;
    }

    int native_motor_joint_def_set_body_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("set_body_b expects 1 arg (Body)");
            return 0;
        }
        b2Body *body = require_body_arg(vm, args[0], "MotorJointDef.set_body_b");
        if (!body)
            return 0;
        static_cast<b2MotorJointDef *>(data)->bodyB = body;
        return 0;
    }

    int native_motor_joint_def_initialize(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("initialize expects 2 args (bodyA, bodyB)");
            return 0;
        }
        b2Body *bodyA = require_body_arg(vm, args[0], "MotorJointDef.initialize");
        if (!bodyA)
            return 0;
        b2Body *bodyB = require_body_arg(vm, args[1], "MotorJointDef.initialize");
        if (!bodyB)
            return 0;
        static_cast<b2MotorJointDef *>(data)->Initialize(bodyA, bodyB);
        return 0;
    }

    int native_motor_joint_def_set_linear_offset(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_linear_offset expects 2 args (x, y)");
            return 0;
        }
        static_cast<b2MotorJointDef *>(data)->linearOffset.Set(
            ((float)args[0].asNumber()),
            ((float)args[1].asNumber()));
        return 0;
    }

    int native_motor_joint_def_set_angular_offset(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_angular_offset expects 1 arg (radians)");
            return 0;
        }
        static_cast<b2MotorJointDef *>(data)->angularOffset = ((float)args[0].asNumber());
        return 0;
    }

    int native_motor_joint_def_set_max_force(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_max_force expects 1 arg");
            return 0;
        }
        static_cast<b2MotorJointDef *>(data)->maxForce = (float)args[0].asNumber();
        return 0;
    }

    int native_motor_joint_def_set_max_torque(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_max_torque expects 1 arg");
            return 0;
        }
        static_cast<b2MotorJointDef *>(data)->maxTorque = (float)args[0].asNumber();
        return 0;
    }

    int native_motor_joint_def_set_correction_factor(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_correction_factor expects 1 arg");
            return 0;
        }
        static_cast<b2MotorJointDef *>(data)->correctionFactor = (float)args[0].asNumber();
        return 0;
    }

    int native_motor_joint_def_set_collide_connected(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("set_collide_connected expects 1 bool arg");
            return 0;
        }
        static_cast<b2MotorJointDef *>(data)->collideConnected = enabled;
        return 0;
    }

    void *ctor_native_motor_joint(Interpreter *vm, int argCount, Value *args)
    {
        if (argCount != 1 && argCount != 3)
        {
            Error("MotorJoint expects (MotorJointDef) or (Body, Body, MotorJointDef)");
            return nullptr;
        }

        int defArgIndex = (argCount == 3) ? 2 : 0;
        NativeClassInstance *defInstance = require_native_instance(vm, args[defArgIndex], kMotorJointDefClass, "MotorJoint");
        if (!defInstance)
            return nullptr;

        b2MotorJointDef def = *static_cast<b2MotorJointDef *>(defInstance->userData);
        if (argCount == 3)
        {
            b2Body *bodyA = require_body_arg(vm, args[0], "MotorJoint");
            if (!bodyA)
                return nullptr;
            b2Body *bodyB = require_body_arg(vm, args[1], "MotorJoint");
            if (!bodyB)
                return nullptr;
            def.bodyA = bodyA;
            def.bodyB = bodyB;
        }

        if (!def.bodyA || !def.bodyB)
        {
            Error("MotorJointDef needs bodyA and bodyB");
            return nullptr;
        }

        b2World *world = resolve_world_from_bodies(def.bodyA, def.bodyB, "MotorJoint");
        if (!world)
            return nullptr;
        if (world->IsLocked())
        {
            Error("MotorJoint cannot be created while world is locked");
            return nullptr;
        }

        b2Joint *joint = world->CreateJoint(&def);
        if (!joint)
        {
            Error("MotorJoint failed to create joint");
            return nullptr;
        }
        return static_cast<b2MotorJoint *>(joint);
    }

    void dtor_native_motor_joint(Interpreter *vm, void *data)
    {
        (void)vm;
        (void)data;
    }

    int native_motor_joint_set_linear_offset(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_linear_offset expects 2 args (x, y)");
            return 0;
        }
        b2MotorJoint *joint = as_joint<b2MotorJoint>(data, "set_linear_offset", kMotorJointClass);
        if (!joint)
            return 0;
        joint->SetLinearOffset(b2Vec2(
            ((float)args[0].asNumber()),
            ((float)args[1].asNumber())));
        return 0;
    }

    int native_motor_joint_get_linear_offset(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_linear_offset expects no args");
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2MotorJoint *joint = as_joint<b2MotorJoint>(data, "get_linear_offset", kMotorJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        const b2Vec2 &offset = joint->GetLinearOffset();
        vm->pushDouble((offset.x));
        vm->pushDouble((offset.y));
        return 2;
    }

    int native_motor_joint_set_angular_offset(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_angular_offset expects 1 arg (radians)");
            return 0;
        }
        b2MotorJoint *joint = as_joint<b2MotorJoint>(data, "set_angular_offset", kMotorJointClass);
        if (!joint)
            return 0;
        joint->SetAngularOffset(((float)args[0].asNumber()));
        return 0;
    }

    int native_motor_joint_get_angular_offset(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_angular_offset expects no args");
            vm->pushDouble(0);
            return 1;
        }
        b2MotorJoint *joint = as_joint<b2MotorJoint>(data, "get_angular_offset", kMotorJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble((joint->GetAngularOffset()));
        return 1;
    }

    int native_motor_joint_set_max_force(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_max_force expects 1 arg");
            return 0;
        }
        b2MotorJoint *joint = as_joint<b2MotorJoint>(data, "set_max_force", kMotorJointClass);
        if (!joint)
            return 0;
        joint->SetMaxForce((float)args[0].asNumber());
        return 0;
    }

    int native_motor_joint_get_max_force(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_max_force expects no args");
            vm->pushDouble(0);
            return 1;
        }
        b2MotorJoint *joint = as_joint<b2MotorJoint>(data, "get_max_force", kMotorJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble(joint->GetMaxForce());
        return 1;
    }

    int native_motor_joint_set_max_torque(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_max_torque expects 1 arg");
            return 0;
        }
        b2MotorJoint *joint = as_joint<b2MotorJoint>(data, "set_max_torque", kMotorJointClass);
        if (!joint)
            return 0;
        joint->SetMaxTorque((float)args[0].asNumber());
        return 0;
    }

    int native_motor_joint_get_max_torque(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_max_torque expects no args");
            vm->pushDouble(0);
            return 1;
        }
        b2MotorJoint *joint = as_joint<b2MotorJoint>(data, "get_max_torque", kMotorJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble(joint->GetMaxTorque());
        return 1;
    }

    int native_motor_joint_set_correction_factor(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_correction_factor expects 1 arg");
            return 0;
        }
        b2MotorJoint *joint = as_joint<b2MotorJoint>(data, "set_correction_factor", kMotorJointClass);
        if (!joint)
            return 0;
        joint->SetCorrectionFactor((float)args[0].asNumber());
        return 0;
    }

    int native_motor_joint_get_correction_factor(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_correction_factor expects no args");
            vm->pushDouble(0);
            return 1;
        }
        b2MotorJoint *joint = as_joint<b2MotorJoint>(data, "get_correction_factor", kMotorJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            return 1;
        }
        vm->pushDouble(joint->GetCorrectionFactor());
        return 1;
    }

    int native_motor_joint_get_anchor_a(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_anchor_a expects no args");
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2MotorJoint *joint = as_joint<b2MotorJoint>(data, "get_anchor_a", kMotorJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2Vec2 a = joint->GetAnchorA();
        vm->pushDouble((a.x));
        vm->pushDouble((a.y));
        return 2;
    }

    int native_motor_joint_get_anchor_b(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_anchor_b expects no args");
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2MotorJoint *joint = as_joint<b2MotorJoint>(data, "get_anchor_b", kMotorJointClass);
        if (!joint)
        {
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2Vec2 a = joint->GetAnchorB();
        vm->pushDouble((a.x));
        vm->pushDouble((a.y));
        return 2;
    }

    int native_motor_joint_destroy(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("destroy expects no args");
            return 0;
        }
        destroy_joint_now(data);
        return 0;
    }

    int native_motor_joint_exists(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("exists expects no args");
            vm->pushBool(false);
            return 1;
        }
        vm->pushBool(joint_data_exists(data));
        return 1;
    }

    // -------------------------------------------------------------------------
    // b2RopeTuning / b2RopeDef / b2Rope (direct Box2D API)
    // -------------------------------------------------------------------------

    void *ctor_native_b2_rope_tuning(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("b2RopeTuning expects no args");
            return nullptr;
        }
        return new b2RopeTuning();
    }

    void dtor_native_b2_rope_tuning(Interpreter *vm, void *data)
    {
        (void)vm;
        delete static_cast<b2RopeTuning *>(data);
    }

    int native_b2_rope_tuning_set_stretching_model(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_stretching_model expects 1 number arg");
            return 0;
        }
        b2RopeTuning *tuning = as_b2_rope_tuning(data, "set_stretching_model");
        if (!tuning)
            return 0;
        tuning->stretchingModel = (b2StretchingModel)(int)args[0].asNumber();
        return 0;
    }

    int native_b2_rope_tuning_set_bending_model(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_bending_model expects 1 number arg");
            return 0;
        }
        b2RopeTuning *tuning = as_b2_rope_tuning(data, "set_bending_model");
        if (!tuning)
            return 0;
        tuning->bendingModel = (b2BendingModel)(int)args[0].asNumber();
        return 0;
    }

    int native_b2_rope_tuning_set_damping(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_damping expects 1 number arg");
            return 0;
        }
        b2RopeTuning *tuning = as_b2_rope_tuning(data, "set_damping");
        if (!tuning)
            return 0;
        tuning->damping = (float)args[0].asNumber();
        return 0;
    }

    int native_b2_rope_tuning_set_stretch_stiffness(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_stretch_stiffness expects 1 number arg");
            return 0;
        }
        b2RopeTuning *tuning = as_b2_rope_tuning(data, "set_stretch_stiffness");
        if (!tuning)
            return 0;
        tuning->stretchStiffness = (float)args[0].asNumber();
        return 0;
    }

    int native_b2_rope_tuning_set_stretch_hertz(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_stretch_hertz expects 1 number arg");
            return 0;
        }
        b2RopeTuning *tuning = as_b2_rope_tuning(data, "set_stretch_hertz");
        if (!tuning)
            return 0;
        tuning->stretchHertz = (float)args[0].asNumber();
        return 0;
    }

    int native_b2_rope_tuning_set_stretch_damping(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_stretch_damping expects 1 number arg");
            return 0;
        }
        b2RopeTuning *tuning = as_b2_rope_tuning(data, "set_stretch_damping");
        if (!tuning)
            return 0;
        tuning->stretchDamping = (float)args[0].asNumber();
        return 0;
    }

    int native_b2_rope_tuning_set_bend_stiffness(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_bend_stiffness expects 1 number arg");
            return 0;
        }
        b2RopeTuning *tuning = as_b2_rope_tuning(data, "set_bend_stiffness");
        if (!tuning)
            return 0;
        tuning->bendStiffness = (float)args[0].asNumber();
        return 0;
    }

    int native_b2_rope_tuning_set_bend_hertz(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_bend_hertz expects 1 number arg");
            return 0;
        }
        b2RopeTuning *tuning = as_b2_rope_tuning(data, "set_bend_hertz");
        if (!tuning)
            return 0;
        tuning->bendHertz = (float)args[0].asNumber();
        return 0;
    }

    int native_b2_rope_tuning_set_bend_damping(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_bend_damping expects 1 number arg");
            return 0;
        }
        b2RopeTuning *tuning = as_b2_rope_tuning(data, "set_bend_damping");
        if (!tuning)
            return 0;
        tuning->bendDamping = (float)args[0].asNumber();
        return 0;
    }

    int native_b2_rope_tuning_set_isometric(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("set_isometric expects 1 bool arg");
            return 0;
        }
        b2RopeTuning *tuning = as_b2_rope_tuning(data, "set_isometric");
        if (!tuning)
            return 0;
        tuning->isometric = enabled;
        return 0;
    }

    int native_b2_rope_tuning_set_fixed_effective_mass(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("set_fixed_effective_mass expects 1 bool arg");
            return 0;
        }
        b2RopeTuning *tuning = as_b2_rope_tuning(data, "set_fixed_effective_mass");
        if (!tuning)
            return 0;
        tuning->fixedEffectiveMass = enabled;
        return 0;
    }

    int native_b2_rope_tuning_set_warm_start(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("set_warm_start expects 1 bool arg");
            return 0;
        }
        b2RopeTuning *tuning = as_b2_rope_tuning(data, "set_warm_start");
        if (!tuning)
            return 0;
        tuning->warmStart = enabled;
        return 0;
    }

    void *ctor_native_b2_rope_def(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("b2RopeDef expects 1 number arg (count)");
            return nullptr;
        }

        int32 count = (int32)args[0].asNumber();
        if (count <= 0)
        {
            Error("b2RopeDef count must be > 0");
            return nullptr;
        }

        b2RopeDef *def = new b2RopeDef();
        def->vertices = new b2Vec2[count];
        def->masses = new float[count];
        def->count = count;
        return def;
    }

    void dtor_native_b2_rope_def(Interpreter *vm, void *data)
    {
        (void)vm;
        b2RopeDef *def = static_cast<b2RopeDef *>(data);
        if (def)
        {
            delete[] def->vertices;
            delete[] def->masses; 
            def->vertices = nullptr;
            def->masses = nullptr;
            def->count = 0;
        }
        delete def;
    }

    int native_b2_rope_def_set_position(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_position expects 2 number args (x, y)");
            return 0;
        }
        b2RopeDef *def = as_b2_rope_def(data, "set_position");
        if (!def)
            return 0;
        def->position.Set(((float)args[0].asNumber()), ((float)args[1].asNumber()));
        return 0;
    }

    int native_b2_rope_def_set_gravity(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("set_gravity expects 2 number args (gx, gy)");
            return 0;
        }
        b2RopeDef *def = as_b2_rope_def(data, "set_gravity");
        if (!def)
            return 0;
        def->gravity.Set((float)args[0].asNumber(), (float)args[1].asNumber());
        return 0;
    }

    int native_b2_rope_def_set_tuning(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("set_tuning expects 1 arg (b2RopeTuning)");
            return 0;
        }
        b2RopeDef *def = as_b2_rope_def(data, "set_tuning");
        if (!def)
            return 0;
        b2RopeTuning *tuning = require_b2_rope_tuning_arg(vm, args[0], "b2RopeDef.set_tuning");
        if (!tuning)
            return 0;
        def->tuning = *tuning;
        return 0;
    }

    int native_b2_rope_def_set_stretching_model(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_stretching_model expects 1 number arg");
            return 0;
        }
        b2RopeDef *def = as_b2_rope_def(data, "set_stretching_model");
        if (!def)
            return 0;
        def->tuning.stretchingModel = (b2StretchingModel)(int)args[0].asNumber();
        return 0;
    }

    int native_b2_rope_def_set_bending_model(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_bending_model expects 1 number arg");
            return 0;
        }
        b2RopeDef *def = as_b2_rope_def(data, "set_bending_model");
        if (!def)
            return 0;
        def->tuning.bendingModel = (b2BendingModel)(int)args[0].asNumber();
        return 0;
    }

    int native_b2_rope_def_set_damping(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_damping expects 1 number arg");
            return 0;
        }
        b2RopeDef *def = as_b2_rope_def(data, "set_damping");
        if (!def)
            return 0;
        def->tuning.damping = (float)args[0].asNumber();
        return 0;
    }

    int native_b2_rope_def_set_stretch_stiffness(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_stretch_stiffness expects 1 number arg");
            return 0;
        }
        b2RopeDef *def = as_b2_rope_def(data, "set_stretch_stiffness");
        if (!def)
            return 0;
        def->tuning.stretchStiffness = (float)args[0].asNumber();
        return 0;
    }

    int native_b2_rope_def_set_stretch_hertz(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_stretch_hertz expects 1 number arg");
            return 0;
        }
        b2RopeDef *def = as_b2_rope_def(data, "set_stretch_hertz");
        if (!def)
            return 0;
        def->tuning.stretchHertz = (float)args[0].asNumber();
        return 0;
    }

    int native_b2_rope_def_set_stretch_damping(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_stretch_damping expects 1 number arg");
            return 0;
        }
        b2RopeDef *def = as_b2_rope_def(data, "set_stretch_damping");
        if (!def)
            return 0;
        def->tuning.stretchDamping = (float)args[0].asNumber();
        return 0;
    }

    int native_b2_rope_def_set_bend_stiffness(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_bend_stiffness expects 1 number arg");
            return 0;
        }
        b2RopeDef *def = as_b2_rope_def(data, "set_bend_stiffness");
        if (!def)
            return 0;
        def->tuning.bendStiffness = (float)args[0].asNumber();
        return 0;
    }

    int native_b2_rope_def_set_bend_hertz(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_bend_hertz expects 1 number arg");
            return 0;
        }
        b2RopeDef *def = as_b2_rope_def(data, "set_bend_hertz");
        if (!def)
            return 0;
        def->tuning.bendHertz = (float)args[0].asNumber();
        return 0;
    }

    int native_b2_rope_def_set_bend_damping(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("set_bend_damping expects 1 number arg");
            return 0;
        }
        b2RopeDef *def = as_b2_rope_def(data, "set_bend_damping");
        if (!def)
            return 0;
        def->tuning.bendDamping = (float)args[0].asNumber();
        return 0;
    }

    int native_b2_rope_def_set_isometric(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("set_isometric expects 1 bool arg");
            return 0;
        }
        b2RopeDef *def = as_b2_rope_def(data, "set_isometric");
        if (!def)
            return 0;
        def->tuning.isometric = enabled;
        return 0;
    }

    int native_b2_rope_def_set_fixed_effective_mass(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("set_fixed_effective_mass expects 1 bool arg");
            return 0;
        }
        b2RopeDef *def = as_b2_rope_def(data, "set_fixed_effective_mass");
        if (!def)
            return 0;
        def->tuning.fixedEffectiveMass = enabled;
        return 0;
    }

    int native_b2_rope_def_set_warm_start(Interpreter *vm, void *data, int argCount, Value *args)
    {
        bool enabled = false;
        if (argCount != 1 || !value_to_bool(args[0], &enabled))
        {
            Error("set_warm_start expects 1 bool arg");
            return 0;
        }
        b2RopeDef *def = as_b2_rope_def(data, "set_warm_start");
        if (!def)
            return 0;
        def->tuning.warmStart = enabled;
        return 0;
    }

    int native_b2_rope_def_set_vertices(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isArray() || !args[1].isArray())
        {
            Error("set_vertices expects (pointsArray, massesArray)");
            return 0;
        }

        b2RopeDef *def = as_b2_rope_def(data, "set_vertices");
        if (!def)
            return 0;

        ArrayInstance *pointsArray = args[0].asArray();
        ArrayInstance *massesArray = args[1].asArray();
        const size_t pointValueCount = pointsArray->values.size();

        if ((pointValueCount % 2) != 0)
        {
            Error("set_vertices expects even point array [x0, y0, ...]");
            return 0;
        }

        const size_t pointCount = pointValueCount / 2;
        if (pointCount < 3)
        {
            Error("set_vertices needs at least 3 points");
            return 0;
        }
        if (massesArray->values.size() != pointCount)
        {
            Error("set_vertices masses length must be %d", (int)pointCount);
            return 0;
        }

        int32 capacity = def->count;
        if (capacity <= 0)
        {
            Error("set_vertices invalid b2RopeDef count");
            return 0;
        }
        if ((int32)pointCount != capacity)
        {
            Error("set_vertices expects exactly %d points", (int)capacity);
            return 0;
        }

        for (size_t i = 0; i < pointCount; ++i)
        {
            const Value &vx = pointsArray->values[i * 2];
            const Value &vy = pointsArray->values[i * 2 + 1];
            const Value &mv = massesArray->values[i];
            if (!vx.isNumber() || !vy.isNumber() || !mv.isNumber())
            {
                Error("set_vertices expects numeric points and masses");
                return 0;
            }
            def->vertices[i].Set(
                ((float)vx.asNumber()),
                ((float)vy.asNumber()));
            def->masses[i] = (float)mv.asNumber();
        }

        return 0;
    }

    int native_b2_rope_def_clear_vertices(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("clear_vertices expects no args");
            return 0;
        }
        b2RopeDef *def = as_b2_rope_def(data, "clear_vertices");
        if (!def)
            return 0;
        for (int32 i = 0; i < def->count; ++i)
        {
            def->vertices[i].Set(0.0f, 0.0f);
            def->masses[i] = 0.0f;
        }
        return 0;
    }

    void *ctor_native_b2_rope(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("b2Rope expects no args");
            return nullptr;
        }
        return new b2Rope();
    }

    void dtor_native_b2_rope(Interpreter *vm, void *data)
    {
        (void)vm;
        delete static_cast<b2Rope *>(data);
    }

    int native_b2_rope_create(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("create expects 1 arg (b2RopeDef)");
            return 0;
        }
        b2Rope *rope = as_b2_rope(data, "create");
        if (!rope)
            return 0;
        b2RopeDef *def = require_b2_rope_def_arg(vm, args[0], "b2Rope.create");
        if (!def)
            return 0;
        if (!def->vertices || !def->masses || def->count < 3)
        {
            Error("b2RopeDef is missing vertices/masses/count");
            return 0;
        }
        rope->Create(*def);
        return 0;
    }

    int native_b2_rope_set_tuning(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("set_tuning expects 1 arg (b2RopeTuning)");
            return 0;
        }
        b2Rope *rope = as_b2_rope(data, "set_tuning");
        if (!rope)
            return 0;
        b2RopeTuning *tuning = require_b2_rope_tuning_arg(vm, args[0], "b2Rope.set_tuning");
        if (!tuning)
            return 0;
        rope->SetTuning(*tuning);
        return 0;
    }

    int native_b2_rope_step(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 4 || !args[0].isNumber() || !args[1].isNumber() || !args[2].isNumber() || !args[3].isNumber())
        {
            Error("step expects 4 args (dt, iterations, x, y)");
            return 0;
        }
        b2Rope *rope = as_b2_rope(data, "step");
        if (!rope)
            return 0;
        rope->Step(
            (float)args[0].asNumber(),
            (int32)args[1].asNumber(),
            b2Vec2(((float)args[2].asNumber()), ((float)args[3].asNumber())));
        return 0;
    }

    int native_b2_rope_reset(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)vm;
        if (argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("reset expects 2 args (x, y)");
            return 0;
        }
        b2Rope *rope = as_b2_rope(data, "reset");
        if (!rope)
            return 0;
        rope->Reset(b2Vec2(((float)args[0].asNumber()), ((float)args[1].asNumber())));
        return 0;
    }

 

    int native_b2_rope_get_count(Interpreter *vm, void *data, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("get_count expects no args");
            vm->pushInt(0);
            return 1;
        }
        b2Rope *rope = as_b2_rope(data, "get_count");
        if (!rope)
        {
            vm->pushInt(0);
            return 1;
        }
        vm->pushInt((int)rope->GetPointCount());
        return 1;
    }

    int native_b2_rope_get_point(Interpreter *vm, void *data, int argCount, Value *args)
    {
        if (argCount != 1 || !args[0].isNumber())
        {
            Error("get_point expects 1 number arg (index)");
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2Rope *rope = as_b2_rope(data, "get_point");
        if (!rope)
        {
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        int32 index = (int32)args[0].asNumber();
        const int32 count = rope->GetPointCount();
        if (index < 0 || index >= count)
        {
            Error("get_point index out of range");
            vm->pushDouble(0);
            vm->pushDouble(0);
            return 2;
        }
        b2Vec2 p = rope->GetPoint(index);
        vm->pushDouble((p.x));
        vm->pushDouble((p.y));
        return 2;
    }

    // -------------------------------------------------------------------------
    // Registration and lifecycle
    // -------------------------------------------------------------------------

    void register_box2d_joints(Interpreter &vm)
    {
        NativeClassDef *mouseJointDef = vm.registerNativeClass(
            kMouseJointDefClass,
            ctor_native_mouse_joint_def,
            dtor_native_mouse_joint_def,
            0,
            false);

        vm.addNativeMethod(mouseJointDef, "set_body_a", native_mouse_joint_def_set_body_a);
        vm.addNativeMethod(mouseJointDef, "set_body_b", native_mouse_joint_def_set_body_b);
        vm.addNativeMethod(mouseJointDef, "initialize", native_mouse_joint_def_initialize);
        vm.addNativeMethod(mouseJointDef, "set_target", native_mouse_joint_def_set_target);
        vm.addNativeMethod(mouseJointDef, "set_max_force", native_mouse_joint_def_set_max_force);
        vm.addNativeMethod(mouseJointDef, "set_force", native_mouse_joint_def_set_max_force);
        vm.addNativeMethod(mouseJointDef, "set_stiffness", native_mouse_joint_def_set_stiffness);
        vm.addNativeMethod(mouseJointDef, "set_damping", native_mouse_joint_def_set_damping);
        vm.addNativeMethod(mouseJointDef, "set_collide_connected", native_mouse_joint_def_set_collide_connected);

        NativeClassDef *mouseJoint = vm.registerNativeClass(
            kMouseJointClass,
            ctor_native_mouse_joint,
            dtor_native_mouse_joint,
            1,
            false);

        vm.addNativeMethod(mouseJoint, "set_target", native_mouse_joint_set_target);
        vm.addNativeMethod(mouseJoint, "target", native_mouse_joint_set_target);
        vm.addNativeMethod(mouseJoint, "set_max_force", native_mouse_joint_set_max_force);
        vm.addNativeMethod(mouseJoint, "set_force", native_mouse_joint_set_max_force);
        vm.addNativeMethod(mouseJoint, "set_stiffness", native_mouse_joint_set_stiffness);
        vm.addNativeMethod(mouseJoint, "set_damping", native_mouse_joint_set_damping);
        vm.addNativeMethod(mouseJoint, "get_target", native_mouse_joint_get_target);
        vm.addNativeMethod(mouseJoint, "destroy", native_mouse_joint_destroy);
        vm.addNativeMethod(mouseJoint, "exists", native_mouse_joint_exists);

        NativeClassDef *revoluteJointDef = vm.registerNativeClass(
            kRevoluteJointDefClass,
            ctor_native_revolute_joint_def,
            dtor_native_revolute_joint_def,
            0,
            false);

        vm.addNativeMethod(revoluteJointDef, "set_body_a", native_revolute_joint_def_set_body_a);
        vm.addNativeMethod(revoluteJointDef, "set_body_b", native_revolute_joint_def_set_body_b);
        vm.addNativeMethod(revoluteJointDef, "initialize", native_revolute_joint_def_initialize);
        vm.addNativeMethod(revoluteJointDef, "set_local_anchor_a", native_revolute_joint_def_set_local_anchor_a);
        vm.addNativeMethod(revoluteJointDef, "set_local_anchor_b", native_revolute_joint_def_set_local_anchor_b);
        vm.addNativeMethod(revoluteJointDef, "set_reference_angle", native_revolute_joint_def_set_reference_angle);
        vm.addNativeMethod(revoluteJointDef, "set_enable_limit", native_revolute_joint_def_set_enable_limit);
        vm.addNativeMethod(revoluteJointDef, "set_limits", native_revolute_joint_def_set_limits);
        vm.addNativeMethod(revoluteJointDef, "set_enable_motor", native_revolute_joint_def_set_enable_motor);
        vm.addNativeMethod(revoluteJointDef, "set_motor_speed", native_revolute_joint_def_set_motor_speed);
        vm.addNativeMethod(revoluteJointDef, "set_max_motor_torque", native_revolute_joint_def_set_max_motor_torque);
        vm.addNativeMethod(revoluteJointDef, "set_collide_connected", native_revolute_joint_def_set_collide_connected);

        NativeClassDef *revoluteJoint = vm.registerNativeClass(
            kRevoluteJointClass,
            ctor_native_revolute_joint,
            dtor_native_revolute_joint,
            1,
            false);

        vm.addNativeMethod(revoluteJoint, "enable_limit", native_revolute_joint_enable_limit);
        vm.addNativeMethod(revoluteJoint, "set_limits", native_revolute_joint_set_limits);
        vm.addNativeMethod(revoluteJoint, "enable_motor", native_revolute_joint_enable_motor);
        vm.addNativeMethod(revoluteJoint, "set_motor_speed", native_revolute_joint_set_motor_speed);
        vm.addNativeMethod(revoluteJoint, "set_max_motor_torque", native_revolute_joint_set_max_motor_torque);
        vm.addNativeMethod(revoluteJoint, "get_joint_angle", native_revolute_joint_get_joint_angle);
        vm.addNativeMethod(revoluteJoint, "get_joint_speed", native_revolute_joint_get_joint_speed);
        vm.addNativeMethod(revoluteJoint, "get_motor_torque", native_revolute_joint_get_motor_torque);
        vm.addNativeMethod(revoluteJoint, "get_anchor_a", native_revolute_joint_get_anchor_a);
        vm.addNativeMethod(revoluteJoint, "get_anchor_b", native_revolute_joint_get_anchor_b);
        vm.addNativeMethod(revoluteJoint, "destroy", native_revolute_joint_destroy);
        vm.addNativeMethod(revoluteJoint, "exists", native_revolute_joint_exists);

        NativeClassDef *wheelJointDef = vm.registerNativeClass(
            kWheelJointDefClass,
            ctor_native_wheel_joint_def,
            dtor_native_wheel_joint_def,
            0,
            false);

        vm.addNativeMethod(wheelJointDef, "set_body_a", native_wheel_joint_def_set_body_a);
        vm.addNativeMethod(wheelJointDef, "set_body_b", native_wheel_joint_def_set_body_b);
        vm.addNativeMethod(wheelJointDef, "initialize", native_wheel_joint_def_initialize);
        vm.addNativeMethod(wheelJointDef, "set_local_anchor_a", native_wheel_joint_def_set_local_anchor_a);
        vm.addNativeMethod(wheelJointDef, "set_local_anchor_b", native_wheel_joint_def_set_local_anchor_b);
        vm.addNativeMethod(wheelJointDef, "set_local_axis_a", native_wheel_joint_def_set_local_axis_a);
        vm.addNativeMethod(wheelJointDef, "set_enable_motor", native_wheel_joint_def_set_enable_motor);
        vm.addNativeMethod(wheelJointDef, "set_max_motor_torque", native_wheel_joint_def_set_max_motor_torque);
        vm.addNativeMethod(wheelJointDef, "set_motor_speed", native_wheel_joint_def_set_motor_speed);
        vm.addNativeMethod(wheelJointDef, "set_stiffness", native_wheel_joint_def_set_stiffness);
        vm.addNativeMethod(wheelJointDef, "set_damping", native_wheel_joint_def_set_damping);
        vm.addNativeMethod(wheelJointDef, "set_collide_connected", native_wheel_joint_def_set_collide_connected);

        NativeClassDef *wheelJoint = vm.registerNativeClass(
            kWheelJointClass,
            ctor_native_wheel_joint,
            dtor_native_wheel_joint,
            1,
            false);

        vm.addNativeMethod(wheelJoint, "enable_motor", native_wheel_joint_enable_motor);
        vm.addNativeMethod(wheelJoint, "set_max_motor_torque", native_wheel_joint_set_max_motor_torque);
        vm.addNativeMethod(wheelJoint, "set_motor_speed", native_wheel_joint_set_motor_speed);
        vm.addNativeMethod(wheelJoint, "set_stiffness", native_wheel_joint_set_stiffness);
        vm.addNativeMethod(wheelJoint, "set_damping", native_wheel_joint_set_damping);
        vm.addNativeMethod(wheelJoint, "get_motor_speed", native_wheel_joint_get_motor_speed);
        vm.addNativeMethod(wheelJoint, "get_joint_translation", native_wheel_joint_get_joint_translation);
        vm.addNativeMethod(wheelJoint, "get_joint_linear_speed", native_wheel_joint_get_joint_linear_speed);
        vm.addNativeMethod(wheelJoint, "get_motor_torque", native_wheel_joint_get_motor_torque);
        vm.addNativeMethod(wheelJoint, "get_anchor_a", native_wheel_joint_get_anchor_a);
        vm.addNativeMethod(wheelJoint, "get_anchor_b", native_wheel_joint_get_anchor_b);
        vm.addNativeMethod(wheelJoint, "destroy", native_wheel_joint_destroy);
        vm.addNativeMethod(wheelJoint, "exists", native_wheel_joint_exists);

        NativeClassDef *distanceJointDef = vm.registerNativeClass(
            kDistanceJointDefClass,
            ctor_native_distance_joint_def,
            dtor_native_distance_joint_def,
            0,
            false);

        vm.addNativeMethod(distanceJointDef, "set_body_a", native_distance_joint_def_set_body_a);
        vm.addNativeMethod(distanceJointDef, "set_body_b", native_distance_joint_def_set_body_b);
        vm.addNativeMethod(distanceJointDef, "initialize", native_distance_joint_def_initialize);
        vm.addNativeMethod(distanceJointDef, "set_local_anchor_a", native_distance_joint_def_set_local_anchor_a);
        vm.addNativeMethod(distanceJointDef, "set_local_anchor_b", native_distance_joint_def_set_local_anchor_b);
        vm.addNativeMethod(distanceJointDef, "set_length", native_distance_joint_def_set_length);
        vm.addNativeMethod(distanceJointDef, "set_min_length", native_distance_joint_def_set_min_length);
        vm.addNativeMethod(distanceJointDef, "set_max_length", native_distance_joint_def_set_max_length);
        vm.addNativeMethod(distanceJointDef, "set_stiffness", native_distance_joint_def_set_stiffness);
        vm.addNativeMethod(distanceJointDef, "set_damping", native_distance_joint_def_set_damping);
        vm.addNativeMethod(distanceJointDef, "set_collide_connected", native_distance_joint_def_set_collide_connected);

        NativeClassDef *distanceJoint = vm.registerNativeClass(
            kDistanceJointClass,
            ctor_native_distance_joint,
            dtor_native_distance_joint,
            1,
            false);

        vm.addNativeMethod(distanceJoint, "set_length", native_distance_joint_set_length);
        vm.addNativeMethod(distanceJoint, "set_min_length", native_distance_joint_set_min_length);
        vm.addNativeMethod(distanceJoint, "set_max_length", native_distance_joint_set_max_length);
        vm.addNativeMethod(distanceJoint, "set_stiffness", native_distance_joint_set_stiffness);
        vm.addNativeMethod(distanceJoint, "set_damping", native_distance_joint_set_damping);
        vm.addNativeMethod(distanceJoint, "get_length", native_distance_joint_get_length);
        vm.addNativeMethod(distanceJoint, "get_current_length", native_distance_joint_get_current_length);
        vm.addNativeMethod(distanceJoint, "get_anchor_a", native_distance_joint_get_anchor_a);
        vm.addNativeMethod(distanceJoint, "get_anchor_b", native_distance_joint_get_anchor_b);
        vm.addNativeMethod(distanceJoint, "destroy", native_distance_joint_destroy);
        vm.addNativeMethod(distanceJoint, "exists", native_distance_joint_exists);

        NativeClassDef *prismaticJointDef = vm.registerNativeClass(
            kPrismaticJointDefClass,
            ctor_native_prismatic_joint_def,
            dtor_native_prismatic_joint_def,
            0,
            false);

        vm.addNativeMethod(prismaticJointDef, "set_body_a", native_prismatic_joint_def_set_body_a);
        vm.addNativeMethod(prismaticJointDef, "set_body_b", native_prismatic_joint_def_set_body_b);
        vm.addNativeMethod(prismaticJointDef, "initialize", native_prismatic_joint_def_initialize);
        vm.addNativeMethod(prismaticJointDef, "set_local_anchor_a", native_prismatic_joint_def_set_local_anchor_a);
        vm.addNativeMethod(prismaticJointDef, "set_local_anchor_b", native_prismatic_joint_def_set_local_anchor_b);
        vm.addNativeMethod(prismaticJointDef, "set_local_axis_a", native_prismatic_joint_def_set_local_axis_a);
        vm.addNativeMethod(prismaticJointDef, "set_reference_angle", native_prismatic_joint_def_set_reference_angle);
        vm.addNativeMethod(prismaticJointDef, "set_enable_limit", native_prismatic_joint_def_set_enable_limit);
        vm.addNativeMethod(prismaticJointDef, "set_limits", native_prismatic_joint_def_set_limits);
        vm.addNativeMethod(prismaticJointDef, "set_enable_motor", native_prismatic_joint_def_set_enable_motor);
        vm.addNativeMethod(prismaticJointDef, "set_motor_speed", native_prismatic_joint_def_set_motor_speed);
        vm.addNativeMethod(prismaticJointDef, "set_max_motor_force", native_prismatic_joint_def_set_max_motor_force);
        vm.addNativeMethod(prismaticJointDef, "set_collide_connected", native_prismatic_joint_def_set_collide_connected);

        NativeClassDef *prismaticJoint = vm.registerNativeClass(
            kPrismaticJointClass,
            ctor_native_prismatic_joint,
            dtor_native_prismatic_joint,
            1,
            false);

        vm.addNativeMethod(prismaticJoint, "enable_limit", native_prismatic_joint_enable_limit);
        vm.addNativeMethod(prismaticJoint, "set_limits", native_prismatic_joint_set_limits);
        vm.addNativeMethod(prismaticJoint, "enable_motor", native_prismatic_joint_enable_motor);
        vm.addNativeMethod(prismaticJoint, "set_motor_speed", native_prismatic_joint_set_motor_speed);
        vm.addNativeMethod(prismaticJoint, "set_max_motor_force", native_prismatic_joint_set_max_motor_force);
        vm.addNativeMethod(prismaticJoint, "get_joint_translation", native_prismatic_joint_get_joint_translation);
        vm.addNativeMethod(prismaticJoint, "get_joint_speed", native_prismatic_joint_get_joint_speed);
        vm.addNativeMethod(prismaticJoint, "get_motor_force", native_prismatic_joint_get_motor_force);
        vm.addNativeMethod(prismaticJoint, "get_anchor_a", native_prismatic_joint_get_anchor_a);
        vm.addNativeMethod(prismaticJoint, "get_anchor_b", native_prismatic_joint_get_anchor_b);
        vm.addNativeMethod(prismaticJoint, "destroy", native_prismatic_joint_destroy);
        vm.addNativeMethod(prismaticJoint, "exists", native_prismatic_joint_exists);

        NativeClassDef *pulleyJointDef = vm.registerNativeClass(
            kPulleyJointDefClass,
            ctor_native_pulley_joint_def,
            dtor_native_pulley_joint_def,
            0,
            false);

        vm.addNativeMethod(pulleyJointDef, "set_body_a", native_pulley_joint_def_set_body_a);
        vm.addNativeMethod(pulleyJointDef, "set_body_b", native_pulley_joint_def_set_body_b);
        vm.addNativeMethod(pulleyJointDef, "initialize", native_pulley_joint_def_initialize);
        vm.addNativeMethod(pulleyJointDef, "set_ground_anchor_a", native_pulley_joint_def_set_ground_anchor_a);
        vm.addNativeMethod(pulleyJointDef, "set_ground_anchor_b", native_pulley_joint_def_set_ground_anchor_b);
        vm.addNativeMethod(pulleyJointDef, "set_local_anchor_a", native_pulley_joint_def_set_local_anchor_a);
        vm.addNativeMethod(pulleyJointDef, "set_local_anchor_b", native_pulley_joint_def_set_local_anchor_b);
        vm.addNativeMethod(pulleyJointDef, "set_length_a", native_pulley_joint_def_set_length_a);
        vm.addNativeMethod(pulleyJointDef, "set_length_b", native_pulley_joint_def_set_length_b);
        vm.addNativeMethod(pulleyJointDef, "set_ratio", native_pulley_joint_def_set_ratio);
        vm.addNativeMethod(pulleyJointDef, "set_collide_connected", native_pulley_joint_def_set_collide_connected);

        NativeClassDef *pulleyJoint = vm.registerNativeClass(
            kPulleyJointClass,
            ctor_native_pulley_joint,
            dtor_native_pulley_joint,
            1,
            false);

        vm.addNativeMethod(pulleyJoint, "get_ratio", native_pulley_joint_get_ratio);
        vm.addNativeMethod(pulleyJoint, "get_length_a", native_pulley_joint_get_length_a);
        vm.addNativeMethod(pulleyJoint, "get_length_b", native_pulley_joint_get_length_b);
        vm.addNativeMethod(pulleyJoint, "get_current_length_a", native_pulley_joint_get_current_length_a);
        vm.addNativeMethod(pulleyJoint, "get_current_length_b", native_pulley_joint_get_current_length_b);
        vm.addNativeMethod(pulleyJoint, "get_anchor_a", native_pulley_joint_get_anchor_a);
        vm.addNativeMethod(pulleyJoint, "get_anchor_b", native_pulley_joint_get_anchor_b);
        vm.addNativeMethod(pulleyJoint, "get_ground_anchor_a", native_pulley_joint_get_ground_anchor_a);
        vm.addNativeMethod(pulleyJoint, "get_ground_anchor_b", native_pulley_joint_get_ground_anchor_b);
        vm.addNativeMethod(pulleyJoint, "destroy", native_pulley_joint_destroy);
        vm.addNativeMethod(pulleyJoint, "exists", native_pulley_joint_exists);

        NativeClassDef *frictionJointDef = vm.registerNativeClass(
            kFrictionJointDefClass,
            ctor_native_friction_joint_def,
            dtor_native_friction_joint_def,
            0,
            false);

        vm.addNativeMethod(frictionJointDef, "set_body_a", native_friction_joint_def_set_body_a);
        vm.addNativeMethod(frictionJointDef, "set_body_b", native_friction_joint_def_set_body_b);
        vm.addNativeMethod(frictionJointDef, "initialize", native_friction_joint_def_initialize);
        vm.addNativeMethod(frictionJointDef, "set_local_anchor_a", native_friction_joint_def_set_local_anchor_a);
        vm.addNativeMethod(frictionJointDef, "set_local_anchor_b", native_friction_joint_def_set_local_anchor_b);
        vm.addNativeMethod(frictionJointDef, "set_max_force", native_friction_joint_def_set_max_force);
        vm.addNativeMethod(frictionJointDef, "set_max_torque", native_friction_joint_def_set_max_torque);
        vm.addNativeMethod(frictionJointDef, "set_collide_connected", native_friction_joint_def_set_collide_connected);

        NativeClassDef *frictionJoint = vm.registerNativeClass(
            kFrictionJointClass,
            ctor_native_friction_joint,
            dtor_native_friction_joint,
            1,
            false);

        vm.addNativeMethod(frictionJoint, "set_max_force", native_friction_joint_set_max_force);
        vm.addNativeMethod(frictionJoint, "get_max_force", native_friction_joint_get_max_force);
        vm.addNativeMethod(frictionJoint, "set_max_torque", native_friction_joint_set_max_torque);
        vm.addNativeMethod(frictionJoint, "get_max_torque", native_friction_joint_get_max_torque);
        vm.addNativeMethod(frictionJoint, "get_anchor_a", native_friction_joint_get_anchor_a);
        vm.addNativeMethod(frictionJoint, "get_anchor_b", native_friction_joint_get_anchor_b);
        vm.addNativeMethod(frictionJoint, "destroy", native_friction_joint_destroy);
        vm.addNativeMethod(frictionJoint, "exists", native_friction_joint_exists);

        NativeClassDef *gearJointDef = vm.registerNativeClass(
            kGearJointDefClass,
            ctor_native_gear_joint_def,
            dtor_native_gear_joint_def,
            0,
            false);

        vm.addNativeMethod(gearJointDef, "set_body_a", native_gear_joint_def_set_body_a);
        vm.addNativeMethod(gearJointDef, "set_body_b", native_gear_joint_def_set_body_b);
        vm.addNativeMethod(gearJointDef, "set_joint1", native_gear_joint_def_set_joint1);
        vm.addNativeMethod(gearJointDef, "set_joint2", native_gear_joint_def_set_joint2);
        vm.addNativeMethod(gearJointDef, "set_ratio", native_gear_joint_def_set_ratio);
        vm.addNativeMethod(gearJointDef, "set_collide_connected", native_gear_joint_def_set_collide_connected);

        NativeClassDef *gearJoint = vm.registerNativeClass(
            kGearJointClass,
            ctor_native_gear_joint,
            dtor_native_gear_joint,
            1,
            false);

        vm.addNativeMethod(gearJoint, "set_ratio", native_gear_joint_set_ratio);
        vm.addNativeMethod(gearJoint, "get_ratio", native_gear_joint_get_ratio);
        vm.addNativeMethod(gearJoint, "get_anchor_a", native_gear_joint_get_anchor_a);
        vm.addNativeMethod(gearJoint, "get_anchor_b", native_gear_joint_get_anchor_b);
        vm.addNativeMethod(gearJoint, "destroy", native_gear_joint_destroy);
        vm.addNativeMethod(gearJoint, "exists", native_gear_joint_exists);

        NativeClassDef *motorJointDef = vm.registerNativeClass(
            kMotorJointDefClass,
            ctor_native_motor_joint_def,
            dtor_native_motor_joint_def,
            0,
            false);

        vm.addNativeMethod(motorJointDef, "set_body_a", native_motor_joint_def_set_body_a);
        vm.addNativeMethod(motorJointDef, "set_body_b", native_motor_joint_def_set_body_b);
        vm.addNativeMethod(motorJointDef, "initialize", native_motor_joint_def_initialize);
        vm.addNativeMethod(motorJointDef, "set_linear_offset", native_motor_joint_def_set_linear_offset);
        vm.addNativeMethod(motorJointDef, "set_angular_offset", native_motor_joint_def_set_angular_offset);
        vm.addNativeMethod(motorJointDef, "set_max_force", native_motor_joint_def_set_max_force);
        vm.addNativeMethod(motorJointDef, "set_max_torque", native_motor_joint_def_set_max_torque);
        vm.addNativeMethod(motorJointDef, "set_correction_factor", native_motor_joint_def_set_correction_factor);
        vm.addNativeMethod(motorJointDef, "set_collide_connected", native_motor_joint_def_set_collide_connected);

        NativeClassDef *motorJoint = vm.registerNativeClass(
            kMotorJointClass,
            ctor_native_motor_joint,
            dtor_native_motor_joint,
            1,
            false);

        vm.addNativeMethod(motorJoint, "set_linear_offset", native_motor_joint_set_linear_offset);
        vm.addNativeMethod(motorJoint, "get_linear_offset", native_motor_joint_get_linear_offset);
        vm.addNativeMethod(motorJoint, "set_angular_offset", native_motor_joint_set_angular_offset);
        vm.addNativeMethod(motorJoint, "get_angular_offset", native_motor_joint_get_angular_offset);
        vm.addNativeMethod(motorJoint, "set_max_force", native_motor_joint_set_max_force);
        vm.addNativeMethod(motorJoint, "get_max_force", native_motor_joint_get_max_force);
        vm.addNativeMethod(motorJoint, "set_max_torque", native_motor_joint_set_max_torque);
        vm.addNativeMethod(motorJoint, "get_max_torque", native_motor_joint_get_max_torque);
        vm.addNativeMethod(motorJoint, "set_correction_factor", native_motor_joint_set_correction_factor);
        vm.addNativeMethod(motorJoint, "get_correction_factor", native_motor_joint_get_correction_factor);
        vm.addNativeMethod(motorJoint, "get_anchor_a", native_motor_joint_get_anchor_a);
        vm.addNativeMethod(motorJoint, "get_anchor_b", native_motor_joint_get_anchor_b);
        vm.addNativeMethod(motorJoint, "destroy", native_motor_joint_destroy);
        vm.addNativeMethod(motorJoint, "exists", native_motor_joint_exists);

        NativeClassDef *b2RopeTuningClass = vm.registerNativeClass(
            kB2RopeTuningClass,
            ctor_native_b2_rope_tuning,
            dtor_native_b2_rope_tuning,
            0,
            false);

        vm.addNativeMethod(b2RopeTuningClass, "set_stretching_model", native_b2_rope_tuning_set_stretching_model);
        vm.addNativeMethod(b2RopeTuningClass, "set_bending_model", native_b2_rope_tuning_set_bending_model);
        vm.addNativeMethod(b2RopeTuningClass, "set_damping", native_b2_rope_tuning_set_damping);
        vm.addNativeMethod(b2RopeTuningClass, "set_stretch_stiffness", native_b2_rope_tuning_set_stretch_stiffness);
        vm.addNativeMethod(b2RopeTuningClass, "set_stretch_hertz", native_b2_rope_tuning_set_stretch_hertz);
        vm.addNativeMethod(b2RopeTuningClass, "set_stretch_damping", native_b2_rope_tuning_set_stretch_damping);
        vm.addNativeMethod(b2RopeTuningClass, "set_bend_stiffness", native_b2_rope_tuning_set_bend_stiffness);
        vm.addNativeMethod(b2RopeTuningClass, "set_bend_hertz", native_b2_rope_tuning_set_bend_hertz);
        vm.addNativeMethod(b2RopeTuningClass, "set_bend_damping", native_b2_rope_tuning_set_bend_damping);
        vm.addNativeMethod(b2RopeTuningClass, "set_isometric", native_b2_rope_tuning_set_isometric);
        vm.addNativeMethod(b2RopeTuningClass, "set_fixed_effective_mass", native_b2_rope_tuning_set_fixed_effective_mass);
        vm.addNativeMethod(b2RopeTuningClass, "set_warm_start", native_b2_rope_tuning_set_warm_start);

        NativeClassDef *b2RopeDefClass = vm.registerNativeClass(
            kB2RopeDefClass,
            ctor_native_b2_rope_def,
            dtor_native_b2_rope_def,
            1,
            false);

        vm.addNativeMethod(b2RopeDefClass, "set_position", native_b2_rope_def_set_position);
        vm.addNativeMethod(b2RopeDefClass, "set_gravity", native_b2_rope_def_set_gravity);
        vm.addNativeMethod(b2RopeDefClass, "set_tuning", native_b2_rope_def_set_tuning);
        vm.addNativeMethod(b2RopeDefClass, "set_stretching_model", native_b2_rope_def_set_stretching_model);
        vm.addNativeMethod(b2RopeDefClass, "set_bending_model", native_b2_rope_def_set_bending_model);
        vm.addNativeMethod(b2RopeDefClass, "set_damping", native_b2_rope_def_set_damping);
        vm.addNativeMethod(b2RopeDefClass, "set_stretch_stiffness", native_b2_rope_def_set_stretch_stiffness);
        vm.addNativeMethod(b2RopeDefClass, "set_stretch_hertz", native_b2_rope_def_set_stretch_hertz);
        vm.addNativeMethod(b2RopeDefClass, "set_stretch_damping", native_b2_rope_def_set_stretch_damping);
        vm.addNativeMethod(b2RopeDefClass, "set_bend_stiffness", native_b2_rope_def_set_bend_stiffness);
        vm.addNativeMethod(b2RopeDefClass, "set_bend_hertz", native_b2_rope_def_set_bend_hertz);
        vm.addNativeMethod(b2RopeDefClass, "set_bend_damping", native_b2_rope_def_set_bend_damping);
        vm.addNativeMethod(b2RopeDefClass, "set_isometric", native_b2_rope_def_set_isometric);
        vm.addNativeMethod(b2RopeDefClass, "set_fixed_effective_mass", native_b2_rope_def_set_fixed_effective_mass);
        vm.addNativeMethod(b2RopeDefClass, "set_warm_start", native_b2_rope_def_set_warm_start);
        vm.addNativeMethod(b2RopeDefClass, "set_vertices", native_b2_rope_def_set_vertices);
        vm.addNativeMethod(b2RopeDefClass, "clear_vertices", native_b2_rope_def_clear_vertices);

        NativeClassDef *b2RopeClass = vm.registerNativeClass(
            kB2RopeClass,
            ctor_native_b2_rope,
            dtor_native_b2_rope,
            0,
            false);

        vm.addNativeMethod(b2RopeClass, "create", native_b2_rope_create);
        vm.addNativeMethod(b2RopeClass, "set_tuning", native_b2_rope_set_tuning);
        vm.addNativeMethod(b2RopeClass, "step", native_b2_rope_step);
        vm.addNativeMethod(b2RopeClass, "reset", native_b2_rope_reset);
 
        vm.addNativeMethod(b2RopeClass, "get_count", native_b2_rope_get_count);
        vm.addNativeMethod(b2RopeClass, "get_point", native_b2_rope_get_point);

        vm.addGlobal("b2_pbdStretchingModel", vm.makeInt((int)b2_pbdStretchingModel));
        vm.addGlobal("b2_xpbdStretchingModel", vm.makeInt((int)b2_xpbdStretchingModel));
        vm.addGlobal("b2_springAngleBendingModel", vm.makeInt((int)b2_springAngleBendingModel));
        vm.addGlobal("b2_pbdAngleBendingModel", vm.makeInt((int)b2_pbdAngleBendingModel));
        vm.addGlobal("b2_xpbdAngleBendingModel", vm.makeInt((int)b2_xpbdAngleBendingModel));
        vm.addGlobal("b2_pbdDistanceBendingModel", vm.makeInt((int)b2_pbdDistanceBendingModel));
        vm.addGlobal("b2_pbdHeightBendingModel", vm.makeInt((int)b2_pbdHeightBendingModel));
        vm.addGlobal("b2_pbdTriangleBendingModel", vm.makeInt((int)b2_pbdTriangleBendingModel));

    }

}
