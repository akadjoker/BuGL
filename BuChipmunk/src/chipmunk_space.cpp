#include "chipmunk_common.hpp"

namespace ChipmunkBindings
{
    static int native_cpSpaceNew(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("cpSpaceNew expects 0 arguments");
            return 0;
        }

        return push_space_handle(vm, cpSpaceNew()) ? 1 : 0;
    }

    static int native_cpSpaceFree(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpSpaceFree expects (cpSpace)");
            return 0;
        }

        cpSpace *space = require_space_arg(vm, args[0], "cpSpaceFree", 1);
        if (!space)
            return 0;
        if (cpSpaceIsLocked(space))
        {
            Error("cpSpaceFree cannot run while space is locked");
            return 0;
        }

        cpSpaceFree(space);
        invalidate_native_handle(args[0]);
        return 0;
    }

    static int native_cpSpaceStep(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpSpaceStep expects (cpSpace, dt)");
            return 0;
        }

        cpSpace *space = require_space_arg(vm, args[0], "cpSpaceStep", 1);
        double dt = 0.0;
        if (!space || !read_number_arg(args[1], &dt, "cpSpaceStep", 2))
            return 0;

        cpSpaceStep(space, (cpFloat)dt);
        return 0;
    }

    static int native_cpSpaceIsLocked(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpSpaceIsLocked expects (cpSpace)");
            return 0;
        }

        cpSpace *space = require_space_arg(vm, args[0], "cpSpaceIsLocked", 1);
        if (!space)
            return 0;

        vm->pushBool(cpSpaceIsLocked(space) != 0);
        return 1;
    }

    static int native_cpSpaceGetIterations(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpSpaceGetIterations expects (cpSpace)");
            return 0;
        }

        cpSpace *space = require_space_arg(vm, args[0], "cpSpaceGetIterations", 1);
        if (!space)
            return 0;

        vm->pushInt(cpSpaceGetIterations(space));
        return 1;
    }

    static int native_cpSpaceSetIterations(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpSpaceSetIterations expects (cpSpace, iterations)");
            return 0;
        }

        cpSpace *space = require_space_arg(vm, args[0], "cpSpaceSetIterations", 1);
        int iterations = 0;
        if (!space || !read_int_arg(args[1], &iterations, "cpSpaceSetIterations", 2))
            return 0;

        cpSpaceSetIterations(space, iterations);
        return 0;
    }

    static int native_cpSpaceGetGravity(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpSpaceGetGravity expects (cpSpace)");
            return 0;
        }

        cpSpace *space = require_space_arg(vm, args[0], "cpSpaceGetGravity", 1);
        if (!space)
            return 0;

        push_cpvect_components(vm, cpSpaceGetGravity(space));
        return 2;
    }

    static int native_cpSpaceSetGravity(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpSpaceSetGravity expects (cpSpace, Vector2)");
            return 0;
        }

        cpSpace *space = require_space_arg(vm, args[0], "cpSpaceSetGravity", 1);
        cpVect gravity;
        if (!space || !read_cpvect_arg(args[1], &gravity, "cpSpaceSetGravity", 2))
            return 0;

        cpSpaceSetGravity(space, gravity);
        return 0;
    }

    static int native_cpSpaceGetDamping(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpSpaceGetDamping expects (cpSpace)");
            return 0;
        }

        cpSpace *space = require_space_arg(vm, args[0], "cpSpaceGetDamping", 1);
        if (!space)
            return 0;

        vm->pushDouble((double)cpSpaceGetDamping(space));
        return 1;
    }

    static int native_cpSpaceSetDamping(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpSpaceSetDamping expects (cpSpace, damping)");
            return 0;
        }

        cpSpace *space = require_space_arg(vm, args[0], "cpSpaceSetDamping", 1);
        double damping = 0.0;
        if (!space || !read_number_arg(args[1], &damping, "cpSpaceSetDamping", 2))
            return 0;

        cpSpaceSetDamping(space, (cpFloat)damping);
        return 0;
    }

    static int native_cpSpaceGetStaticBody(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpSpaceGetStaticBody expects (cpSpace)");
            return 0;
        }

        cpSpace *space = require_space_arg(vm, args[0], "cpSpaceGetStaticBody", 1);
        if (!space)
            return 0;

        return push_body_handle(vm, cpSpaceGetStaticBody(space)) ? 1 : 0;
    }

    static int native_cpSpaceAddBody(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpSpaceAddBody expects (cpSpace, cpBody)");
            return 0;
        }

        cpSpace *space = require_space_arg(vm, args[0], "cpSpaceAddBody", 1);
        cpBody *body = require_body_arg(vm, args[1], "cpSpaceAddBody", 2);
        if (!space || !body)
            return 0;
        if (cpSpaceIsLocked(space))
        {
            Error("cpSpaceAddBody cannot run while space is locked");
            return 0;
        }
        if (cpBodyGetSpace(body) == space)
        {
            vm->push(args[1]);
            return 1;
        }

        return push_body_handle(vm, cpSpaceAddBody(space, body)) ? 1 : 0;
    }

    static int native_cpSpaceRemoveBody(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpSpaceRemoveBody expects (cpSpace, cpBody)");
            return 0;
        }

        cpSpace *space = require_space_arg(vm, args[0], "cpSpaceRemoveBody", 1);
        cpBody *body = require_body_arg(vm, args[1], "cpSpaceRemoveBody", 2);
        if (!space || !body)
            return 0;
        if (cpSpaceIsLocked(space))
        {
            Error("cpSpaceRemoveBody cannot run while space is locked");
            return 0;
        }
        if (cpSpaceContainsBody(space, body))
            cpSpaceRemoveBody(space, body);
        return 0;
    }

    static int native_cpSpaceContainsBody(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpSpaceContainsBody expects (cpSpace, cpBody)");
            return 0;
        }

        cpSpace *space = require_space_arg(vm, args[0], "cpSpaceContainsBody", 1);
        cpBody *body = require_body_arg(vm, args[1], "cpSpaceContainsBody", 2);
        if (!space || !body)
            return 0;

        vm->pushBool(cpSpaceContainsBody(space, body) != 0);
        return 1;
    }

    static int native_cpSpaceAddShape(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpSpaceAddShape expects (cpSpace, cpShape)");
            return 0;
        }

        cpSpace *space = require_space_arg(vm, args[0], "cpSpaceAddShape", 1);
        cpShape *shape = require_shape_arg(vm, args[1], "cpSpaceAddShape", 2);
        if (!space || !shape)
            return 0;
        if (cpSpaceIsLocked(space))
        {
            Error("cpSpaceAddShape cannot run while space is locked");
            return 0;
        }
        if (cpShapeGetSpace(shape) == space)
        {
            vm->push(args[1]);
            return 1;
        }

        return push_shape_handle(vm, cpSpaceAddShape(space, shape)) ? 1 : 0;
    }

    static int native_cpSpaceRemoveShape(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpSpaceRemoveShape expects (cpSpace, cpShape)");
            return 0;
        }

        cpSpace *space = require_space_arg(vm, args[0], "cpSpaceRemoveShape", 1);
        cpShape *shape = require_shape_arg(vm, args[1], "cpSpaceRemoveShape", 2);
        if (!space || !shape)
            return 0;
        if (cpSpaceIsLocked(space))
        {
            Error("cpSpaceRemoveShape cannot run while space is locked");
            return 0;
        }
        if (cpSpaceContainsShape(space, shape))
            cpSpaceRemoveShape(space, shape);
        return 0;
    }

    static int native_cpSpaceContainsShape(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpSpaceContainsShape expects (cpSpace, cpShape)");
            return 0;
        }

        cpSpace *space = require_space_arg(vm, args[0], "cpSpaceContainsShape", 1);
        cpShape *shape = require_shape_arg(vm, args[1], "cpSpaceContainsShape", 2);
        if (!space || !shape)
            return 0;

        vm->pushBool(cpSpaceContainsShape(space, shape) != 0);
        return 1;
    }

    static int native_cpSpaceAddConstraint(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpSpaceAddConstraint expects (cpSpace, cpConstraint)");
            return 0;
        }

        cpSpace *space = require_space_arg(vm, args[0], "cpSpaceAddConstraint", 1);
        cpConstraint *constraint = require_constraint_arg(vm, args[1], "cpSpaceAddConstraint", 2);
        if (!space || !constraint)
            return 0;
        if (cpSpaceIsLocked(space))
        {
            Error("cpSpaceAddConstraint cannot run while space is locked");
            return 0;
        }
        if (cpConstraintGetSpace(constraint) == space)
        {
            vm->push(args[1]);
            return 1;
        }

        return push_constraint_handle(vm, cpSpaceAddConstraint(space, constraint)) ? 1 : 0;
    }

    static int native_cpSpaceRemoveConstraint(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpSpaceRemoveConstraint expects (cpSpace, cpConstraint)");
            return 0;
        }

        cpSpace *space = require_space_arg(vm, args[0], "cpSpaceRemoveConstraint", 1);
        cpConstraint *constraint = require_constraint_arg(vm, args[1], "cpSpaceRemoveConstraint", 2);
        if (!space || !constraint)
            return 0;
        if (cpSpaceIsLocked(space))
        {
            Error("cpSpaceRemoveConstraint cannot run while space is locked");
            return 0;
        }
        if (cpSpaceContainsConstraint(space, constraint))
            cpSpaceRemoveConstraint(space, constraint);
        return 0;
    }

    static int native_cpSpaceContainsConstraint(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpSpaceContainsConstraint expects (cpSpace, cpConstraint)");
            return 0;
        }

        cpSpace *space = require_space_arg(vm, args[0], "cpSpaceContainsConstraint", 1);
        cpConstraint *constraint = require_constraint_arg(vm, args[1], "cpSpaceContainsConstraint", 2);
        if (!space || !constraint)
            return 0;

        vm->pushBool(cpSpaceContainsConstraint(space, constraint) != 0);
        return 1;
    }

    void register_space(ModuleBuilder &module, Interpreter &vm)
    {
        (void)vm;
        module.addFunction("cpSpaceNew", native_cpSpaceNew, 0)
            .addFunction("cpSpaceFree", native_cpSpaceFree, 1)
            .addFunction("cpSpaceStep", native_cpSpaceStep, 2)
            .addFunction("cpSpaceIsLocked", native_cpSpaceIsLocked, 1)
            .addFunction("cpSpaceGetIterations", native_cpSpaceGetIterations, 1)
            .addFunction("cpSpaceSetIterations", native_cpSpaceSetIterations, 2)
            .addFunction("cpSpaceGetGravity", native_cpSpaceGetGravity, 1)
            .addFunction("cpSpaceSetGravity", native_cpSpaceSetGravity, 2)
            .addFunction("cpSpaceGetDamping", native_cpSpaceGetDamping, 1)
            .addFunction("cpSpaceSetDamping", native_cpSpaceSetDamping, 2)
            .addFunction("cpSpaceGetStaticBody", native_cpSpaceGetStaticBody, 1)
            .addFunction("cpSpaceAddBody", native_cpSpaceAddBody, 2)
            .addFunction("cpSpaceRemoveBody", native_cpSpaceRemoveBody, 2)
            .addFunction("cpSpaceContainsBody", native_cpSpaceContainsBody, 2)
            .addFunction("cpSpaceAddShape", native_cpSpaceAddShape, 2)
            .addFunction("cpSpaceRemoveShape", native_cpSpaceRemoveShape, 2)
            .addFunction("cpSpaceContainsShape", native_cpSpaceContainsShape, 2)
            .addFunction("cpSpaceAddConstraint", native_cpSpaceAddConstraint, 2)
            .addFunction("cpSpaceRemoveConstraint", native_cpSpaceRemoveConstraint, 2)
            .addFunction("cpSpaceContainsConstraint", native_cpSpaceContainsConstraint, 2);
    }
}
