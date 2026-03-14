#include "bindings.hpp"
#include "raymath.h"
#include <cstring>

namespace SDLBindings
{
    static NativeStructDef *g_vec2_def = nullptr;
    static NativeStructDef *g_vec3_def = nullptr;
    static NativeStructDef *g_vec4_def = nullptr;
    static NativeStructDef *g_quat_def = nullptr;
    static NativeStructDef *g_mat_def = nullptr;

    static void vector2_ctor(Interpreter *vm, void *buffer, int argc, Value *args)
    {
        (void)vm;
        Vector2 *v = (Vector2 *)buffer;
        v->x = (argc > 0 && args[0].isNumber()) ? (float)args[0].asNumber() : 0.0f;
        v->y = (argc > 1 && args[1].isNumber()) ? (float)args[1].asNumber() : 0.0f;
    }

    static void vector3_ctor(Interpreter *vm, void *buffer, int argc, Value *args)
    {
        (void)vm;
        Vector3 *v = (Vector3 *)buffer;
        v->x = (argc > 0 && args[0].isNumber()) ? (float)args[0].asNumber() : 0.0f;
        v->y = (argc > 1 && args[1].isNumber()) ? (float)args[1].asNumber() : 0.0f;
        v->z = (argc > 2 && args[2].isNumber()) ? (float)args[2].asNumber() : 0.0f;
    }

    static void vector4_ctor(Interpreter *vm, void *buffer, int argc, Value *args)
    {
        (void)vm;
        Vector4 *v = (Vector4 *)buffer;
        v->x = (argc > 0 && args[0].isNumber()) ? (float)args[0].asNumber() : 0.0f;
        v->y = (argc > 1 && args[1].isNumber()) ? (float)args[1].asNumber() : 0.0f;
        v->z = (argc > 2 && args[2].isNumber()) ? (float)args[2].asNumber() : 0.0f;
        v->w = (argc > 3 && args[3].isNumber()) ? (float)args[3].asNumber() : 0.0f;
    }

    static void quaternion_ctor(Interpreter *vm, void *buffer, int argc, Value *args)
    {
        (void)vm;
        Quaternion *q = (Quaternion *)buffer;
        q->x = (argc > 0 && args[0].isNumber()) ? (float)args[0].asNumber() : 0.0f;
        q->y = (argc > 1 && args[1].isNumber()) ? (float)args[1].asNumber() : 0.0f;
        q->z = (argc > 2 && args[2].isNumber()) ? (float)args[2].asNumber() : 0.0f;
        q->w = (argc > 3 && args[3].isNumber()) ? (float)args[3].asNumber() : 1.0f;
    }

    static void matrix_ctor(Interpreter *vm, void *buffer, int argc, Value *args)
    {
        (void)vm;
        Matrix *m = (Matrix *)buffer;
        *m = MatrixIdentity();

        float *dst = (float *)&m->m0;
        const int n = (argc < 16) ? argc : 16;
        for (int i = 0; i < n; i++)
        {
          
                dst[i] = (float)args[i].asNumber();
        }
    }

    static void register_structs(Interpreter &vm)
    {
        g_vec2_def = vm.registerNativeStruct("Vector2", sizeof(Vector2), vector2_ctor, nullptr);
        vm.addStructField(g_vec2_def, "x", offsetof(Vector2, x), FieldType::FLOAT);
        vm.addStructField(g_vec2_def, "y", offsetof(Vector2, y), FieldType::FLOAT);

        g_vec3_def = vm.registerNativeStruct("Vector3", sizeof(Vector3), vector3_ctor, nullptr);
        vm.addStructField(g_vec3_def, "x", offsetof(Vector3, x), FieldType::FLOAT);
        vm.addStructField(g_vec3_def, "y", offsetof(Vector3, y), FieldType::FLOAT);
        vm.addStructField(g_vec3_def, "z", offsetof(Vector3, z), FieldType::FLOAT);

        g_vec4_def = vm.registerNativeStruct("Vector4", sizeof(Vector4), vector4_ctor, nullptr);
        vm.addStructField(g_vec4_def, "x", offsetof(Vector4, x), FieldType::FLOAT);
        vm.addStructField(g_vec4_def, "y", offsetof(Vector4, y), FieldType::FLOAT);
        vm.addStructField(g_vec4_def, "z", offsetof(Vector4, z), FieldType::FLOAT);
        vm.addStructField(g_vec4_def, "w", offsetof(Vector4, w), FieldType::FLOAT);

        g_quat_def = vm.registerNativeStruct("Quaternion", sizeof(Quaternion), quaternion_ctor, nullptr);
        vm.addStructField(g_quat_def, "x", offsetof(Quaternion, x), FieldType::FLOAT);
        vm.addStructField(g_quat_def, "y", offsetof(Quaternion, y), FieldType::FLOAT);
        vm.addStructField(g_quat_def, "z", offsetof(Quaternion, z), FieldType::FLOAT);
        vm.addStructField(g_quat_def, "w", offsetof(Quaternion, w), FieldType::FLOAT);

        g_mat_def = vm.registerNativeStruct("Matrix", sizeof(Matrix), matrix_ctor, nullptr);
        vm.addStructField(g_mat_def, "m0", offsetof(Matrix, m0), FieldType::FLOAT);
        vm.addStructField(g_mat_def, "m4", offsetof(Matrix, m4), FieldType::FLOAT);
        vm.addStructField(g_mat_def, "m8", offsetof(Matrix, m8), FieldType::FLOAT);
        vm.addStructField(g_mat_def, "m12", offsetof(Matrix, m12), FieldType::FLOAT);
        vm.addStructField(g_mat_def, "m1", offsetof(Matrix, m1), FieldType::FLOAT);
        vm.addStructField(g_mat_def, "m5", offsetof(Matrix, m5), FieldType::FLOAT);
        vm.addStructField(g_mat_def, "m9", offsetof(Matrix, m9), FieldType::FLOAT);
        vm.addStructField(g_mat_def, "m13", offsetof(Matrix, m13), FieldType::FLOAT);
        vm.addStructField(g_mat_def, "m2", offsetof(Matrix, m2), FieldType::FLOAT);
        vm.addStructField(g_mat_def, "m6", offsetof(Matrix, m6), FieldType::FLOAT);
        vm.addStructField(g_mat_def, "m10", offsetof(Matrix, m10), FieldType::FLOAT);
        vm.addStructField(g_mat_def, "m14", offsetof(Matrix, m14), FieldType::FLOAT);
        vm.addStructField(g_mat_def, "m3", offsetof(Matrix, m3), FieldType::FLOAT);
        vm.addStructField(g_mat_def, "m7", offsetof(Matrix, m7), FieldType::FLOAT);
        vm.addStructField(g_mat_def, "m11", offsetof(Matrix, m11), FieldType::FLOAT);
        vm.addStructField(g_mat_def, "m15", offsetof(Matrix, m15), FieldType::FLOAT);
    }

