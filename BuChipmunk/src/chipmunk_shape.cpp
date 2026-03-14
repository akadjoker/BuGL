#include "chipmunk_common.hpp"

namespace ChipmunkBindings
{
    static int native_cpShapeFree(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpShapeFree expects (cpShape)");
            return 0;
        }

        cpShape *shape = require_shape_arg(vm, args[0], "cpShapeFree", 1);
        if (!shape)
            return 0;
        if (cpShapeGetSpace(shape) != nullptr)
        {
            Error("cpShapeFree expects shape removed from cpSpace first");
            return 0;
        }

        cpShapeFree(shape);
        invalidate_native_handle(args[0]);
        return 0;
    }

    static int native_cpShapeGetSpace(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpShapeGetSpace expects (cpShape)");
            return 0;
        }

        cpShape *shape = require_shape_arg(vm, args[0], "cpShapeGetSpace", 1);
        if (!shape)
            return 0;

        cpSpace *space = cpShapeGetSpace(shape);
        if (!space)
        {
            vm->pushNil();
            return 1;
        }
        return push_space_handle(vm, space) ? 1 : 0;
    }

    static int native_cpShapeGetBody(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpShapeGetBody expects (cpShape)");
            return 0;
        }

        cpShape *shape = require_shape_arg(vm, args[0], "cpShapeGetBody", 1);
        if (!shape)
            return 0;

        cpBody *body = cpShapeGetBody(shape);
        if (!body)
        {
            vm->pushNil();
            return 1;
        }
        return push_body_handle(vm, body) ? 1 : 0;
    }

    static int native_cpShapeGetSensor(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpShapeGetSensor expects (cpShape)");
            return 0;
        }
        cpShape *shape = require_shape_arg(vm, args[0], "cpShapeGetSensor", 1);
        if (!shape)
            return 0;
        vm->pushBool(cpShapeGetSensor(shape) != 0);
        return 1;
    }

    static int native_cpShapeSetSensor(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2 || !args[1].isBool())
        {
            Error("cpShapeSetSensor expects (cpShape, bool)");
            return 0;
        }
        cpShape *shape = require_shape_arg(vm, args[0], "cpShapeSetSensor", 1);
        if (!shape)
            return 0;
        cpShapeSetSensor(shape, args[1].asBool() ? cpTrue : cpFalse);
        return 0;
    }

    static int native_cpShapeGetElasticity(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpShapeGetElasticity expects (cpShape)");
            return 0;
        }
        cpShape *shape = require_shape_arg(vm, args[0], "cpShapeGetElasticity", 1);
        if (!shape)
            return 0;
        vm->pushDouble((double)cpShapeGetElasticity(shape));
        return 1;
    }

    static int native_cpShapeSetElasticity(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpShapeSetElasticity expects (cpShape, elasticity)");
            return 0;
        }
        cpShape *shape = require_shape_arg(vm, args[0], "cpShapeSetElasticity", 1);
        double value = 0.0;
        if (!shape || !read_number_arg(args[1], &value, "cpShapeSetElasticity", 2))
            return 0;
        cpShapeSetElasticity(shape, (cpFloat)value);
        return 0;
    }

    static int native_cpShapeGetFriction(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpShapeGetFriction expects (cpShape)");
            return 0;
        }
        cpShape *shape = require_shape_arg(vm, args[0], "cpShapeGetFriction", 1);
        if (!shape)
            return 0;
        vm->pushDouble((double)cpShapeGetFriction(shape));
        return 1;
    }

    static int native_cpShapeSetFriction(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpShapeSetFriction expects (cpShape, friction)");
            return 0;
        }
        cpShape *shape = require_shape_arg(vm, args[0], "cpShapeSetFriction", 1);
        double value = 0.0;
        if (!shape || !read_number_arg(args[1], &value, "cpShapeSetFriction", 2))
            return 0;
        cpShapeSetFriction(shape, (cpFloat)value);
        return 0;
    }

    static int native_cpShapeGetCollisionType(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpShapeGetCollisionType expects (cpShape)");
            return 0;
        }
        cpShape *shape = require_shape_arg(vm, args[0], "cpShapeGetCollisionType", 1);
        if (!shape)
            return 0;
        vm->pushInt((int)cpShapeGetCollisionType(shape));
        return 1;
    }

    static int native_cpShapeSetCollisionType(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpShapeSetCollisionType expects (cpShape, collisionType)");
            return 0;
        }
        cpShape *shape = require_shape_arg(vm, args[0], "cpShapeSetCollisionType", 1);
        int value = 0;
        if (!shape || !read_int_arg(args[1], &value, "cpShapeSetCollisionType", 2))
            return 0;
        cpShapeSetCollisionType(shape, (cpCollisionType)value);
        return 0;
    }

    static int native_cpCircleShapeNew(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("cpCircleShapeNew expects (cpBody, radius, Vector2)");
            return 0;
        }
        cpBody *body = require_body_arg(vm, args[0], "cpCircleShapeNew", 1);
        double radius = 0.0;
        cpVect offset;
        if (!body ||
            !read_number_arg(args[1], &radius, "cpCircleShapeNew", 2) ||
            !read_cpvect_arg(args[2], &offset, "cpCircleShapeNew", 3))
            return 0;
        return push_shape_handle(vm, cpCircleShapeNew(body, (cpFloat)radius, offset)) ? 1 : 0;
    }

    static int native_cpSegmentShapeNew(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("cpSegmentShapeNew expects (cpBody, a, b, radius)");
            return 0;
        }
        cpBody *body = require_body_arg(vm, args[0], "cpSegmentShapeNew", 1);
        cpVect a;
        cpVect b;
        double radius = 0.0;
        if (!body ||
            !read_cpvect_arg(args[1], &a, "cpSegmentShapeNew", 2) ||
            !read_cpvect_arg(args[2], &b, "cpSegmentShapeNew", 3) ||
            !read_number_arg(args[3], &radius, "cpSegmentShapeNew", 4))
            return 0;
        return push_shape_handle(vm, cpSegmentShapeNew(body, a, b, (cpFloat)radius)) ? 1 : 0;
    }

    static int native_cpPolyShapeNewRaw(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("cpPolyShapeNewRaw expects (cpBody, verts, radius)");
            return 0;
        }
        cpBody *body = require_body_arg(vm, args[0], "cpPolyShapeNewRaw", 1);
        cpVect *verts = nullptr;
        int count = 0;
        double radius = 0.0;
        if (!body ||
            !read_cpvect_array_arg(args[1], &verts, &count, "cpPolyShapeNewRaw", 2) ||
            !read_number_arg(args[2], &radius, "cpPolyShapeNewRaw", 3))
        {
            free_cpvect_array(verts);
            return 0;
        }

        cpShape *shape = cpPolyShapeNewRaw(body, count, verts, (cpFloat)radius);
        free_cpvect_array(verts);
        return push_shape_handle(vm, shape) ? 1 : 0;
    }

    static int native_cpBoxShapeNew(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("cpBoxShapeNew expects (cpBody, width, height, radius)");
            return 0;
        }
        cpBody *body = require_body_arg(vm, args[0], "cpBoxShapeNew", 1);
        double width = 0.0;
        double height = 0.0;
        double radius = 0.0;
        if (!body ||
            !read_number_arg(args[1], &width, "cpBoxShapeNew", 2) ||
            !read_number_arg(args[2], &height, "cpBoxShapeNew", 3) ||
            !read_number_arg(args[3], &radius, "cpBoxShapeNew", 4))
            return 0;
        return push_shape_handle(vm, cpBoxShapeNew(body, (cpFloat)width, (cpFloat)height, (cpFloat)radius)) ? 1 : 0;
    }

    void register_shape(ModuleBuilder &module, Interpreter &vm)
    {
        (void)vm;
        module.addFunction("cpShapeFree", native_cpShapeFree, 1)
            .addFunction("cpShapeGetSpace", native_cpShapeGetSpace, 1)
            .addFunction("cpShapeGetBody", native_cpShapeGetBody, 1)
            .addFunction("cpShapeGetSensor", native_cpShapeGetSensor, 1)
            .addFunction("cpShapeSetSensor", native_cpShapeSetSensor, 2)
            .addFunction("cpShapeGetElasticity", native_cpShapeGetElasticity, 1)
            .addFunction("cpShapeSetElasticity", native_cpShapeSetElasticity, 2)
            .addFunction("cpShapeGetFriction", native_cpShapeGetFriction, 1)
            .addFunction("cpShapeSetFriction", native_cpShapeSetFriction, 2)
            .addFunction("cpShapeGetCollisionType", native_cpShapeGetCollisionType, 1)
            .addFunction("cpShapeSetCollisionType", native_cpShapeSetCollisionType, 2)
            .addFunction("cpCircleShapeNew", native_cpCircleShapeNew, 3)
            .addFunction("cpSegmentShapeNew", native_cpSegmentShapeNew, 4)
            .addFunction("cpPolyShapeNewRaw", native_cpPolyShapeNewRaw, 3)
            .addFunction("cpBoxShapeNew", native_cpBoxShapeNew, 4);
    }
}
