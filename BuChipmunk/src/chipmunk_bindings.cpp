#include "chipmunk_common.hpp"

namespace ChipmunkBindings
{
    NativeClassDef *g_cpSpaceClass = nullptr;
    NativeClassDef *g_cpBodyClass = nullptr;
    NativeClassDef *g_cpShapeClass = nullptr;
    NativeClassDef *g_cpConstraintClass = nullptr;
    NativeStructDef *g_vector2Def = nullptr;

    static void *chipmunk_handle_ctor_error(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        (void)argCount;
        (void)args;
        Error("Chipmunk handles cannot be constructed directly; use cp* factory functions");
        return nullptr;
    }

    static void chipmunk_handle_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        (void)instance;
    }

    static NativeStructDef *get_native_struct_def(Interpreter *vm, const char *name)
    {
        if (!vm || !name)
            return nullptr;

        Value structValue;
        if (!vm->tryGetGlobal(name, &structValue) || !structValue.isNativeStruct())
            return nullptr;

        Value instanceValue = vm->createNativeStruct(structValue.asNativeStructId(), 0, nullptr);
        NativeStructInstance *inst = instanceValue.asNativeStructInstance();
        return inst ? inst->def : nullptr;
    }

    bool read_number_arg(const Value &value, double *out, const char *fn, int argIndex)
    {
        if (!out || !value.isNumber())
        {
            Error("%s arg %d expects number", fn, argIndex);
            return false;
        }
        *out = value.asNumber();
        return true;
    }

    bool read_int_arg(const Value &value, int *out, const char *fn, int argIndex)
    {
        if (!out || !value.isNumber())
        {
            Error("%s arg %d expects int", fn, argIndex);
            return false;
        }
        *out = value.asInt();
        return true;
    }

    bool read_vector2_arg(const Value &value, Vector2 *out, const char *fn, int argIndex)
    {
        if (!out || !value.isNativeStructInstance())
        {
            Error("%s arg %d expects Vector2", fn, argIndex);
            return false;
        }

        NativeStructInstance *inst = value.asNativeStructInstance();
        if (!inst || inst->def != g_vector2Def || !inst->data)
        {
            Error("%s arg %d expects Vector2", fn, argIndex);
            return false;
        }

        *out = *(Vector2 *)inst->data;
        return true;
    }

    bool read_cpvect_arg(const Value &value, cpVect *out, const char *fn, int argIndex)
    {
        if (!out)
            return false;

        Vector2 vec;
        if (!read_vector2_arg(value, &vec, fn, argIndex))
            return false;

        out->x = (cpFloat)vec.x;
        out->y = (cpFloat)vec.y;
        return true;
    }

    bool read_cpvect_array_arg(const Value &value, cpVect **outVerts, int *outCount, const char *fn, int argIndex)
    {
        if (!outVerts || !outCount || !value.isArray())
        {
            Error("%s arg %d expects array", fn, argIndex);
            return false;
        }

        ArrayInstance *arr = value.asArray();
        if (!arr || arr->values.empty())
        {
            Error("%s arg %d expects non-empty array", fn, argIndex);
            return false;
        }

        const int size = (int)arr->values.size();

        bool vectorMode = true;
        for (int i = 0; i < size; ++i)
        {
            NativeStructInstance *inst = arr->values[i].isNativeStructInstance() ? arr->values[i].asNativeStructInstance() : nullptr;
            if (!inst || inst->def != g_vector2Def || !inst->data)
            {
                vectorMode = false;
                break;
            }
        }

        if (vectorMode)
        {
            cpVect *verts = new cpVect[size];
            for (int i = 0; i < size; ++i)
            {
                Vector2 vec = *(Vector2 *)arr->values[i].asNativeStructInstance()->data;
                verts[i].x = (cpFloat)vec.x;
                verts[i].y = (cpFloat)vec.y;
            }
            *outVerts = verts;
            *outCount = size;
            return true;
        }

        if ((size % 2) != 0)
        {
            Error("%s arg %d expects Array<Vector2> or flat numeric array [x0, y0, x1, y1, ...]", fn, argIndex);
            return false;
        }

        cpVect *verts = new cpVect[size / 2];
        for (int i = 0; i < size; i += 2)
        {
            if (!arr->values[i].isNumber() || !arr->values[i + 1].isNumber())
            {
                delete[] verts;
                Error("%s arg %d expects Array<Vector2> or flat numeric array [x0, y0, x1, y1, ...]", fn, argIndex);
                return false;
            }
            verts[i / 2].x = (cpFloat)arr->values[i].asNumber();
            verts[i / 2].y = (cpFloat)arr->values[i + 1].asNumber();
        }

        *outVerts = verts;
        *outCount = size / 2;
        return true;
    }

    void free_cpvect_array(cpVect *verts)
    {
        delete[] verts;
    }

    void push_vector2_components(Interpreter *vm, const Vector2 &value)
    {
        vm->pushDouble((double)value.x);
        vm->pushDouble((double)value.y);
    }

    void push_cpvect_components(Interpreter *vm, const cpVect &value)
    {
        vm->pushDouble((double)value.x);
        vm->pushDouble((double)value.y);
    }

    static bool push_handle(Interpreter *vm, NativeClassDef *klass, void *ptr)
    {
        if (!vm || !klass || !ptr)
            return false;

        Value value = vm->makeNativeClassInstance(false);
        NativeClassInstance *inst = value.asNativeClassInstance();
        if (!inst)
            return false;

        inst->klass = klass;
        inst->userData = ptr;
        vm->push(value);
        return true;
    }

    bool push_space_handle(Interpreter *vm, cpSpace *space)
    {
        return push_handle(vm, g_cpSpaceClass, space);
    }

    bool push_body_handle(Interpreter *vm, cpBody *body)
    {
        return push_handle(vm, g_cpBodyClass, body);
    }

    bool push_shape_handle(Interpreter *vm, cpShape *shape)
    {
        return push_handle(vm, g_cpShapeClass, shape);
    }

    bool push_constraint_handle(Interpreter *vm, cpConstraint *constraint)
    {
        return push_handle(vm, g_cpConstraintClass, constraint);
    }

    static void *require_handle_arg(Interpreter *vm, const Value &value, NativeClassDef *klass, const char *typeName, const char *fn, int argIndex)
    {
        if (!value.isNativeClassInstance())
        {
            Error("%s arg %d expects %s", fn, argIndex, typeName);
            return nullptr;
        }

        NativeClassInstance *inst = value.asNativeClassInstance();
        if (!inst || inst->klass != klass || !inst->userData)
        {
            Error("%s arg %d expects valid %s", fn, argIndex, typeName);
            return nullptr;
        }

        return inst->userData;
    }

    cpSpace *require_space_arg(Interpreter *vm, const Value &value, const char *fn, int argIndex)
    {
        return (cpSpace *)require_handle_arg(vm, value, g_cpSpaceClass, "cpSpace", fn, argIndex);
    }

    cpBody *require_body_arg(Interpreter *vm, const Value &value, const char *fn, int argIndex)
    {
        return (cpBody *)require_handle_arg(vm, value, g_cpBodyClass, "cpBody", fn, argIndex);
    }

    cpShape *require_shape_arg(Interpreter *vm, const Value &value, const char *fn, int argIndex)
    {
        return (cpShape *)require_handle_arg(vm, value, g_cpShapeClass, "cpShape", fn, argIndex);
    }

    cpConstraint *require_constraint_arg(Interpreter *vm, const Value &value, const char *fn, int argIndex)
    {
        return (cpConstraint *)require_handle_arg(vm, value, g_cpConstraintClass, "cpConstraint", fn, argIndex);
    }

    void invalidate_native_handle(const Value &value)
    {
        if (!value.isNativeClassInstance())
            return;

        NativeClassInstance *inst = value.asNativeClassInstance();
        if (inst)
            inst->userData = nullptr;
    }

    static int native_cpMomentForCircle(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("cpMomentForCircle expects (mass, r1, r2, Vector2)");
            return 0;
        }

        double mass = 0.0;
        double r1 = 0.0;
        double r2 = 0.0;
        cpVect offset;
        if (!read_number_arg(args[0], &mass, "cpMomentForCircle", 1) ||
            !read_number_arg(args[1], &r1, "cpMomentForCircle", 2) ||
            !read_number_arg(args[2], &r2, "cpMomentForCircle", 3) ||
            !read_cpvect_arg(args[3], &offset, "cpMomentForCircle", 4))
            return 0;

        vm->pushDouble((double)cpMomentForCircle((cpFloat)mass, (cpFloat)r1, (cpFloat)r2, offset));
        return 1;
    }

    static int native_cpAreaForCircle(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpAreaForCircle expects (r1, r2)");
            return 0;
        }

        double r1 = 0.0;
        double r2 = 0.0;
        if (!read_number_arg(args[0], &r1, "cpAreaForCircle", 1) ||
            !read_number_arg(args[1], &r2, "cpAreaForCircle", 2))
            return 0;

        vm->pushDouble((double)cpAreaForCircle((cpFloat)r1, (cpFloat)r2));
        return 1;
    }

    static int native_cpMomentForSegment(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("cpMomentForSegment expects (mass, a, b, radius)");
            return 0;
        }

        double mass = 0.0;
        double radius = 0.0;
        cpVect a;
        cpVect b;
        if (!read_number_arg(args[0], &mass, "cpMomentForSegment", 1) ||
            !read_cpvect_arg(args[1], &a, "cpMomentForSegment", 2) ||
            !read_cpvect_arg(args[2], &b, "cpMomentForSegment", 3) ||
            !read_number_arg(args[3], &radius, "cpMomentForSegment", 4))
            return 0;

        vm->pushDouble((double)cpMomentForSegment((cpFloat)mass, a, b, (cpFloat)radius));
        return 1;
    }

    static int native_cpAreaForSegment(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("cpAreaForSegment expects (a, b, radius)");
            return 0;
        }

        cpVect a;
        cpVect b;
        double radius = 0.0;
        if (!read_cpvect_arg(args[0], &a, "cpAreaForSegment", 1) ||
            !read_cpvect_arg(args[1], &b, "cpAreaForSegment", 2) ||
            !read_number_arg(args[2], &radius, "cpAreaForSegment", 3))
            return 0;

        vm->pushDouble((double)cpAreaForSegment(a, b, (cpFloat)radius));
        return 1;
    }

    static int native_cpMomentForPoly(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("cpMomentForPoly expects (mass, verts, offset, radius)");
            return 0;
        }

        double mass = 0.0;
        double radius = 0.0;
        cpVect offset;
        cpVect *verts = nullptr;
        int count = 0;
        if (!read_number_arg(args[0], &mass, "cpMomentForPoly", 1) ||
            !read_cpvect_array_arg(args[1], &verts, &count, "cpMomentForPoly", 2) ||
            !read_cpvect_arg(args[2], &offset, "cpMomentForPoly", 3) ||
            !read_number_arg(args[3], &radius, "cpMomentForPoly", 4))
        {
            free_cpvect_array(verts);
            return 0;
        }

        vm->pushDouble((double)cpMomentForPoly((cpFloat)mass, count, verts, offset, (cpFloat)radius));
        free_cpvect_array(verts);
        return 1;
    }

    static int native_cpAreaForPoly(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("cpAreaForPoly expects (verts, radius)");
            return 0;
        }

        cpVect *verts = nullptr;
        int count = 0;
        double radius = 0.0;
        if (!read_cpvect_array_arg(args[0], &verts, &count, "cpAreaForPoly", 1) ||
            !read_number_arg(args[1], &radius, "cpAreaForPoly", 2))
        {
            free_cpvect_array(verts);
            return 0;
        }

        vm->pushDouble((double)cpAreaForPoly(count, verts, (cpFloat)radius));
        free_cpvect_array(verts);
        return 1;
    }

    static int native_cpCentroidForPoly(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("cpCentroidForPoly expects (verts)");
            return 0;
        }

        cpVect *verts = nullptr;
        int count = 0;
        if (!read_cpvect_array_arg(args[0], &verts, &count, "cpCentroidForPoly", 1))
            return 0;

        cpVect centroid = cpCentroidForPoly(count, verts);
        free_cpvect_array(verts);
        push_cpvect_components(vm, centroid);
        return 2;
    }

    static int native_cpMomentForBox(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("cpMomentForBox expects (mass, width, height)");
            return 0;
        }

        double mass = 0.0;
        double width = 0.0;
        double height = 0.0;
        if (!read_number_arg(args[0], &mass, "cpMomentForBox", 1) ||
            !read_number_arg(args[1], &width, "cpMomentForBox", 2) ||
            !read_number_arg(args[2], &height, "cpMomentForBox", 3))
            return 0;

        vm->pushDouble((double)cpMomentForBox((cpFloat)mass, (cpFloat)width, (cpFloat)height));
        return 1;
    }

    void register_misc(ModuleBuilder &module, Interpreter &vm)
    {
        (void)vm;
        module.addFunction("cpMomentForCircle", native_cpMomentForCircle, 4)
            .addFunction("cpAreaForCircle", native_cpAreaForCircle, 2)
            .addFunction("cpMomentForSegment", native_cpMomentForSegment, 4)
            .addFunction("cpAreaForSegment", native_cpAreaForSegment, 3)
            .addFunction("cpMomentForPoly", native_cpMomentForPoly, 4)
            .addFunction("cpAreaForPoly", native_cpAreaForPoly, 2)
            .addFunction("cpCentroidForPoly", native_cpCentroidForPoly, 1)
            .addFunction("cpMomentForBox", native_cpMomentForBox, 3);
    }

    void registerAll(Interpreter &vm)
    {
        g_vector2Def = get_native_struct_def(&vm, "Vector2");

        g_cpSpaceClass = vm.registerNativeClass("cpSpace", chipmunk_handle_ctor_error, chipmunk_handle_dtor, 0, false);
        g_cpBodyClass = vm.registerNativeClass("cpBody", chipmunk_handle_ctor_error, chipmunk_handle_dtor, 0, false);
        g_cpShapeClass = vm.registerNativeClass("cpShape", chipmunk_handle_ctor_error, chipmunk_handle_dtor, 0, false);
        g_cpConstraintClass = vm.registerNativeClass("cpConstraint", chipmunk_handle_ctor_error, chipmunk_handle_dtor, 0, false);

        ModuleBuilder module = vm.addModule("Chip2D");
        module.addString("cpVersionString", cpVersionString)
            .addInt("CP_BODY_TYPE_DYNAMIC", (int)CP_BODY_TYPE_DYNAMIC)
            .addInt("CP_BODY_TYPE_KINEMATIC", (int)CP_BODY_TYPE_KINEMATIC)
            .addInt("CP_BODY_TYPE_STATIC", (int)CP_BODY_TYPE_STATIC);

        register_misc(module, vm);
        register_space(module, vm);
        register_body(module, vm);
        register_shape(module, vm);
        register_constraint(module, vm);
    }
}