    static bool read_number(const Value &v, double *out, const char *fn, int argIndex)
    {
        if (!v.isNumber())
        {
            Error("%s arg %d expects number", fn, argIndex);
            return false;
        }
        *out = v.asNumber();
        return true;
    }

    static bool read_struct_data(const Value &v, NativeStructDef *def, const char *typeName, const char *fn, int argIndex, void **outData)
    {
        if (!v.isNativeStructInstance())
        {
            Error("%s arg %d expects %s", fn, argIndex, typeName);
            return false;
        }

        NativeStructInstance *inst = v.asNativeStructInstance();
        if (!inst || inst->def != def || !inst->data)
        {
            Error("%s arg %d expects %s", fn, argIndex, typeName);
            return false;
        }

        *outData = inst->data;
        return true;
    }

    static bool read_vec2(const Value &v, Vector2 *out, const char *fn, int argIndex)
    {
        void *data = nullptr;
        if (!read_struct_data(v, g_vec2_def, "Vector2", fn, argIndex, &data))
            return false;
        *out = *(Vector2 *)data;
        return true;
    }

    static bool read_vec3(const Value &v, Vector3 *out, const char *fn, int argIndex)
    {
        void *data = nullptr;
        if (!read_struct_data(v, g_vec3_def, "Vector3", fn, argIndex, &data))
            return false;
        *out = *(Vector3 *)data;
        return true;
    }

    static bool read_quat(const Value &v, Quaternion *out, const char *fn, int argIndex)
    {
        void *data = nullptr;
        if (!read_struct_data(v, g_quat_def, "Quaternion", fn, argIndex, &data))
            return false;
        *out = *(Quaternion *)data;
        return true;
    }

    static bool read_mat(const Value &v, Matrix *out, const char *fn, int argIndex)
    {
        void *data = nullptr;
        if (!read_struct_data(v, g_mat_def, "Matrix", fn, argIndex, &data))
            return false;
        *out = *(Matrix *)data;
        return true;
    }

    static bool push_vec2(Interpreter *vm, const Vector2 &v)
    {
        if (!g_vec2_def)
            return false;
        Value out = vm->createNativeStruct(g_vec2_def->id, 0, nullptr);
        NativeStructInstance *inst = out.asNativeStructInstance();
        if (!inst || !inst->data)
            return false;
        *(Vector2 *)inst->data = v;
        vm->push(out);
        return true;
    }

    static bool push_vec3(Interpreter *vm, const Vector3 &v)
    {
        if (!g_vec3_def)
            return false;
        Value out = vm->createNativeStruct(g_vec3_def->id, 0, nullptr);
        NativeStructInstance *inst = out.asNativeStructInstance();
        if (!inst || !inst->data)
            return false;
        *(Vector3 *)inst->data = v;
        vm->push(out);
        return true;
    }

    static bool push_quat(Interpreter *vm, const Quaternion &q)
    {
        if (!g_quat_def)
            return false;
        Value out = vm->createNativeStruct(g_quat_def->id, 0, nullptr);
        NativeStructInstance *inst = out.asNativeStructInstance();
        if (!inst || !inst->data)
            return false;
        *(Quaternion *)inst->data = q;
        vm->push(out);
        return true;
    }

    static bool push_mat(Interpreter *vm, const Matrix &m)
    {
        if (!g_mat_def)
            return false;
        Value out = vm->createNativeStruct(g_mat_def->id, 0, nullptr);
        NativeStructInstance *inst = out.asNativeStructInstance();
        if (!inst || !inst->data)
            return false;
        *(Matrix *)inst->data = m;
        vm->push(out);
        return true;
    }

