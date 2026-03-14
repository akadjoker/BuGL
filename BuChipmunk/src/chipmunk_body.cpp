#include "chipmunk_common.hpp"

namespace ChipmunkBindings
{
    static int native_cpBodyNew(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpBodyNew expects (mass, moment)");
            return 0;
        }

        double mass = 0.0;
        double moment = 0.0;
        if (!read_number_arg(args[0], &mass, "cpBodyNew", 1) ||
            !read_number_arg(args[1], &moment, "cpBodyNew", 2))
            return 0;

        return push_body_handle(vm, cpBodyNew((cpFloat)mass, (cpFloat)moment)) ? 1 : 0;
    }

    static int native_cpBodyNewKinematic(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("cpBodyNewKinematic expects 0 arguments");
            return 0;
        }
        return push_body_handle(vm, cpBodyNewKinematic()) ? 1 : 0;
    }

    static int native_cpBodyNewStatic(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("cpBodyNewStatic expects 0 arguments");
            return 0;
        }
        return push_body_handle(vm, cpBodyNewStatic()) ? 1 : 0;
    }

    static int native_cpBodyFree(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpBodyFree expects (cpBody)");
            return 0;
        }

        cpBody *body = require_body_arg(vm, args[0], "cpBodyFree", 1);
        if (!body)
            return 0;
        if (cpBodyGetSpace(body) != nullptr)
        {
            Error("cpBodyFree expects body removed from cpSpace first");
            return 0;
        }

        cpBodyFree(body);
        invalidate_native_handle(args[0]);
        return 0;
    }

    static int native_cpBodyGetSpace(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpBodyGetSpace expects (cpBody)");
            return 0;
        }

        cpBody *body = require_body_arg(vm, args[0], "cpBodyGetSpace", 1);
        if (!body)
            return 0;

        cpSpace *space = cpBodyGetSpace(body);
        if (!space)
        {
            vm->pushNil();
            return 1;
        }
        return push_space_handle(vm, space) ? 1 : 0;
    }

    static int native_cpBodyGetType(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpBodyGetType expects (cpBody)");
            return 0;
        }
        cpBody *body = require_body_arg(vm, args[0], "cpBodyGetType", 1);
        if (!body)
            return 0;
        vm->pushInt((int)cpBodyGetType(body));
        return 1;
    }

    static int native_cpBodySetType(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpBodySetType expects (cpBody, type)");
            return 0;
        }
        cpBody *body = require_body_arg(vm, args[0], "cpBodySetType", 1);
        int type = 0;
        if (!body || !read_int_arg(args[1], &type, "cpBodySetType", 2))
            return 0;
        cpBodySetType(body, (cpBodyType)type);
        return 0;
    }

    static int native_cpBodyGetMass(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpBodyGetMass expects (cpBody)");
            return 0;
        }
        cpBody *body = require_body_arg(vm, args[0], "cpBodyGetMass", 1);
        if (!body)
            return 0;
        vm->pushDouble((double)cpBodyGetMass(body));
        return 1;
    }

    static int native_cpBodySetMass(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpBodySetMass expects (cpBody, mass)");
            return 0;
        }
        cpBody *body = require_body_arg(vm, args[0], "cpBodySetMass", 1);
        double mass = 0.0;
        if (!body || !read_number_arg(args[1], &mass, "cpBodySetMass", 2))
            return 0;
        cpBodySetMass(body, (cpFloat)mass);
        return 0;
    }

    static int native_cpBodyGetMoment(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpBodyGetMoment expects (cpBody)");
            return 0;
        }
        cpBody *body = require_body_arg(vm, args[0], "cpBodyGetMoment", 1);
        if (!body)
            return 0;
        vm->pushDouble((double)cpBodyGetMoment(body));
        return 1;
    }

    static int native_cpBodySetMoment(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpBodySetMoment expects (cpBody, moment)");
            return 0;
        }
        cpBody *body = require_body_arg(vm, args[0], "cpBodySetMoment", 1);
        double moment = 0.0;
        if (!body || !read_number_arg(args[1], &moment, "cpBodySetMoment", 2))
            return 0;
        cpBodySetMoment(body, (cpFloat)moment);
        return 0;
    }

    static int native_cpBodyGetPosition(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpBodyGetPosition expects (cpBody)");
            return 0;
        }
        cpBody *body = require_body_arg(vm, args[0], "cpBodyGetPosition", 1);
        if (!body)
            return 0;
        push_cpvect_components(vm, cpBodyGetPosition(body));
        return 2;
    }

    static int native_cpBodySetPosition(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpBodySetPosition expects (cpBody, Vector2)");
            return 0;
        }
        cpBody *body = require_body_arg(vm, args[0], "cpBodySetPosition", 1);
        cpVect pos;
        if (!body || !read_cpvect_arg(args[1], &pos, "cpBodySetPosition", 2))
            return 0;
        cpBodySetPosition(body, pos);
        return 0;
    }

    static int native_cpBodyGetVelocity(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpBodyGetVelocity expects (cpBody)");
            return 0;
        }
        cpBody *body = require_body_arg(vm, args[0], "cpBodyGetVelocity", 1);
        if (!body)
            return 0;
        push_cpvect_components(vm, cpBodyGetVelocity(body));
        return 2;
    }

    static int native_cpBodySetVelocity(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpBodySetVelocity expects (cpBody, Vector2)");
            return 0;
        }
        cpBody *body = require_body_arg(vm, args[0], "cpBodySetVelocity", 1);
        cpVect velocity;
        if (!body || !read_cpvect_arg(args[1], &velocity, "cpBodySetVelocity", 2))
            return 0;
        cpBodySetVelocity(body, velocity);
        return 0;
    }

    static int native_cpBodyGetAngle(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpBodyGetAngle expects (cpBody)");
            return 0;
        }
        cpBody *body = require_body_arg(vm, args[0], "cpBodyGetAngle", 1);
        if (!body)
            return 0;
        vm->pushDouble((double)cpBodyGetAngle(body));
        return 1;
    }

    static int native_cpBodySetAngle(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpBodySetAngle expects (cpBody, angle)");
            return 0;
        }
        cpBody *body = require_body_arg(vm, args[0], "cpBodySetAngle", 1);
        double angle = 0.0;
        if (!body || !read_number_arg(args[1], &angle, "cpBodySetAngle", 2))
            return 0;
        cpBodySetAngle(body, (cpFloat)angle);
        return 0;
    }

    static int native_cpBodyGetAngularVelocity(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpBodyGetAngularVelocity expects (cpBody)");
            return 0;
        }
        cpBody *body = require_body_arg(vm, args[0], "cpBodyGetAngularVelocity", 1);
        if (!body)
            return 0;
        vm->pushDouble((double)cpBodyGetAngularVelocity(body));
        return 1;
    }

    static int native_cpBodySetAngularVelocity(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpBodySetAngularVelocity expects (cpBody, omega)");
            return 0;
        }
        cpBody *body = require_body_arg(vm, args[0], "cpBodySetAngularVelocity", 1);
        double omega = 0.0;
        if (!body || !read_number_arg(args[1], &omega, "cpBodySetAngularVelocity", 2))
            return 0;
        cpBodySetAngularVelocity(body, (cpFloat)omega);
        return 0;
    }

    static int native_cpBodyGetRotation(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpBodyGetRotation expects (cpBody)");
            return 0;
        }
        cpBody *body = require_body_arg(vm, args[0], "cpBodyGetRotation", 1);
        if (!body)
            return 0;
        push_cpvect_components(vm, cpBodyGetRotation(body));
        return 2;
    }

    static int native_cpBodyApplyForceAtWorldPoint(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("cpBodyApplyForceAtWorldPoint expects (cpBody, force, point)");
            return 0;
        }
        cpBody *body = require_body_arg(vm, args[0], "cpBodyApplyForceAtWorldPoint", 1);
        cpVect force;
        cpVect point;
        if (!body ||
            !read_cpvect_arg(args[1], &force, "cpBodyApplyForceAtWorldPoint", 2) ||
            !read_cpvect_arg(args[2], &point, "cpBodyApplyForceAtWorldPoint", 3))
            return 0;
        cpBodyApplyForceAtWorldPoint(body, force, point);
        return 0;
    }

    static int native_cpBodyApplyImpulseAtWorldPoint(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("cpBodyApplyImpulseAtWorldPoint expects (cpBody, impulse, point)");
            return 0;
        }
        cpBody *body = require_body_arg(vm, args[0], "cpBodyApplyImpulseAtWorldPoint", 1);
        cpVect impulse;
        cpVect point;
        if (!body ||
            !read_cpvect_arg(args[1], &impulse, "cpBodyApplyImpulseAtWorldPoint", 2) ||
            !read_cpvect_arg(args[2], &point, "cpBodyApplyImpulseAtWorldPoint", 3))
            return 0;
        cpBodyApplyImpulseAtWorldPoint(body, impulse, point);
        return 0;
    }

    void register_body(ModuleBuilder &module, Interpreter &vm)
    {
        (void)vm;
        module.addFunction("cpBodyNew", native_cpBodyNew, 2)
            .addFunction("cpBodyNewKinematic", native_cpBodyNewKinematic, 0)
            .addFunction("cpBodyNewStatic", native_cpBodyNewStatic, 0)
            .addFunction("cpBodyFree", native_cpBodyFree, 1)
            .addFunction("cpBodyGetSpace", native_cpBodyGetSpace, 1)
            .addFunction("cpBodyGetType", native_cpBodyGetType, 1)
            .addFunction("cpBodySetType", native_cpBodySetType, 2)
            .addFunction("cpBodyGetMass", native_cpBodyGetMass, 1)
            .addFunction("cpBodySetMass", native_cpBodySetMass, 2)
            .addFunction("cpBodyGetMoment", native_cpBodyGetMoment, 1)
            .addFunction("cpBodySetMoment", native_cpBodySetMoment, 2)
            .addFunction("cpBodyGetPosition", native_cpBodyGetPosition, 1)
            .addFunction("cpBodySetPosition", native_cpBodySetPosition, 2)
            .addFunction("cpBodyGetVelocity", native_cpBodyGetVelocity, 1)
            .addFunction("cpBodySetVelocity", native_cpBodySetVelocity, 2)
            .addFunction("cpBodyGetAngle", native_cpBodyGetAngle, 1)
            .addFunction("cpBodySetAngle", native_cpBodySetAngle, 2)
            .addFunction("cpBodyGetAngularVelocity", native_cpBodyGetAngularVelocity, 1)
            .addFunction("cpBodySetAngularVelocity", native_cpBodySetAngularVelocity, 2)
            .addFunction("cpBodyGetRotation", native_cpBodyGetRotation, 1)
            .addFunction("cpBodyApplyForceAtWorldPoint", native_cpBodyApplyForceAtWorldPoint, 3)
            .addFunction("cpBodyApplyImpulseAtWorldPoint", native_cpBodyApplyImpulseAtWorldPoint, 3);
    }
}
