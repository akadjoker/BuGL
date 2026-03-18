#include "jolt_core.hpp"
#include <Jolt/Physics/Body/BodyLock.h>

namespace JoltBindings
{
    static void *jolt_body_ctor_error(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)argCount;
        (void)args;
        Error("JoltBody cannot be constructed directly; use JoltWorld.createBox/createSphere/createStaticBox");
        return nullptr;
    }

    static void jolt_body_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        JoltBodyHandle *body = (JoltBodyHandle *)instance;
        if (!body)
            return;

        body->wrapperAlive = false;

        if (body->world == nullptr || !body->valid)
            delete body;
    }

    static int jolt_body_destroy(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("JoltBody.destroy() expects 0 arguments");
            return 0;
        }

        destroy_body_runtime((JoltBodyHandle *)instance);
        return 0;
    }

    static int jolt_body_is_valid(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltBody.isValid() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltBodyHandle *body = (JoltBodyHandle *)instance;
        vm->pushBool(body != nullptr && body->valid && body->world != nullptr && body->world->valid);
        return 1;
    }

    static int jolt_body_is_active(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltBody.isActive() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.isActive()");
        if (!body)
            return push_nil1(vm);

        vm->pushBool(body->world->physicsSystem.GetBodyInterface().IsActive(body->id));
        return 1;
    }

    static int jolt_body_activate(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltBody.activate() expects 0 arguments");
            return 0;
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.activate()");
        if (!body)
            return 0;

        body->world->physicsSystem.GetBodyInterface().ActivateBody(body->id);
        return 0;
    }

    static int jolt_body_deactivate(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltBody.deactivate() expects 0 arguments");
            return 0;
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.deactivate()");
        if (!body)
            return 0;

        body->world->physicsSystem.GetBodyInterface().DeactivateBody(body->id);
        return 0;
    }

    static int jolt_body_get_position(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltBody.getPosition() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.getPosition()");
        if (!body)
            return push_nil1(vm);

        return push_vector3(vm,
                            from_jolt_rvec3(body->world->physicsSystem.GetBodyInterface().GetCenterOfMassPosition(body->id)))
                   ? 1
                   : 0;
    }

    static int jolt_body_get_rotation(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltBody.getRotation() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.getRotation()");
        if (!body)
            return push_nil1(vm);

        return push_quaternion(vm,
                               from_jolt_quat(body->world->physicsSystem.GetBodyInterface().GetRotation(body->id)))
                   ? 1
                   : 0;
    }

    static int jolt_body_get_linear_velocity(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltBody.getLinearVelocity() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.getLinearVelocity()");
        if (!body)
            return push_nil1(vm);

        return push_vector3(vm,
                            from_jolt_vec3(body->world->physicsSystem.GetBodyInterface().GetLinearVelocity(body->id)))
                   ? 1
                   : 0;
    }

    static int jolt_body_get_motion_type(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltBody.getMotionType() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.getMotionType()");
        if (!body)
            return push_nil1(vm);

        vm->pushInt((int)body->world->physicsSystem.GetBodyInterface().GetMotionType(body->id));
        return 1;
    }

    static int jolt_body_get_angular_velocity(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltBody.getAngularVelocity() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.getAngularVelocity()");
        if (!body)
            return push_nil1(vm);

        return push_vector3(vm,
                            from_jolt_vec3(body->world->physicsSystem.GetBodyInterface().GetAngularVelocity(body->id)))
                   ? 1
                   : 0;
    }

    static int jolt_body_set_linear_velocity(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltBody.setLinearVelocity() expects (Vector3)");
            return 0;
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.setLinearVelocity()");
        if (!body)
            return 0;

        Vector3 velocity;
        if (!read_vector3_arg(args[0], &velocity, "JoltBody.setLinearVelocity()", 1))
            return 0;

        body->world->physicsSystem.GetBodyInterface().SetLinearVelocity(body->id, to_jolt_vec3(velocity));
        return 0;
    }

    static int jolt_body_set_position(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltBody.setPosition() expects (Vector3)");
            return 0;
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.setPosition()");
        if (!body)
            return 0;

        Vector3 position;
        if (!read_vector3_arg(args[0], &position, "JoltBody.setPosition()", 1))
            return 0;

        body->world->physicsSystem.GetBodyInterface().SetPosition(body->id,
                                                                  to_jolt_rvec3(position),
                                                                  EActivation::Activate);
        return 0;
    }

    static int jolt_body_set_angular_velocity(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltBody.setAngularVelocity() expects (Vector3)");
            return 0;
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.setAngularVelocity()");
        if (!body)
            return 0;

        Vector3 velocity;
        if (!read_vector3_arg(args[0], &velocity, "JoltBody.setAngularVelocity()", 1))
            return 0;

        body->world->physicsSystem.GetBodyInterface().SetAngularVelocity(body->id, to_jolt_vec3(velocity));
        return 0;
    }

    static int jolt_body_set_rotation(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltBody.setRotation() expects (Quaternion)");
            return 0;
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.setRotation()");
        if (!body)
            return 0;

        Quaternion rotation;
        if (!read_quaternion_arg(args[0], &rotation, "JoltBody.setRotation()", 1))
            return 0;

        body->world->physicsSystem.GetBodyInterface().SetRotation(body->id,
                                                                  Quat(rotation.x, rotation.y, rotation.z, rotation.w),
                                                                  EActivation::DontActivate);
        return 0;
    }

    static int jolt_body_set_position_and_rotation(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("JoltBody.setPositionAndRotation() expects (Vector3, Quaternion)");
            return 0;
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.setPositionAndRotation()");
        if (!body)
            return 0;

        Vector3 position;
        Quaternion rotation;
        if (!read_vector3_arg(args[0], &position, "JoltBody.setPositionAndRotation()", 1) ||
            !read_quaternion_arg(args[1], &rotation, "JoltBody.setPositionAndRotation()", 2))
            return 0;

        body->world->physicsSystem.GetBodyInterface().SetPositionAndRotation(body->id,
                                                                             to_jolt_rvec3(position),
                                                                             Quat(rotation.x, rotation.y, rotation.z, rotation.w),
                                                                             EActivation::Activate);
        return 0;
    }

    static int jolt_body_move_kinematic(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 3)
        {
            Error("JoltBody.moveKinematic() expects (Vector3, Quaternion, number)");
            return 0;
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.moveKinematic()");
        if (!body)
            return 0;

        Vector3 position;
        Quaternion rotation;
        double delta_time = 0.0;
        if (!read_vector3_arg(args[0], &position, "JoltBody.moveKinematic()", 1) ||
            !read_quaternion_arg(args[1], &rotation, "JoltBody.moveKinematic()", 2) ||
            !read_number_arg(args[2], &delta_time, "JoltBody.moveKinematic()", 3))
            return 0;

        body->world->physicsSystem.GetBodyInterface().MoveKinematic(body->id,
                                                                    to_jolt_rvec3(position),
                                                                    Quat(rotation.x, rotation.y, rotation.z, rotation.w),
                                                                    (float)delta_time);
        return 0;
    }

    static int jolt_body_set_motion_type(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltBody.setMotionType() expects (int)");
            return 0;
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.setMotionType()");
        if (!body)
            return 0;

        int motion_type = 0;
        if (!read_int_arg(args[0], &motion_type, "JoltBody.setMotionType()", 1))
            return 0;

        body->world->physicsSystem.GetBodyInterface().SetMotionType(body->id,
                                                                    (EMotionType)motion_type,
                                                                    EActivation::Activate);
        return 0;
    }

    static int jolt_body_add_impulse(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltBody.addImpulse() expects (Vector3)");
            return 0;
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.addImpulse()");
        if (!body)
            return 0;

        Vector3 impulse;
        if (!read_vector3_arg(args[0], &impulse, "JoltBody.addImpulse()", 1))
            return 0;

        body->world->physicsSystem.GetBodyInterface().AddImpulse(body->id, to_jolt_vec3(impulse));
        return 0;
    }

    static int jolt_body_add_impulse_at_position(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("JoltBody.addImpulseAtPosition() expects (Vector3 impulse, Vector3 position)");
            return 0;
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.addImpulseAtPosition()");
        if (!body)
            return 0;

        Vector3 impulse;
        Vector3 position;
        if (!read_vector3_arg(args[0], &impulse, "JoltBody.addImpulseAtPosition()", 1) ||
            !read_vector3_arg(args[1], &position, "JoltBody.addImpulseAtPosition()", 2))
            return 0;

        body->world->physicsSystem.GetBodyInterface().AddImpulse(body->id,
                                                                 to_jolt_vec3(impulse),
                                                                 to_jolt_rvec3(position));
        return 0;
    }

    static int jolt_body_add_force(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltBody.addForce() expects (Vector3)");
            return 0;
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.addForce()");
        if (!body)
            return 0;

        Vector3 force;
        if (!read_vector3_arg(args[0], &force, "JoltBody.addForce()", 1))
            return 0;

        body->world->physicsSystem.GetBodyInterface().AddForce(body->id, to_jolt_vec3(force));
        return 0;
    }

    static int jolt_body_add_force_at_position(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("JoltBody.addForceAtPosition() expects (Vector3 force, Vector3 position)");
            return 0;
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.addForceAtPosition()");
        if (!body)
            return 0;

        Vector3 force;
        Vector3 position;
        if (!read_vector3_arg(args[0], &force, "JoltBody.addForceAtPosition()", 1) ||
            !read_vector3_arg(args[1], &position, "JoltBody.addForceAtPosition()", 2))
            return 0;

        body->world->physicsSystem.GetBodyInterface().AddForce(body->id,
                                                               to_jolt_vec3(force),
                                                               to_jolt_rvec3(position));
        return 0;
    }

    static int jolt_body_add_torque(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltBody.addTorque() expects (Vector3)");
            return 0;
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.addTorque()");
        if (!body)
            return 0;

        Vector3 torque;
        if (!read_vector3_arg(args[0], &torque, "JoltBody.addTorque()", 1))
            return 0;

        body->world->physicsSystem.GetBodyInterface().AddTorque(body->id, to_jolt_vec3(torque));
        return 0;
    }

    static int jolt_body_add_angular_impulse(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltBody.addAngularImpulse() expects (Vector3)");
            return 0;
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.addAngularImpulse()");
        if (!body)
            return 0;

        Vector3 impulse;
        if (!read_vector3_arg(args[0], &impulse, "JoltBody.addAngularImpulse()", 1))
            return 0;

        body->world->physicsSystem.GetBodyInterface().AddAngularImpulse(body->id, to_jolt_vec3(impulse));
        return 0;
    }

    static int jolt_body_set_friction(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltBody.setFriction() expects (number)");
            return 0;
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.setFriction()");
        if (!body)
            return 0;

        double friction = 0.0;
        if (!read_number_arg(args[0], &friction, "JoltBody.setFriction()", 1))
            return 0;

        body->world->physicsSystem.GetBodyInterface().SetFriction(body->id, (float)friction);
        return 0;
    }

    static int jolt_body_set_restitution(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltBody.setRestitution() expects (number)");
            return 0;
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.setRestitution()");
        if (!body)
            return 0;

        double restitution = 0.0;
        if (!read_number_arg(args[0], &restitution, "JoltBody.setRestitution()", 1))
            return 0;

        body->world->physicsSystem.GetBodyInterface().SetRestitution(body->id, (float)restitution);
        return 0;
    }

    // ─── Body property getters ────────────────────────────────────────────

    static int jolt_body_get_friction(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltBody.getFriction() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.getFriction()");
        if (!body)
            return push_nil1(vm);

        vm->pushFloat(body->world->physicsSystem.GetBodyInterface().GetFriction(body->id));
        return 1;
    }

    static int jolt_body_get_restitution(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltBody.getRestitution() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.getRestitution()");
        if (!body)
            return push_nil1(vm);

        vm->pushFloat(body->world->physicsSystem.GetBodyInterface().GetRestitution(body->id));
        return 1;
    }

    static int jolt_body_get_inverse_mass(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltBody.getInverseMass() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.getInverseMass()");
        if (!body)
            return push_nil1(vm);

        BodyLockRead lock(body->world->physicsSystem.GetBodyLockInterface(), body->id);
        if (!lock.Succeeded())
            return push_nil1(vm);

        const Body &b = lock.GetBody();
        if (!b.IsDynamic())
        {
            vm->pushFloat(0.0f);
            return 1;
        }

        vm->pushFloat(b.GetMotionProperties()->GetInverseMass());
        return 1;
    }

    // ─── Gravity factor ──────────────────────────────────────────────────

    static int jolt_body_set_gravity_factor(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltBody.setGravityFactor() expects (number)");
            return 0;
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.setGravityFactor()");
        if (!body)
            return 0;

        double factor = 0.0;
        if (!read_number_arg(args[0], &factor, "JoltBody.setGravityFactor()", 1))
            return 0;

        body->world->physicsSystem.GetBodyInterface().SetGravityFactor(body->id, (float)factor);
        return 0;
    }

    static int jolt_body_get_gravity_factor(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltBody.getGravityFactor() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.getGravityFactor()");
        if (!body)
            return push_nil1(vm);

        vm->pushFloat(body->world->physicsSystem.GetBodyInterface().GetGravityFactor(body->id));
        return 1;
    }

    // ─── Linear & angular damping ────────────────────────────────────────

    static int jolt_body_set_linear_velocity_clamped(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("JoltBody.setLinearVelocityClamped() expects (Vector3)");
            return 0;
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.setLinearVelocityClamped()");
        if (!body)
            return 0;

        Vector3 velocity;
        if (!read_vector3_arg(args[0], &velocity, "JoltBody.setLinearVelocityClamped()", 1))
            return 0;

        body->world->physicsSystem.GetBodyInterface().SetLinearAndAngularVelocity(
            body->id,
            to_jolt_vec3(velocity),
            body->world->physicsSystem.GetBodyInterface().GetAngularVelocity(body->id));
        return 0;
    }

    static int jolt_body_get_world_transform(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("JoltBody.getWorldTransform() expects 0 arguments");
            return push_nil1(vm);
        }

        JoltBodyHandle *body = require_body(instance, "JoltBody.getWorldTransform()");
        if (!body)
            return push_nil1(vm);

        BodyInterface &bi = body->world->physicsSystem.GetBodyInterface();
        RVec3 pos = bi.GetCenterOfMassPosition(body->id);
        Quat rot = bi.GetRotation(body->id);

        if (!push_vector3(vm, from_jolt_rvec3(pos)))
            return push_nil1(vm);
        if (!push_quaternion(vm, from_jolt_quat(rot)))
            return 0; // 1 already pushed
        return 2;
    }

    void register_jolt_body(Interpreter &vm)
    {
        g_bodyClass = vm.registerNativeClass("JoltBody", jolt_body_ctor_error, jolt_body_dtor, 0, false);

        vm.addNativeMethod(g_bodyClass, "destroy", jolt_body_destroy);
        vm.addNativeMethod(g_bodyClass, "isValid", jolt_body_is_valid);
        vm.addNativeMethod(g_bodyClass, "isActive", jolt_body_is_active);
        vm.addNativeMethod(g_bodyClass, "activate", jolt_body_activate);
        vm.addNativeMethod(g_bodyClass, "deactivate", jolt_body_deactivate);
        vm.addNativeMethod(g_bodyClass, "getPosition", jolt_body_get_position);
        vm.addNativeMethod(g_bodyClass, "getRotation", jolt_body_get_rotation);
        vm.addNativeMethod(g_bodyClass, "getMotionType", jolt_body_get_motion_type);
        vm.addNativeMethod(g_bodyClass, "getLinearVelocity", jolt_body_get_linear_velocity);
        vm.addNativeMethod(g_bodyClass, "getAngularVelocity", jolt_body_get_angular_velocity);
        vm.addNativeMethod(g_bodyClass, "setPosition", jolt_body_set_position);
        vm.addNativeMethod(g_bodyClass, "setLinearVelocity", jolt_body_set_linear_velocity);
        vm.addNativeMethod(g_bodyClass, "setAngularVelocity", jolt_body_set_angular_velocity);
        vm.addNativeMethod(g_bodyClass, "setMotionType", jolt_body_set_motion_type);
        vm.addNativeMethod(g_bodyClass, "setRotation", jolt_body_set_rotation);
        vm.addNativeMethod(g_bodyClass, "setPositionAndRotation", jolt_body_set_position_and_rotation);
        vm.addNativeMethod(g_bodyClass, "moveKinematic", jolt_body_move_kinematic);
        vm.addNativeMethod(g_bodyClass, "addForce", jolt_body_add_force);
        vm.addNativeMethod(g_bodyClass, "addForceAtPosition", jolt_body_add_force_at_position);
        vm.addNativeMethod(g_bodyClass, "addTorque", jolt_body_add_torque);
        vm.addNativeMethod(g_bodyClass, "addImpulse", jolt_body_add_impulse);
        vm.addNativeMethod(g_bodyClass, "addImpulseAtPosition", jolt_body_add_impulse_at_position);
        vm.addNativeMethod(g_bodyClass, "addAngularImpulse", jolt_body_add_angular_impulse);
        vm.addNativeMethod(g_bodyClass, "setFriction", jolt_body_set_friction);
        vm.addNativeMethod(g_bodyClass, "setRestitution", jolt_body_set_restitution);
        vm.addNativeMethod(g_bodyClass, "getFriction", jolt_body_get_friction);
        vm.addNativeMethod(g_bodyClass, "getRestitution", jolt_body_get_restitution);
        vm.addNativeMethod(g_bodyClass, "getInverseMass", jolt_body_get_inverse_mass);
        vm.addNativeMethod(g_bodyClass, "setGravityFactor", jolt_body_set_gravity_factor);
        vm.addNativeMethod(g_bodyClass, "getGravityFactor", jolt_body_get_gravity_factor);
        vm.addNativeMethod(g_bodyClass, "setLinearVelocityClamped", jolt_body_set_linear_velocity_clamped);
        vm.addNativeMethod(g_bodyClass, "getWorldTransform", jolt_body_get_world_transform);
    }
}