    // ------------------------------------------------------------
    // Macro wrappers by signature
    // ------------------------------------------------------------
    #define RM_WRAP_0_TO_V2(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            (void)args; \
            if (argc != 0) { Error(#fn " expects 0 arguments"); return 0; } \
            return push_vec2(vm, fn()) ? 1 : 0; \
        }

    #define RM_WRAP_V2_TO_V2(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 1) { Error(#fn " expects (Vector2)"); return 0; } \
            Vector2 a; \
            if (!read_vec2(args[0], &a, #fn, 1)) return 0; \
            return push_vec2(vm, fn(a)) ? 1 : 0; \
        }

    #define RM_WRAP_V2_V2_TO_V2(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 2) { Error(#fn " expects (Vector2, Vector2)"); return 0; } \
            Vector2 a, b; \
            if (!read_vec2(args[0], &a, #fn, 1) || !read_vec2(args[1], &b, #fn, 2)) return 0; \
            return push_vec2(vm, fn(a, b)) ? 1 : 0; \
        }

    #define RM_WRAP_V2_F_TO_V2(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 2) { Error(#fn " expects (Vector2, number)"); return 0; } \
            Vector2 a; double s; \
            if (!read_vec2(args[0], &a, #fn, 1) || !read_number(args[1], &s, #fn, 2)) return 0; \
            return push_vec2(vm, fn(a, (float)s)) ? 1 : 0; \
        }

    #define RM_WRAP_V2_FF_TO_V2(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 3) { Error(#fn " expects (Vector2, number, number)"); return 0; } \
            Vector2 a; double b, c; \
            if (!read_vec2(args[0], &a, #fn, 1) || !read_number(args[1], &b, #fn, 2) || !read_number(args[2], &c, #fn, 3)) return 0; \
            return push_vec2(vm, fn(a, (float)b, (float)c)) ? 1 : 0; \
        }

    #define RM_WRAP_V2_V2_F_TO_V2(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 3) { Error(#fn " expects (Vector2, Vector2, number)"); return 0; } \
            Vector2 a, b; double c; \
            if (!read_vec2(args[0], &a, #fn, 1) || !read_vec2(args[1], &b, #fn, 2) || !read_number(args[2], &c, #fn, 3)) return 0; \
            return push_vec2(vm, fn(a, b, (float)c)) ? 1 : 0; \
        }

    #define RM_WRAP_V2_V2_V2_TO_V2(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 3) { Error(#fn " expects (Vector2, Vector2, Vector2)"); return 0; } \
            Vector2 a, b, c; \
            if (!read_vec2(args[0], &a, #fn, 1) || !read_vec2(args[1], &b, #fn, 2) || !read_vec2(args[2], &c, #fn, 3)) return 0; \
            return push_vec2(vm, fn(a, b, c)) ? 1 : 0; \
        }

    #define RM_WRAP_V2_MAT_TO_V2(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 2) { Error(#fn " expects (Vector2, Matrix)"); return 0; } \
            Vector2 a; Matrix m; \
            if (!read_vec2(args[0], &a, #fn, 1) || !read_mat(args[1], &m, #fn, 2)) return 0; \
            return push_vec2(vm, fn(a, m)) ? 1 : 0; \
        }

    #define RM_WRAP_V2_TO_F(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 1) { Error(#fn " expects (Vector2)"); return 0; } \
            Vector2 a; \
            if (!read_vec2(args[0], &a, #fn, 1)) return 0; \
            vm->pushDouble((double)fn(a)); \
            return 1; \
        }

    #define RM_WRAP_V2_V2_TO_F(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 2) { Error(#fn " expects (Vector2, Vector2)"); return 0; } \
            Vector2 a, b; \
            if (!read_vec2(args[0], &a, #fn, 1) || !read_vec2(args[1], &b, #fn, 2)) return 0; \
            vm->pushDouble((double)fn(a, b)); \
            return 1; \
        }

    #define RM_WRAP_V2_V2_TO_BOOL(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 2) { Error(#fn " expects (Vector2, Vector2)"); return 0; } \
            Vector2 a, b; \
            if (!read_vec2(args[0], &a, #fn, 1) || !read_vec2(args[1], &b, #fn, 2)) return 0; \
            vm->pushBool(fn(a, b) != 0); \
            return 1; \
        }

    #define RM_WRAP_0_TO_V3(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            (void)args; \
            if (argc != 0) { Error(#fn " expects 0 arguments"); return 0; } \
            return push_vec3(vm, fn()) ? 1 : 0; \
        }

    #define RM_WRAP_V3_TO_V3(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 1) { Error(#fn " expects (Vector3)"); return 0; } \
            Vector3 a; \
            if (!read_vec3(args[0], &a, #fn, 1)) return 0; \
            return push_vec3(vm, fn(a)) ? 1 : 0; \
        }

    #define RM_WRAP_V3_V3_TO_V3(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 2) { Error(#fn " expects (Vector3, Vector3)"); return 0; } \
            Vector3 a, b; \
            if (!read_vec3(args[0], &a, #fn, 1) || !read_vec3(args[1], &b, #fn, 2)) return 0; \
            return push_vec3(vm, fn(a, b)) ? 1 : 0; \
        }

    #define RM_WRAP_V3_F_TO_V3(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 2) { Error(#fn " expects (Vector3, number)"); return 0; } \
            Vector3 a; double s; \
            if (!read_vec3(args[0], &a, #fn, 1) || !read_number(args[1], &s, #fn, 2)) return 0; \
            return push_vec3(vm, fn(a, (float)s)) ? 1 : 0; \
        }

    #define RM_WRAP_V3_V3_F_TO_V3(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 3) { Error(#fn " expects (Vector3, Vector3, number)"); return 0; } \
            Vector3 a, b; double c; \
            if (!read_vec3(args[0], &a, #fn, 1) || !read_vec3(args[1], &b, #fn, 2) || !read_number(args[2], &c, #fn, 3)) return 0; \
            return push_vec3(vm, fn(a, b, (float)c)) ? 1 : 0; \
        }

    #define RM_WRAP_V3_MAT_TO_V3(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 2) { Error(#fn " expects (Vector3, Matrix)"); return 0; } \
            Vector3 a; Matrix m; \
            if (!read_vec3(args[0], &a, #fn, 1) || !read_mat(args[1], &m, #fn, 2)) return 0; \
            return push_vec3(vm, fn(a, m)) ? 1 : 0; \
        }

    #define RM_WRAP_V3_MAT_MAT_TO_V3(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 3) { Error(#fn " expects (Vector3, Matrix, Matrix)"); return 0; } \
            Vector3 a; Matrix p, v; \
            if (!read_vec3(args[0], &a, #fn, 1) || !read_mat(args[1], &p, #fn, 2) || !read_mat(args[2], &v, #fn, 3)) return 0; \
            return push_vec3(vm, fn(a, p, v)) ? 1 : 0; \
        }

    #define RM_WRAP_V3_QUAT_TO_V3(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 2) { Error(#fn " expects (Vector3, Quaternion)"); return 0; } \
            Vector3 a; Quaternion q; \
            if (!read_vec3(args[0], &a, #fn, 1) || !read_quat(args[1], &q, #fn, 2)) return 0; \
            return push_vec3(vm, fn(a, q)) ? 1 : 0; \
        }

    #define RM_WRAP_V3_V3_V3_TO_V3(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 3) { Error(#fn " expects (Vector3, Vector3, Vector3)"); return 0; } \
            Vector3 a, b, c; \
            if (!read_vec3(args[0], &a, #fn, 1) || !read_vec3(args[1], &b, #fn, 2) || !read_vec3(args[2], &c, #fn, 3)) return 0; \
            return push_vec3(vm, fn(a, b, c)) ? 1 : 0; \
        }

    #define RM_WRAP_V3_V3_V3_V3_TO_V3(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 4) { Error(#fn " expects (Vector3, Vector3, Vector3, Vector3)"); return 0; } \
            Vector3 a, b, c, d; \
            if (!read_vec3(args[0], &a, #fn, 1) || !read_vec3(args[1], &b, #fn, 2) || !read_vec3(args[2], &c, #fn, 3) || !read_vec3(args[3], &d, #fn, 4)) return 0; \
            return push_vec3(vm, fn(a, b, c, d)) ? 1 : 0; \
        }

    #define RM_WRAP_V3_TO_F(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 1) { Error(#fn " expects (Vector3)"); return 0; } \
            Vector3 a; \
            if (!read_vec3(args[0], &a, #fn, 1)) return 0; \
            vm->pushDouble((double)fn(a)); \
            return 1; \
        }

    #define RM_WRAP_V3_V3_TO_F(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 2) { Error(#fn " expects (Vector3, Vector3)"); return 0; } \
            Vector3 a, b; \
            if (!read_vec3(args[0], &a, #fn, 1) || !read_vec3(args[1], &b, #fn, 2)) return 0; \
            vm->pushDouble((double)fn(a, b)); \
            return 1; \
        }

    #define RM_WRAP_V3_V3_TO_BOOL(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 2) { Error(#fn " expects (Vector3, Vector3)"); return 0; } \
            Vector3 a, b; \
            if (!read_vec3(args[0], &a, #fn, 1) || !read_vec3(args[1], &b, #fn, 2)) return 0; \
            vm->pushBool(fn(a, b) != 0); \
            return 1; \
        }

    #define RM_WRAP_0_TO_Q(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            (void)args; \
            if (argc != 0) { Error(#fn " expects 0 arguments"); return 0; } \
            return push_quat(vm, fn()) ? 1 : 0; \
        }

    #define RM_WRAP_Q_TO_Q(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 1) { Error(#fn " expects (Quaternion)"); return 0; } \
            Quaternion q; \
            if (!read_quat(args[0], &q, #fn, 1)) return 0; \
            return push_quat(vm, fn(q)) ? 1 : 0; \
        }

    #define RM_WRAP_Q_Q_TO_Q(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 2) { Error(#fn " expects (Quaternion, Quaternion)"); return 0; } \
            Quaternion a, b; \
            if (!read_quat(args[0], &a, #fn, 1) || !read_quat(args[1], &b, #fn, 2)) return 0; \
            return push_quat(vm, fn(a, b)) ? 1 : 0; \
        }

    #define RM_WRAP_Q_F_TO_Q(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 2) { Error(#fn " expects (Quaternion, number)"); return 0; } \
            Quaternion q; double s; \
            if (!read_quat(args[0], &q, #fn, 1) || !read_number(args[1], &s, #fn, 2)) return 0; \
            return push_quat(vm, fn(q, (float)s)) ? 1 : 0; \
        }

    #define RM_WRAP_Q_Q_F_TO_Q(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 3) { Error(#fn " expects (Quaternion, Quaternion, number)"); return 0; } \
            Quaternion a, b; double c; \
            if (!read_quat(args[0], &a, #fn, 1) || !read_quat(args[1], &b, #fn, 2) || !read_number(args[2], &c, #fn, 3)) return 0; \
            return push_quat(vm, fn(a, b, (float)c)) ? 1 : 0; \
        }

    #define RM_WRAP_V3_V3_TO_Q(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 2) { Error(#fn " expects (Vector3, Vector3)"); return 0; } \
            Vector3 a, b; \
            if (!read_vec3(args[0], &a, #fn, 1) || !read_vec3(args[1], &b, #fn, 2)) return 0; \
            return push_quat(vm, fn(a, b)) ? 1 : 0; \
        }

    #define RM_WRAP_MAT_TO_Q(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 1) { Error(#fn " expects (Matrix)"); return 0; } \
            Matrix m; \
            if (!read_mat(args[0], &m, #fn, 1)) return 0; \
            return push_quat(vm, fn(m)) ? 1 : 0; \
        }

    #define RM_WRAP_Q_TO_MAT(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 1) { Error(#fn " expects (Quaternion)"); return 0; } \
            Quaternion q; \
            if (!read_quat(args[0], &q, #fn, 1)) return 0; \
            return push_mat(vm, fn(q)) ? 1 : 0; \
        }

    #define RM_WRAP_V3_F_TO_Q(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 2) { Error(#fn " expects (Vector3, number)"); return 0; } \
            Vector3 a; double b; \
            if (!read_vec3(args[0], &a, #fn, 1) || !read_number(args[1], &b, #fn, 2)) return 0; \
            return push_quat(vm, fn(a, (float)b)) ? 1 : 0; \
        }

    #define RM_WRAP_FFF_TO_Q(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 3) { Error(#fn " expects (number, number, number)"); return 0; } \
            double a, b, c; \
            if (!read_number(args[0], &a, #fn, 1) || !read_number(args[1], &b, #fn, 2) || !read_number(args[2], &c, #fn, 3)) return 0; \
            return push_quat(vm, fn((float)a, (float)b, (float)c)) ? 1 : 0; \
        }

    #define RM_WRAP_Q_TO_V3(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 1) { Error(#fn " expects (Quaternion)"); return 0; } \
            Quaternion q; \
            if (!read_quat(args[0], &q, #fn, 1)) return 0; \
            return push_vec3(vm, fn(q)) ? 1 : 0; \
        }

    #define RM_WRAP_Q_MAT_TO_Q(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 2) { Error(#fn " expects (Quaternion, Matrix)"); return 0; } \
            Quaternion q; Matrix m; \
            if (!read_quat(args[0], &q, #fn, 1) || !read_mat(args[1], &m, #fn, 2)) return 0; \
            return push_quat(vm, fn(q, m)) ? 1 : 0; \
        }

    #define RM_WRAP_Q_TO_F(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 1) { Error(#fn " expects (Quaternion)"); return 0; } \
            Quaternion q; \
            if (!read_quat(args[0], &q, #fn, 1)) return 0; \
            vm->pushDouble((double)fn(q)); \
            return 1; \
        }

    #define RM_WRAP_Q_Q_TO_BOOL(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 2) { Error(#fn " expects (Quaternion, Quaternion)"); return 0; } \
            Quaternion a, b; \
            if (!read_quat(args[0], &a, #fn, 1) || !read_quat(args[1], &b, #fn, 2)) return 0; \
            vm->pushBool(fn(a, b) != 0); \
            return 1; \
        }

    #define RM_WRAP_0_TO_MAT(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            (void)args; \
            if (argc != 0) { Error(#fn " expects 0 arguments"); return 0; } \
            return push_mat(vm, fn()) ? 1 : 0; \
        }

    #define RM_WRAP_MAT_TO_MAT(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 1) { Error(#fn " expects (Matrix)"); return 0; } \
            Matrix m; \
            if (!read_mat(args[0], &m, #fn, 1)) return 0; \
            return push_mat(vm, fn(m)) ? 1 : 0; \
        }

    #define RM_WRAP_MAT_MAT_TO_MAT(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 2) { Error(#fn " expects (Matrix, Matrix)"); return 0; } \
            Matrix a, b; \
            if (!read_mat(args[0], &a, #fn, 1) || !read_mat(args[1], &b, #fn, 2)) return 0; \
            return push_mat(vm, fn(a, b)) ? 1 : 0; \
        }

    #define RM_WRAP_MAT_TO_F(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 1) { Error(#fn " expects (Matrix)"); return 0; } \
            Matrix m; \
            if (!read_mat(args[0], &m, #fn, 1)) return 0; \
            vm->pushDouble((double)fn(m)); \
            return 1; \
        }

    #define RM_WRAP_FFF_TO_MAT(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 3) { Error(#fn " expects (number, number, number)"); return 0; } \
            double a, b, c; \
            if (!read_number(args[0], &a, #fn, 1) || !read_number(args[1], &b, #fn, 2) || !read_number(args[2], &c, #fn, 3)) return 0; \
            return push_mat(vm, fn((float)a, (float)b, (float)c)) ? 1 : 0; \
        }

    #define RM_WRAP_F_TO_MAT(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 1) { Error(#fn " expects (number)"); return 0; } \
            double a; \
            if (!read_number(args[0], &a, #fn, 1)) return 0; \
            return push_mat(vm, fn((float)a)) ? 1 : 0; \
        }

    #define RM_WRAP_V3_F_TO_MAT(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 2) { Error(#fn " expects (Vector3, number)"); return 0; } \
            Vector3 v; double a; \
            if (!read_vec3(args[0], &v, #fn, 1) || !read_number(args[1], &a, #fn, 2)) return 0; \
            return push_mat(vm, fn(v, (float)a)) ? 1 : 0; \
        }

    #define RM_WRAP_V3_TO_MAT(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 1) { Error(#fn " expects (Vector3)"); return 0; } \
            Vector3 v; \
            if (!read_vec3(args[0], &v, #fn, 1)) return 0; \
            return push_mat(vm, fn(v)) ? 1 : 0; \
        }

    #define RM_WRAP_D4_TO_MAT(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 4) { Error(#fn " expects 4 numeric arguments"); return 0; } \
            double a, b, c, d; \
            if (!read_number(args[0], &a, #fn, 1) || !read_number(args[1], &b, #fn, 2) || !read_number(args[2], &c, #fn, 3) || !read_number(args[3], &d, #fn, 4)) return 0; \
            return push_mat(vm, fn(a, b, c, d)) ? 1 : 0; \
        }

    #define RM_WRAP_D6_TO_MAT(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 6) { Error(#fn " expects 6 numeric arguments"); return 0; } \
            double a, b, c, d, e, f; \
            if (!read_number(args[0], &a, #fn, 1) || !read_number(args[1], &b, #fn, 2) || !read_number(args[2], &c, #fn, 3) || !read_number(args[3], &d, #fn, 4) || !read_number(args[4], &e, #fn, 5) || !read_number(args[5], &f, #fn, 6)) return 0; \
            return push_mat(vm, fn(a, b, c, d, e, f)) ? 1 : 0; \
        }

    #define RM_WRAP_V3_V3_V3_TO_MAT(fn) \
        static int native_##fn(Interpreter *vm, int argc, Value *args) { \
            if (argc != 3) { Error(#fn " expects (Vector3, Vector3, Vector3)"); return 0; } \
            Vector3 a, b, c; \
            if (!read_vec3(args[0], &a, #fn, 1) || !read_vec3(args[1], &b, #fn, 2) || !read_vec3(args[2], &c, #fn, 3)) return 0; \
            return push_mat(vm, fn(a, b, c)) ? 1 : 0; \
        }

    // ------------------------------------------------------------
    // Function lists
    // ------------------------------------------------------------
    #define RM_V2_0_TO_V2_LIST(X) \
        X(Vector2Zero)            \
        X(Vector2One)

    #define RM_V2_TO_V2_LIST(X) \
        X(Vector2Negate)         \
        X(Vector2Normalize)      \
        X(Vector2Invert)

    #define RM_V2_V2_TO_V2_LIST(X) \
        X(Vector2Add)               \
        X(Vector2Subtract)          \
        X(Vector2Multiply)          \
        X(Vector2Divide)            \
        X(Vector2Reflect)

    #define RM_V2_V2_V2_TO_V2_LIST(X) \
        X(Vector2Clamp)

    #define RM_V2_F_TO_V2_LIST(X) \
        X(Vector2AddValue)         \
        X(Vector2SubtractValue)    \
        X(Vector2Scale)            \
        X(Vector2Rotate)

    #define RM_V2_FF_TO_V2_LIST(X) \
        X(Vector2ClampValue)

    #define RM_V2_V2_F_TO_V2_LIST(X) \
        X(Vector2Lerp)                \
        X(Vector2MoveTowards)

    #define RM_V2_MAT_TO_V2_LIST(X) \
        X(Vector2Transform)

    #define RM_V2_TO_F_LIST(X) \
        X(Vector2Length)        \
        X(Vector2LengthSqr)

    #define RM_V2_V2_TO_F_LIST(X) \
        X(Vector2DotProduct)       \
        X(Vector2Distance)         \
        X(Vector2DistanceSqr)      \
        X(Vector2Angle)            \
        X(Vector2LineAngle)

    #define RM_V2_V2_TO_BOOL_LIST(X) \
        X(Vector2Equals)

    #define RM_V3_0_TO_V3_LIST(X) \
        X(Vector3Zero)             \
        X(Vector3One)

    #define RM_V3_TO_V3_LIST(X) \
        X(Vector3Perpendicular)   \
        X(Vector3Negate)          \
        X(Vector3Normalize)       \
        X(Vector3Invert)

    #define RM_V3_V3_TO_V3_LIST(X) \
        X(Vector3Add)               \
        X(Vector3Subtract)          \
        X(Vector3Multiply)          \
        X(Vector3CrossProduct)      \
        X(Vector3Divide)            \
        X(Vector3Project)           \
        X(Vector3Reject)            \
        X(Vector3Reflect)           \
        X(Vector3Min)               \
        X(Vector3Max)

    #define RM_V3_F_TO_V3_LIST(X) \
        X(Vector3AddValue)         \
        X(Vector3SubtractValue)    \
        X(Vector3Scale)

    #define RM_V3_V3_F_TO_V3_LIST(X) \
        X(Vector3Lerp)                \
        X(Vector3RotateByAxisAngle)   \
        X(Vector3Refract)

    #define RM_V3_MAT_TO_V3_LIST(X) \
        X(Vector3Transform)

    #define RM_V3_MAT_MAT_TO_V3_LIST(X) \
        X(Vector3Unproject)

    #define RM_V3_QUAT_TO_V3_LIST(X) \
        X(Vector3RotateByQuaternion)

    #define RM_V3_V3_V3_TO_V3_LIST(X)

    #define RM_V3_V3_V3_V3_TO_V3_LIST(X) \
        X(Vector3Barycenter)

    #define RM_V3_TO_F_LIST(X) \
        X(Vector3Length)        \
        X(Vector3LengthSqr)

    #define RM_V3_V3_TO_F_LIST(X) \
        X(Vector3DotProduct)       \
        X(Vector3Distance)         \
        X(Vector3DistanceSqr)      \
        X(Vector3Angle)

    #define RM_V3_V3_TO_BOOL_LIST(X) \
        X(Vector3Equals)

    #define RM_Q_0_TO_Q_LIST(X) \
        X(QuaternionIdentity)

    #define RM_Q_TO_Q_LIST(X) \
        X(QuaternionNormalize) \
        X(QuaternionInvert)

    #define RM_Q_Q_TO_Q_LIST(X) \
        X(QuaternionAdd)         \
        X(QuaternionSubtract)    \
        X(QuaternionMultiply)    \
        X(QuaternionDivide)

    #define RM_Q_F_TO_Q_LIST(X) \
        X(QuaternionAddValue)    \
        X(QuaternionSubtractValue) \
        X(QuaternionScale)

    #define RM_Q_Q_F_TO_Q_LIST(X) \
        X(QuaternionLerp)          \
        X(QuaternionNlerp)         \
        X(QuaternionSlerp)

    #define RM_Q_TO_F_LIST(X) \
        X(QuaternionLength)

    #define RM_Q_Q_TO_BOOL_LIST(X) \
        X(QuaternionEquals)

    #define RM_V3_V3_TO_Q_LIST(X) \
        X(QuaternionFromVector3ToVector3)

    #define RM_MAT_TO_Q_LIST(X) \
        X(QuaternionFromMatrix)

    #define RM_Q_TO_MAT_LIST(X) \
        X(QuaternionToMatrix)

    #define RM_V3_F_TO_Q_LIST(X) \
        X(QuaternionFromAxisAngle)

    #define RM_FFF_TO_Q_LIST(X) \
        X(QuaternionFromEuler)

    #define RM_Q_TO_V3_LIST(X) \
        X(QuaternionToEuler)

    #define RM_Q_MAT_TO_Q_LIST(X) \
        X(QuaternionTransform)

    #define RM_MAT_0_TO_MAT_LIST(X) \
        X(MatrixIdentity)

    #define RM_MAT_TO_MAT_LIST(X) \
        X(MatrixTranspose)         \
        X(MatrixInvert)

    #define RM_MAT_MAT_TO_MAT_LIST(X) \
        X(MatrixAdd)                   \
        X(MatrixSubtract)              \
        X(MatrixMultiply)

    #define RM_MAT_TO_F_LIST(X) \
        X(MatrixDeterminant)     \
        X(MatrixTrace)

    #define RM_FFF_TO_MAT_LIST(X) \
        X(MatrixTranslate)         \
        X(MatrixScale)

    #define RM_F_TO_MAT_LIST(X) \
        X(MatrixRotateX)         \
        X(MatrixRotateY)         \
        X(MatrixRotateZ)

    #define RM_V3_F_TO_MAT_LIST(X) \
        X(MatrixRotate)

    #define RM_V3_TO_MAT_LIST(X) \
        X(MatrixRotateXYZ)        \
        X(MatrixRotateZYX)

    #define RM_D4_TO_MAT_LIST(X) \
        X(MatrixPerspective)

    #define RM_D6_TO_MAT_LIST(X) \
        X(MatrixFrustum)          \
        X(MatrixOrtho)

    #define RM_V3_V3_V3_TO_MAT_LIST(X) \
        X(MatrixLookAt)

    // ------------------------------------------------------------
    // Generate wrappers from lists
    // ------------------------------------------------------------
    #define X(fn) RM_WRAP_0_TO_V2(fn)
    RM_V2_0_TO_V2_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V2_TO_V2(fn)
    RM_V2_TO_V2_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V2_V2_TO_V2(fn)
    RM_V2_V2_TO_V2_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V2_V2_V2_TO_V2(fn)
    RM_V2_V2_V2_TO_V2_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V2_F_TO_V2(fn)
    RM_V2_F_TO_V2_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V2_FF_TO_V2(fn)
    RM_V2_FF_TO_V2_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V2_V2_F_TO_V2(fn)
    RM_V2_V2_F_TO_V2_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V2_MAT_TO_V2(fn)
    RM_V2_MAT_TO_V2_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V2_TO_F(fn)
    RM_V2_TO_F_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V2_V2_TO_F(fn)
    RM_V2_V2_TO_F_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V2_V2_TO_BOOL(fn)
    RM_V2_V2_TO_BOOL_LIST(X)
    #undef X

    #define X(fn) RM_WRAP_0_TO_V3(fn)
    RM_V3_0_TO_V3_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V3_TO_V3(fn)
    RM_V3_TO_V3_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V3_V3_TO_V3(fn)
    RM_V3_V3_TO_V3_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V3_F_TO_V3(fn)
    RM_V3_F_TO_V3_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V3_V3_F_TO_V3(fn)
    RM_V3_V3_F_TO_V3_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V3_MAT_TO_V3(fn)
    RM_V3_MAT_TO_V3_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V3_MAT_MAT_TO_V3(fn)
    RM_V3_MAT_MAT_TO_V3_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V3_QUAT_TO_V3(fn)
    RM_V3_QUAT_TO_V3_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V3_V3_V3_TO_V3(fn)
    RM_V3_V3_V3_TO_V3_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V3_V3_V3_V3_TO_V3(fn)
    RM_V3_V3_V3_V3_TO_V3_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V3_TO_F(fn)
    RM_V3_TO_F_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V3_V3_TO_F(fn)
    RM_V3_V3_TO_F_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V3_V3_TO_BOOL(fn)
    RM_V3_V3_TO_BOOL_LIST(X)
    #undef X

    #define X(fn) RM_WRAP_0_TO_Q(fn)
    RM_Q_0_TO_Q_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_Q_TO_Q(fn)
    RM_Q_TO_Q_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_Q_Q_TO_Q(fn)
    RM_Q_Q_TO_Q_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_Q_F_TO_Q(fn)
    RM_Q_F_TO_Q_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_Q_Q_F_TO_Q(fn)
    RM_Q_Q_F_TO_Q_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_Q_TO_F(fn)
    RM_Q_TO_F_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_Q_Q_TO_BOOL(fn)
    RM_Q_Q_TO_BOOL_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V3_V3_TO_Q(fn)
    RM_V3_V3_TO_Q_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_MAT_TO_Q(fn)
    RM_MAT_TO_Q_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_Q_TO_MAT(fn)
    RM_Q_TO_MAT_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V3_F_TO_Q(fn)
    RM_V3_F_TO_Q_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_FFF_TO_Q(fn)
    RM_FFF_TO_Q_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_Q_TO_V3(fn)
    RM_Q_TO_V3_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_Q_MAT_TO_Q(fn)
    RM_Q_MAT_TO_Q_LIST(X)
    #undef X

    #define X(fn) RM_WRAP_0_TO_MAT(fn)
    RM_MAT_0_TO_MAT_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_MAT_TO_MAT(fn)
    RM_MAT_TO_MAT_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_MAT_MAT_TO_MAT(fn)
    RM_MAT_MAT_TO_MAT_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_MAT_TO_F(fn)
    RM_MAT_TO_F_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_FFF_TO_MAT(fn)
    RM_FFF_TO_MAT_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_F_TO_MAT(fn)
    RM_F_TO_MAT_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V3_F_TO_MAT(fn)
    RM_V3_F_TO_MAT_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V3_TO_MAT(fn)
    RM_V3_TO_MAT_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_D4_TO_MAT(fn)
    RM_D4_TO_MAT_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_D6_TO_MAT(fn)
    RM_D6_TO_MAT_LIST(X)
    #undef X
    #define X(fn) RM_WRAP_V3_V3_V3_TO_MAT(fn)
    RM_V3_V3_V3_TO_MAT_LIST(X)
    #undef X

    // ------------------------------------------------------------
    // Custom wrappers not covered by simple signatures
    // ------------------------------------------------------------
    static int native_QuaternionToAxisAngle(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("QuaternionToAxisAngle expects (Quaternion)");
            return 0;
        }

        Quaternion q;
        if (!read_quat(args[0], &q, "QuaternionToAxisAngle", 1))
            return 0;

        Vector3 axis = {0.0f, 0.0f, 0.0f};
        float angle = 0.0f;
        QuaternionToAxisAngle(q, &axis, &angle);

        if (!push_vec3(vm, axis))
            return 0;
        vm->pushDouble((double)angle);
        return 2;
    }

    static int native_Vector3OrthoNormalize(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("Vector3OrthoNormalize expects (Vector3, Vector3)");
            return 0;
        }

        Vector3 a, b;
        if (!read_vec3(args[0], &a, "Vector3OrthoNormalize", 1) ||
            !read_vec3(args[1], &b, "Vector3OrthoNormalize", 2))
        {
            return 0;
        }

        Vector3OrthoNormalize(&a, &b);
        if (!push_vec3(vm, a) || !push_vec3(vm, b))
            return 0;
        return 2;
    }

    static int native_MatrixToArray(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("MatrixToArray expects (Matrix)");
            return 0;
        }

        Matrix m;
        if (!read_mat(args[0], &m, "MatrixToArray", 1))
            return 0;

        Value out = vm->makeArray();
        ArrayInstance *arr = out.asArray();
        if (!arr)
            return 0;

        arr->values.reserve(16);
        const float *f = (const float *)&m.m0;
        for (int i = 0; i < 16; i++)
            arr->values.push(vm->makeFloat(f[i]));

        vm->push(out);
        return 1;
    }

    static int native_MatrixToBuffer(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("MatrixToBuffer expects (Matrix)");
            return 0;
        }

        Matrix m;
        if (!read_mat(args[0], &m, "MatrixToBuffer", 1))
            return 0;

        Value out = vm->makeBuffer(16, (int)BufferType::FLOAT);
        BufferInstance *buf = out.asBuffer();
        if (!buf || !buf->data)
            return 0;

        // Matrix struct field order is not contiguous [m0..m15] in memory.
        // Serialize explicitly to OpenGL-friendly column-major float[16].
        float outMat[16] = {
            m.m0, m.m1, m.m2, m.m3,
            m.m4, m.m5, m.m6, m.m7,
            m.m8, m.m9, m.m10, m.m11,
            m.m12, m.m13, m.m14, m.m15};
        std::memcpy(buf->data, outMat, sizeof(outMat));
        vm->push(out);
        return 1;
    }

    static int native_MatrixWriteBuffer(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("MatrixWriteBuffer expects (Matrix, buffer)");
            return 0;
        }

        Matrix m;
        if (!read_mat(args[0], &m, "MatrixWriteBuffer", 1))
            return 0;

        if (!args[1].isBuffer())
        {
            Error("MatrixWriteBuffer arg 2 expects buffer");
            return 0;
        }

        BufferInstance *buf = args[1].asBuffer();
        if (!buf || !buf->data)
        {
            Error("MatrixWriteBuffer buffer is null");
            return 0;
        }

        const size_t needBytes = 16 * sizeof(float);
        const size_t haveBytes = (size_t)buf->count * (size_t)buf->elementSize;
        if (haveBytes < needBytes)
        {
            Error("MatrixWriteBuffer needs at least 16 float slots (64 bytes)");
            return 0;
        }

        float outMat[16] = {
            m.m0, m.m1, m.m2, m.m3,
            m.m4, m.m5, m.m6, m.m7,
            m.m8, m.m9, m.m10, m.m11,
            m.m12, m.m13, m.m14, m.m15};
        std::memcpy(buf->data, outMat, sizeof(outMat));
        vm->pushBool(true);
        return 1;
    }

    void register_raymath(Interpreter &vm)
    {
        register_structs(vm);

        ModuleBuilder module = vm.addModule("GLM");
   

        #define X(fn) module.addFunction(#fn, native_##fn, 0);
        RM_V2_0_TO_V2_LIST(X)
        RM_V3_0_TO_V3_LIST(X)
        RM_Q_0_TO_Q_LIST(X)
        RM_MAT_0_TO_MAT_LIST(X)
        #undef X

        #define X(fn) module.addFunction(#fn, native_##fn, 1);
        RM_V2_TO_V2_LIST(X)
        RM_V2_TO_F_LIST(X)
        RM_V3_TO_V3_LIST(X)
        RM_V3_TO_F_LIST(X)
        RM_Q_TO_Q_LIST(X)
        RM_Q_TO_F_LIST(X)
        RM_MAT_TO_MAT_LIST(X)
        RM_MAT_TO_F_LIST(X)
        RM_MAT_TO_Q_LIST(X)
        RM_Q_TO_MAT_LIST(X)
        RM_Q_TO_V3_LIST(X)
        RM_V3_TO_MAT_LIST(X)
        RM_F_TO_MAT_LIST(X)
        #undef X

        #define X(fn) module.addFunction(#fn, native_##fn, 2);
        RM_V2_V2_TO_V2_LIST(X)
        RM_V2_F_TO_V2_LIST(X)
        RM_V2_MAT_TO_V2_LIST(X)
        RM_V2_V2_TO_F_LIST(X)
        RM_V2_V2_TO_BOOL_LIST(X)
        RM_V3_V3_TO_V3_LIST(X)
        RM_V3_F_TO_V3_LIST(X)
        RM_V3_MAT_TO_V3_LIST(X)
        RM_V3_QUAT_TO_V3_LIST(X)
        RM_V3_V3_TO_F_LIST(X)
        RM_V3_V3_TO_BOOL_LIST(X)
        RM_Q_Q_TO_Q_LIST(X)
        RM_Q_F_TO_Q_LIST(X)
        RM_V3_V3_TO_Q_LIST(X)
        RM_V3_F_TO_Q_LIST(X)
        RM_Q_MAT_TO_Q_LIST(X)
        RM_Q_Q_TO_BOOL_LIST(X)
        RM_MAT_MAT_TO_MAT_LIST(X)
        RM_V3_F_TO_MAT_LIST(X)
        #undef X

        #define X(fn) module.addFunction(#fn, native_##fn, 3);
        RM_V2_V2_F_TO_V2_LIST(X)
        RM_V2_V2_V2_TO_V2_LIST(X)
        RM_V3_V3_F_TO_V3_LIST(X)
        RM_V3_V3_V3_TO_V3_LIST(X)
        RM_Q_Q_F_TO_Q_LIST(X)
        RM_FFF_TO_Q_LIST(X)
        RM_FFF_TO_MAT_LIST(X)
        RM_V3_V3_V3_TO_MAT_LIST(X)
        #undef X

        #define X(fn) module.addFunction(#fn, native_##fn, 4);
        RM_V3_V3_V3_V3_TO_V3_LIST(X)
        RM_D4_TO_MAT_LIST(X)
        #undef X

        #define X(fn) module.addFunction(#fn, native_##fn, 6);
        RM_D6_TO_MAT_LIST(X)
        #undef X

        module.addFunction("QuaternionToAxisAngle", native_QuaternionToAxisAngle, 1);
        module.addFunction("Vector3OrthoNormalize", native_Vector3OrthoNormalize, 2);
        module.addFunction("MatrixToArray", native_MatrixToArray, 1);
        module.addFunction("MatrixToBuffer", native_MatrixToBuffer, 1);
        module.addFunction("MatrixWriteBuffer", native_MatrixWriteBuffer, 2);
    }
}
