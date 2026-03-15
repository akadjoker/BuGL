#include "pch.h"
#include "bindings.hpp"
#include "batch.hpp"
#include "spriteFont.hpp"
#include "raymath.h"

#include <cstring>
#include <vector>

namespace SDLBindings
{
    namespace
    {
        static constexpr int kBatchModeLines = 0x0001;
        static constexpr int kBatchModeTriangles = 0x0004;
        static constexpr int kBatchModeQuad = 0x0008;

        struct BatchHandle
        {
            RenderBatch batch;
            bool initialized{false};
        };

        static NativeStructDef *g_vec2Def = nullptr;
        static NativeStructDef *g_vec3Def = nullptr;
        static NativeStructDef *g_matDef = nullptr;

        static BatchHandle *as_batch(void *instance)
        {
            return (BatchHandle *)instance;
        }

        static bool read_boolish(const Value &value, bool *out, const char *fn, int argIndex)
        {
            if (!out)
                return false;

            if (value.isBool())
            {
                *out = value.asBool();
                return true;
            }

            if (value.isNumber())
            {
                *out = value.asNumber() != 0.0;
                return true;
            }

            Error("%s arg %d expects bool", fn, argIndex);
            return false;
        }

        static void ensure_struct_defs(Interpreter *vm)
        {
            if (!vm)
                return;

            if (!g_vec2Def)
                vm->tryGetNativeStructDef("Vector2", &g_vec2Def);
            if (!g_vec3Def)
                vm->tryGetNativeStructDef("Vector3", &g_vec3Def);
            if (!g_matDef)
                vm->tryGetNativeStructDef("Matrix", &g_matDef);
        }

        static bool read_vec2_arg(Interpreter *vm, const Value &value, Vector2 *out, const char *fn, int argIndex)
        {
            if (!out)
                return false;

            ensure_struct_defs(vm);

            if (value.isNativeStructInstance())
            {
                NativeStructInstance *inst = value.asNativeStructInstance();
                if (inst && inst->data && inst->def == g_vec2Def)
                {
                    const Vector2 *v = (const Vector2 *)inst->data;
                    *out = *v;
                    return true;
                }
            }

            float xy[2];
            if (vm && vm->getVec2(value, xy))
            {
                out->x = xy[0];
                out->y = xy[1];
                return true;
            }

            Error("%s arg %d expects Vector2 or [x, y]", fn, argIndex);
            return false;
        }

        static bool read_vec3_arg(Interpreter *vm, const Value &value, Vector3 *out, const char *fn, int argIndex)
        {
            if (!out)
                return false;

            ensure_struct_defs(vm);

            if (value.isNativeStructInstance())
            {
                NativeStructInstance *inst = value.asNativeStructInstance();
                if (inst && inst->data && inst->def == g_vec3Def)
                {
                    const Vector3 *v = (const Vector3 *)inst->data;
                    *out = *v;
                    return true;
                }
            }

            float xyz[3];
            if (vm && vm->getVec3(value, xyz))
            {
                out->x = xyz[0];
                out->y = xyz[1];
                out->z = xyz[2];
                return true;
            }

            Error("%s arg %d expects Vector3 or [x, y, z]", fn, argIndex);
            return false;
        }

        static bool read_mat4_arg(Interpreter *vm, const Value &value, Matrix *out, const char *fn, int argIndex)
        {
            if (!out)
                return false;

            ensure_struct_defs(vm);

            if (value.isNativeStructInstance())
            {
                NativeStructInstance *inst = value.asNativeStructInstance();
                if (inst && inst->data && inst->def == g_matDef)
                {
                    const Matrix *m = (const Matrix *)inst->data;
                    *out = *m;
                    return true;
                }
            }

            float values[16];
            if (vm && vm->getMat4(value, values))
            {
                std::memcpy(out, values, sizeof(values));
                return true;
            }

            Error("%s arg %d expects Matrix or float[16]", fn, argIndex);
            return false;
        }

        static bool read_vec2_array_arg(Interpreter *vm, const Value &value, std::vector<Vector2> *out, const char *fn, int argIndex, int minCount = 0)
        {
            if (!out)
                return false;

            if (!value.isArray())
            {
                Error("%s arg %d expects Vector2[]", fn, argIndex);
                return false;
            }

            ArrayInstance *arr = value.asArray();
            if (!arr)
            {
                Error("%s arg %d expects Vector2[]", fn, argIndex);
                return false;
            }

            if (minCount > 0 && (int)arr->values.size() < minCount)
            {
                Error("%s arg %d expects at least %d points", fn, argIndex, minCount);
                return false;
            }

            out->clear();
            out->reserve(arr->values.size());
            for (size_t i = 0; i < arr->values.size(); ++i)
            {
                Vector2 point;
                if (!read_vec2_arg(vm, arr->values[i], &point, fn, argIndex))
                    return false;
                out->push_back(point);
            }
            return true;
        }

        static bool read_texvertex_array_arg(const Value &value, std::vector<TexVertex> *out, const char *fn, int argIndex, int minCount = 0)
        {
            if (!out)
                return false;

            if (!value.isArray())
            {
                Error("%s arg %d expects [[x, y, u, v], ...]", fn, argIndex);
                return false;
            }

            ArrayInstance *arr = value.asArray();
            if (!arr)
            {
                Error("%s arg %d expects [[x, y, u, v], ...]", fn, argIndex);
                return false;
            }

            if (minCount > 0 && (int)arr->values.size() < minCount)
            {
                Error("%s arg %d expects at least %d vertices", fn, argIndex, minCount);
                return false;
            }

            out->clear();
            out->reserve(arr->values.size());
            for (size_t i = 0; i < arr->values.size(); ++i)
            {
                const Value &item = arr->values[i];
                if (!item.isArray())
                {
                    Error("%s arg %d vertex %d expects [x, y, u, v]", fn, argIndex, (int)i + 1);
                    return false;
                }

                ArrayInstance *vertex = item.asArray();
                if (!vertex || vertex->values.size() < 4)
                {
                    Error("%s arg %d vertex %d expects [x, y, u, v]", fn, argIndex, (int)i + 1);
                    return false;
                }

                TexVertex texVertex{};
                texVertex.position.x = (float)vertex->values[0].asNumber();
                texVertex.position.y = (float)vertex->values[1].asNumber();
                texVertex.texCoord.x = (float)vertex->values[2].asNumber();
                texVertex.texCoord.y = (float)vertex->values[3].asNumber();
                out->push_back(texVertex);
            }
            return true;
        }

        static bool read_hermite_point_array_arg(Interpreter *vm, const Value &value, std::vector<HermitePoint> *out, const char *fn, int argIndex, int minCount = 0)
        {
            if (!out)
                return false;

            if (!value.isArray())
            {
                Error("%s arg %d expects [[x, y, tx, ty], ...] or [[pos], [tangent]]", fn, argIndex);
                return false;
            }

            ArrayInstance *arr = value.asArray();
            if (!arr)
            {
                Error("%s arg %d expects [[x, y, tx, ty], ...] or [[pos], [tangent]]", fn, argIndex);
                return false;
            }

            if (minCount > 0 && (int)arr->values.size() < minCount)
            {
                Error("%s arg %d expects at least %d points", fn, argIndex, minCount);
                return false;
            }

            out->clear();
            out->reserve(arr->values.size());
            for (size_t i = 0; i < arr->values.size(); ++i)
            {
                const Value &item = arr->values[i];
                if (!item.isArray())
                {
                    Error("%s arg %d point %d expects [x, y, tx, ty] or [Vector2, Vector2]", fn, argIndex, (int)i + 1);
                    return false;
                }

                ArrayInstance *pointData = item.asArray();
                if (!pointData)
                {
                    Error("%s arg %d point %d expects [x, y, tx, ty] or [Vector2, Vector2]", fn, argIndex, (int)i + 1);
                    return false;
                }

                HermitePoint point{};
                if (pointData->values.size() >= 4 && pointData->values[0].isNumber())
                {
                    point.position.x = (float)pointData->values[0].asNumber();
                    point.position.y = (float)pointData->values[1].asNumber();
                    point.tangent.x = (float)pointData->values[2].asNumber();
                    point.tangent.y = (float)pointData->values[3].asNumber();
                }
                else if (pointData->values.size() >= 2 &&
                         read_vec2_arg(vm, pointData->values[0], &point.position, fn, argIndex) &&
                         read_vec2_arg(vm, pointData->values[1], &point.tangent, fn, argIndex))
                {
                }
                else
                {
                    Error("%s arg %d point %d expects [x, y, tx, ty] or [Vector2, Vector2]", fn, argIndex, (int)i + 1);
                    return false;
                }

                out->push_back(point);
            }
            return true;
        }

        static void *batch_ctor(Interpreter *vm, int argCount, Value *args)
        {
            int numBuffers = 1;
            int bufferElements = 10000;

            if (argCount > 2)
            {
                Error("Batch expects () or (numBuffers[, bufferElements])");
                return nullptr;
            }

            if (argCount >= 1)
            {
                if (!args[0].isNumber())
                {
                    Error("Batch arg 1 expects number");
                    return nullptr;
                }
                numBuffers = args[0].asInt();
            }

            if (argCount == 2)
            {
                if (!args[1].isNumber())
                {
                    Error("Batch arg 2 expects number");
                    return nullptr;
                }
                bufferElements = args[1].asInt();
            }

            if (numBuffers <= 0 || bufferElements <= 0)
            {
                Error("Batch expects positive numBuffers and bufferElements");
                return nullptr;
            }

            BatchHandle *handle = new BatchHandle();
            handle->batch.Init(numBuffers, bufferElements);
            handle->initialized = true;
            (void)vm;
            return handle;
        }

        static void batch_dtor(Interpreter *vm, void *instance)
        {
            (void)vm;
            BatchHandle *handle = as_batch(instance);
            if (!handle)
                return;

            if (handle->initialized)
            {
                handle->batch.Release();
                handle->initialized = false;
            }

            delete handle;
        }

        static int batch_release(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            (void)args;
            BatchHandle *handle = as_batch(instance);
            if (!handle || argCount != 0)
            {
                Error("Batch.release() expects no arguments");
                return 0;
            }

            if (handle->initialized)
            {
                handle->batch.Release();
                handle->initialized = false;
            }
            return 0;
        }

        static int batch_render(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            (void)args;
            BatchHandle *handle = as_batch(instance);
            if (!handle || argCount != 0)
            {
                Error("Batch.render() expects no arguments");
                return 0;
            }

            handle->batch.Render();
            return 0;
        }

        static int batch_set_mode(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            if (!handle || argCount != 1 || !args[0].isNumber())
            {
                Error("Batch.setMode() expects (mode)");
                return 0;
            }

            handle->batch.SetMode(args[0].asInt());
            return 0;
        }

        static int batch_set_matrix(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            Matrix matrix;
            if (!handle || argCount != 1 || !read_mat4_arg(vm, args[0], &matrix, "Batch.setMatrix()", 1))
                return 0;

            handle->batch.SetMatrix(matrix);
            return 0;
        }

        static int batch_begin_transform(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            Matrix matrix;
            if (!handle || argCount != 1 || !read_mat4_arg(vm, args[0], &matrix, "Batch.beginTransform()", 1))
                return 0;

            handle->batch.BeginTransform(matrix);
            return 0;
        }

        static int batch_end_transform(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            (void)args;
            BatchHandle *handle = as_batch(instance);
            if (!handle || argCount != 0)
            {
                Error("Batch.endTransform() expects no arguments");
                return 0;
            }

            handle->batch.EndTransform();
            return 0;
        }

        static int batch_set_color(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            if (!handle || (argCount != 3 && argCount != 4))
            {
                Error("Batch.setColor() expects (r, g, b[, a])");
                return 0;
            }

            for (int i = 0; i < argCount; ++i)
            {
                if (!args[i].isNumber())
                {
                    Error("Batch.setColor() expects numeric arguments");
                    return 0;
                }
            }

            handle->batch.SetColor((float)args[0].asNumber(),
                                   (float)args[1].asNumber(),
                                   (float)args[2].asNumber());
            if (argCount == 4)
                handle->batch.SetAlpha((float)args[3].asNumber());
            return 0;
        }

        static int batch_set_color_bytes(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            if (!handle || argCount != 4)
            {
                Error("Batch.setColorBytes() expects (r, g, b, a)");
                return 0;
            }

            for (int i = 0; i < 4; ++i)
            {
                if (!args[i].isNumber())
                {
                    Error("Batch.setColorBytes() expects numeric arguments");
                    return 0;
                }
            }

            handle->batch.SetColor((u8)args[0].asInt(),
                                   (u8)args[1].asInt(),
                                   (u8)args[2].asInt(),
                                   (u8)args[3].asInt());
            return 0;
        }

        static int batch_set_alpha(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            if (!handle || argCount != 1 || !args[0].isNumber())
            {
                Error("Batch.setAlpha() expects (alpha)");
                return 0;
            }

            handle->batch.SetAlpha((float)args[0].asNumber());
            return 0;
        }

        static int batch_set_texture(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            if (!handle || argCount != 1 || !args[0].isNumber())
            {
                Error("Batch.setTexture() expects (textureId)");
                return 0;
            }

            handle->batch.SetTexture((unsigned int)args[0].asInt());
            return 0;
        }

        static int batch_line2d(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            if (!handle)
                return 0;

            if (argCount == 4)
            {
                for (int i = 0; i < 4; ++i)
                {
                    if (!args[i].isNumber())
                    {
                        Error("Batch.line2D() expects (x1, y1, x2, y2) or (Vector2, Vector2)");
                        return 0;
                    }
                }

                handle->batch.Line2D(args[0].asInt(), args[1].asInt(), args[2].asInt(), args[3].asInt());
                return 0;
            }

            if (argCount == 2)
            {
                Vector2 start;
                Vector2 end;
                if (!read_vec2_arg(vm, args[0], &start, "Batch.line2D()", 1) ||
                    !read_vec2_arg(vm, args[1], &end, "Batch.line2D()", 2))
                    return 0;

                handle->batch.Line2D(start, end);
                return 0;
            }

            Error("Batch.line2D() expects (x1, y1, x2, y2) or (Vector2, Vector2)");
            return 0;
        }

        static int batch_line3d(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            if (!handle)
                return 0;

            if (argCount == 6)
            {
                for (int i = 0; i < 6; ++i)
                {
                    if (!args[i].isNumber())
                    {
                        Error("Batch.line3D() expects (x1, y1, z1, x2, y2, z2) or (Vector3, Vector3)");
                        return 0;
                    }
                }

                handle->batch.Line3D((float)args[0].asNumber(), (float)args[1].asNumber(), (float)args[2].asNumber(),
                                     (float)args[3].asNumber(), (float)args[4].asNumber(), (float)args[5].asNumber());
                return 0;
            }

            if (argCount == 2)
            {
                Vector3 start;
                Vector3 end;
                if (!read_vec3_arg(vm, args[0], &start, "Batch.line3D()", 1) ||
                    !read_vec3_arg(vm, args[1], &end, "Batch.line3D()", 2))
                    return 0;

                handle->batch.Line3D(start, end);
                return 0;
            }

            Error("Batch.line3D() expects (x1, y1, z1, x2, y2, z2) or (Vector3, Vector3)");
            return 0;
        }

        static int batch_rectangle(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            bool fill = false;
            if (!handle || argCount < 4 || argCount > 5)
            {
                Error("Batch.rectangle() expects (x, y, width, height[, fill])");
                return 0;
            }

            for (int i = 0; i < 4; ++i)
            {
                if (!args[i].isNumber())
                {
                    Error("Batch.rectangle() expects numeric position/size");
                    return 0;
                }
            }

            if (argCount == 5 && !read_boolish(args[4], &fill, "Batch.rectangle()", 5))
                return 0;

            handle->batch.Rectangle(args[0].asInt(), args[1].asInt(), args[2].asInt(), args[3].asInt(), fill);
            return 0;
        }

        static int batch_circle(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            bool fill = false;
            if (!handle || argCount < 3 || argCount > 4)
            {
                Error("Batch.circle() expects (x, y, radius[, fill])");
                return 0;
            }

            if (!args[0].isNumber() || !args[1].isNumber() || !args[2].isNumber())
            {
                Error("Batch.circle() expects numeric center/radius");
                return 0;
            }

            if (argCount == 4 && !read_boolish(args[3], &fill, "Batch.circle()", 4))
                return 0;

            handle->batch.Circle(args[0].asInt(), args[1].asInt(), (float)args[2].asNumber(), fill);
            return 0;
        }

        static int batch_triangle(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            if (!handle)
                return 0;

            if (argCount == 3)
            {
                Vector3 a, b, c;
                if (!read_vec3_arg(vm, args[0], &a, "Batch.triangle()", 1) ||
                    !read_vec3_arg(vm, args[1], &b, "Batch.triangle()", 2) ||
                    !read_vec3_arg(vm, args[2], &c, "Batch.triangle()", 3))
                    return 0;

                handle->batch.Triangle(a, b, c);
                return 0;
            }

            if (argCount == 6 || argCount == 7)
            {
                bool fill = true;
                for (int i = 0; i < 6; ++i)
                {
                    if (!args[i].isNumber())
                    {
                        Error("Batch.triangle() expects (Vector3, Vector3, Vector3) or (x1, y1, x2, y2, x3, y3[, fill])");
                        return 0;
                    }
                }
                if (argCount == 7 && !read_boolish(args[6], &fill, "Batch.triangle()", 7))
                    return 0;

                handle->batch.Triangle((float)args[0].asNumber(), (float)args[1].asNumber(),
                                       (float)args[2].asNumber(), (float)args[3].asNumber(),
                                       (float)args[4].asNumber(), (float)args[5].asNumber(),
                                       fill);
                return 0;
            }

            Error("Batch.triangle() expects (Vector3, Vector3, Vector3) or (x1, y1, x2, y2, x3, y3[, fill])");
            return 0;
        }

        static int batch_triangle_lines(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            Vector3 a, b, c;
            if (!handle || argCount != 3 ||
                !read_vec3_arg(vm, args[0], &a, "Batch.triangleLines()", 1) ||
                !read_vec3_arg(vm, args[1], &b, "Batch.triangleLines()", 2) ||
                !read_vec3_arg(vm, args[2], &c, "Batch.triangleLines()", 3))
                return 0;

            handle->batch.TriangleLines(a, b, c);
            return 0;
        }

        static int batch_grid(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            bool axes = true;
            if (!handle || argCount < 2 || argCount > 3 || !args[0].isNumber() || !args[1].isNumber())
            {
                Error("Batch.grid() expects (slices, spacing[, axes])");
                return 0;
            }

            if (argCount == 3 && !read_boolish(args[2], &axes, "Batch.grid()", 3))
                return 0;

            handle->batch.Grid(args[0].asInt(), (float)args[1].asNumber(), axes);
            return 0;
        }

        static int batch_cube(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            Vector3 position;
            bool wire = true;
            if (!handle || argCount < 4 || argCount > 5)
            {
                Error("Batch.cube() expects (position, width, height, depth[, wire])");
                return 0;
            }

            if (!read_vec3_arg(vm, args[0], &position, "Batch.cube()", 1) ||
                !args[1].isNumber() || !args[2].isNumber() || !args[3].isNumber())
            {
                Error("Batch.cube() expects (position, width, height, depth[, wire])");
                return 0;
            }

            if (argCount == 5 && !read_boolish(args[4], &wire, "Batch.cube()", 5))
                return 0;

            handle->batch.Cube(position, (float)args[1].asNumber(), (float)args[2].asNumber(), (float)args[3].asNumber(), wire);
            return 0;
        }

        static int batch_sphere(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            Vector3 position;
            bool wire = true;
            if (!handle || argCount < 4 || argCount > 5)
            {
                Error("Batch.sphere() expects (position, radius, rings, slices[, wire])");
                return 0;
            }

            if (!read_vec3_arg(vm, args[0], &position, "Batch.sphere()", 1) ||
                !args[1].isNumber() || !args[2].isNumber() || !args[3].isNumber())
            {
                Error("Batch.sphere() expects (position, radius, rings, slices[, wire])");
                return 0;
            }

            if (argCount == 5 && !read_boolish(args[4], &wire, "Batch.sphere()", 5))
                return 0;

            handle->batch.Sphere(position, (float)args[1].asNumber(), args[2].asInt(), args[3].asInt(), wire);
            return 0;
        }

        static int batch_cone(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            Vector3 position;
            bool wire = true;
            if (!handle || argCount < 4 || argCount > 5)
            {
                Error("Batch.cone() expects (position, radius, height, segments[, wire])");
                return 0;
            }

            if (!read_vec3_arg(vm, args[0], &position, "Batch.cone()", 1) ||
                !args[1].isNumber() || !args[2].isNumber() || !args[3].isNumber())
            {
                Error("Batch.cone() expects (position, radius, height, segments[, wire])");
                return 0;
            }

            if (argCount == 5 && !read_boolish(args[4], &wire, "Batch.cone()", 5))
                return 0;

            handle->batch.Cone(position, (float)args[1].asNumber(), (float)args[2].asNumber(), args[3].asInt(), wire);
            return 0;
        }

        static int batch_cylinder(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            Vector3 position;
            bool wire = true;
            if (!handle || argCount < 4 || argCount > 5)
            {
                Error("Batch.cylinder() expects (position, radius, height, segments[, wire])");
                return 0;
            }

            if (!read_vec3_arg(vm, args[0], &position, "Batch.cylinder()", 1) ||
                !args[1].isNumber() || !args[2].isNumber() || !args[3].isNumber())
            {
                Error("Batch.cylinder() expects (position, radius, height, segments[, wire])");
                return 0;
            }

            if (argCount == 5 && !read_boolish(args[4], &wire, "Batch.cylinder()", 5))
                return 0;

            handle->batch.Cylinder(position, (float)args[1].asNumber(), (float)args[2].asNumber(), args[3].asInt(), wire);
            return 0;
        }

        static int batch_quad(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            (void)vm;
            BatchHandle *handle = as_batch(instance);
            if (!handle)
                return 0;

            if (argCount == 4)
            {
                for (int i = 0; i < 4; ++i)
                {
                    if (!args[i].isNumber())
                    {
                        Error("Batch.quad() expects (x, y, width, height) or (textureId, x, y, width, height)");
                        return 0;
                    }
                }

                handle->batch.Quad(0u, (float)args[0].asNumber(), (float)args[1].asNumber(),
                                   (float)args[2].asNumber(), (float)args[3].asNumber());
                return 0;
            }

            if (argCount == 5)
            {
                for (int i = 0; i < 5; ++i)
                {
                    if (!args[i].isNumber())
                    {
                        Error("Batch.quad() expects (x, y, width, height) or (textureId, x, y, width, height)");
                        return 0;
                    }
                }

                handle->batch.Quad((u32)args[0].asInt(), (float)args[1].asNumber(), (float)args[2].asNumber(),
                                   (float)args[3].asNumber(), (float)args[4].asNumber());
                return 0;
            }

            Error("Batch.quad() expects (x, y, width, height) or (textureId, x, y, width, height)");
            return 0;
        }

        static int batch_ellipse(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            bool fill = true;
            if (!handle || argCount < 4 || argCount > 5)
            {
                Error("Batch.ellipse() expects (x, y, radiusX, radiusY[, fill])");
                return 0;
            }

            for (int i = 0; i < 4; ++i)
            {
                if (!args[i].isNumber())
                {
                    Error("Batch.ellipse() expects numeric arguments");
                    return 0;
                }
            }
            if (argCount == 5 && !read_boolish(args[4], &fill, "Batch.ellipse()", 5))
                return 0;

            handle->batch.Ellipse(args[0].asInt(), args[1].asInt(), (float)args[2].asNumber(), (float)args[3].asNumber(), fill);
            return 0;
        }

        static int batch_polygon(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            bool fill = true;
            float rotation = 0.0f;
            if (!handle || argCount < 4 || argCount > 6)
            {
                Error("Batch.polygon() expects (x, y, sides, radius[, rotation[, fill]])");
                return 0;
            }

            for (int i = 0; i < 4; ++i)
            {
                if (!args[i].isNumber())
                {
                    Error("Batch.polygon() expects numeric arguments");
                    return 0;
                }
            }
            if (argCount >= 5)
            {
                if (!args[4].isNumber())
                {
                    Error("Batch.polygon() rotation must be numeric");
                    return 0;
                }
                rotation = (float)args[4].asNumber();
            }
            if (argCount == 6 && !read_boolish(args[5], &fill, "Batch.polygon()", 6))
                return 0;

            handle->batch.Polygon(args[0].asInt(), args[1].asInt(), args[2].asInt(), (float)args[3].asNumber(), rotation, fill);
            return 0;
        }

        static int batch_rounded_rectangle(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            int segments = 8;
            bool fill = true;
            if (!handle || argCount < 5 || argCount > 7)
            {
                Error("Batch.roundedRectangle() expects (x, y, width, height, roundness[, segments[, fill]])");
                return 0;
            }

            for (int i = 0; i < 5; ++i)
            {
                if (!args[i].isNumber())
                {
                    Error("Batch.roundedRectangle() expects numeric position/size/roundness");
                    return 0;
                }
            }
            if (argCount >= 6)
            {
                if (!args[5].isNumber())
                {
                    Error("Batch.roundedRectangle() segments must be numeric");
                    return 0;
                }
                segments = args[5].asInt();
            }
            if (argCount == 7 && !read_boolish(args[6], &fill, "Batch.roundedRectangle()", 7))
                return 0;

            handle->batch.RoundedRectangle(args[0].asInt(), args[1].asInt(), args[2].asInt(), args[3].asInt(),
                                           (float)args[4].asNumber(), segments, fill);
            return 0;
        }

        static int batch_circle_sector(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            int segments = 16;
            bool fill = true;
            if (!handle || argCount < 5 || argCount > 7)
            {
                Error("Batch.circleSector() expects (x, y, radius, startAngle, endAngle[, segments[, fill]])");
                return 0;
            }

            for (int i = 0; i < 5; ++i)
            {
                if (!args[i].isNumber())
                {
                    Error("Batch.circleSector() expects numeric arguments");
                    return 0;
                }
            }
            if (argCount >= 6)
            {
                if (!args[5].isNumber())
                {
                    Error("Batch.circleSector() segments must be numeric");
                    return 0;
                }
                segments = args[5].asInt();
            }
            if (argCount == 7 && !read_boolish(args[6], &fill, "Batch.circleSector()", 7))
                return 0;

            handle->batch.CircleSector(args[0].asInt(), args[1].asInt(), (float)args[2].asNumber(),
                                       (float)args[3].asNumber(), (float)args[4].asNumber(), segments, fill);
            return 0;
        }

        static int batch_polyline(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            bool closed = false;
            std::vector<Vector2> points;
            if (!handle || argCount < 1 || argCount > 2)
            {
                Error("Batch.polyline() expects (points[, closed])");
                return 0;
            }

            if (!read_vec2_array_arg(vm, args[0], &points, "Batch.polyline()", 1, 2))
                return 0;

            if (argCount == 2 && !read_boolish(args[1], &closed, "Batch.polyline()", 2))
                return 0;

            if (closed && points.size() >= 2)
            {
                points.push_back(points.front());
            }

            handle->batch.Polyline(points.data(), (int)points.size());
            return 0;
        }

        static int batch_draw_quad(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            if (!handle)
                return 0;

            if (argCount == 8)
            {
                for (int i = 0; i < 8; ++i)
                {
                    if (!args[i].isNumber())
                    {
                        Error("Batch.drawQuad() expects (x1, y1, x2, y2, u0, v0, u1, v1) or (textureId, x1, y1, x2, y2, u0, v0, u1, v1)");
                        return 0;
                    }
                }
                handle->batch.DrawQuad((float)args[0].asNumber(), (float)args[1].asNumber(),
                                       (float)args[2].asNumber(), (float)args[3].asNumber(),
                                       (float)args[4].asNumber(), (float)args[5].asNumber(),
                                       (float)args[6].asNumber(), (float)args[7].asNumber());
                return 0;
            }

            if (argCount == 9)
            {
                for (int i = 0; i < 9; ++i)
                {
                    if (!args[i].isNumber())
                    {
                        Error("Batch.drawQuad() expects (x1, y1, x2, y2, u0, v0, u1, v1) or (textureId, x1, y1, x2, y2, u0, v0, u1, v1)");
                        return 0;
                    }
                }
                handle->batch.DrawQuad((u32)args[0].asInt(),
                                       (float)args[1].asNumber(), (float)args[2].asNumber(),
                                       (float)args[3].asNumber(), (float)args[4].asNumber(),
                                       (float)args[5].asNumber(), (float)args[6].asNumber(),
                                       (float)args[7].asNumber(), (float)args[8].asNumber());
                return 0;
            }

            Error("Batch.drawQuad() expects (x1, y1, x2, y2, u0, v0, u1, v1) or (textureId, x1, y1, x2, y2, u0, v0, u1, v1)");
            return 0;
        }

        static int batch_quad_clip(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            (void)vm;
            BatchHandle *handle = as_batch(instance);
            if (!handle || argCount != 11)
            {
                Error("Batch.quadClip() expects (textureId, srcX, srcY, srcW, srcH, x, y, width, height, textureWidth, textureHeight)");
                return 0;
            }

            for (int i = 0; i < 11; ++i)
            {
                if (!args[i].isNumber())
                {
                    Error("Batch.quadClip() expects numeric arguments");
                    return 0;
                }
            }

            FloatRect src((float)args[1].asNumber(), (float)args[2].asNumber(),
                          (float)args[3].asNumber(), (float)args[4].asNumber());
            handle->batch.Quad((u32)args[0].asInt(),
                               src,
                               (float)args[5].asNumber(), (float)args[6].asNumber(),
                               (float)args[7].asNumber(), (float)args[8].asNumber(),
                               (float)args[9].asNumber(), (float)args[10].asNumber());
            return 0;
        }

        static int batch_textured_quad(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            if (!handle)
                return 0;

            Vector2 p1, p2, p3, p4;
            if (argCount == 5)
            {
                if (!read_vec2_arg(vm, args[0], &p1, "Batch.texturedQuad()", 1) ||
                    !read_vec2_arg(vm, args[1], &p2, "Batch.texturedQuad()", 2) ||
                    !read_vec2_arg(vm, args[2], &p3, "Batch.texturedQuad()", 3) ||
                    !read_vec2_arg(vm, args[3], &p4, "Batch.texturedQuad()", 4) ||
                    !args[4].isNumber())
                {
                    Error("Batch.texturedQuad() expects (p1, p2, p3, p4, textureId) or (p1, p2, p3, p4, uv1, uv2, uv3, uv4, textureId)");
                    return 0;
                }

                handle->batch.TexturedQuad(p1, p2, p3, p4, (u32)args[4].asInt());
                return 0;
            }

            if (argCount == 9)
            {
                Vector2 uv1, uv2, uv3, uv4;
                if (!read_vec2_arg(vm, args[0], &p1, "Batch.texturedQuad()", 1) ||
                    !read_vec2_arg(vm, args[1], &p2, "Batch.texturedQuad()", 2) ||
                    !read_vec2_arg(vm, args[2], &p3, "Batch.texturedQuad()", 3) ||
                    !read_vec2_arg(vm, args[3], &p4, "Batch.texturedQuad()", 4) ||
                    !read_vec2_arg(vm, args[4], &uv1, "Batch.texturedQuad()", 5) ||
                    !read_vec2_arg(vm, args[5], &uv2, "Batch.texturedQuad()", 6) ||
                    !read_vec2_arg(vm, args[6], &uv3, "Batch.texturedQuad()", 7) ||
                    !read_vec2_arg(vm, args[7], &uv4, "Batch.texturedQuad()", 8) ||
                    !args[8].isNumber())
                {
                    Error("Batch.texturedQuad() expects (p1, p2, p3, p4, textureId) or (p1, p2, p3, p4, uv1, uv2, uv3, uv4, textureId)");
                    return 0;
                }

                handle->batch.TexturedQuad(p1, p2, p3, p4, uv1, uv2, uv3, uv4, (u32)args[8].asInt());
                return 0;
            }

            Error("Batch.texturedQuad() expects (p1, p2, p3, p4, textureId) or (p1, p2, p3, p4, uv1, uv2, uv3, uv4, textureId)");
            return 0;
        }

        static int batch_textured_triangle(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            if (!handle)
                return 0;

            Vector2 p1, p2, p3;
            if (argCount == 4)
            {
                if (!read_vec2_arg(vm, args[0], &p1, "Batch.texturedTriangle()", 1) ||
                    !read_vec2_arg(vm, args[1], &p2, "Batch.texturedTriangle()", 2) ||
                    !read_vec2_arg(vm, args[2], &p3, "Batch.texturedTriangle()", 3) ||
                    !args[3].isNumber())
                {
                    Error("Batch.texturedTriangle() expects (p1, p2, p3, textureId) or (p1, p2, p3, uv1, uv2, uv3, textureId)");
                    return 0;
                }

                handle->batch.TexturedTriangle(p1, p2, p3, (u32)args[3].asInt());
                return 0;
            }

            if (argCount == 7)
            {
                Vector2 uv1, uv2, uv3;
                if (!read_vec2_arg(vm, args[0], &p1, "Batch.texturedTriangle()", 1) ||
                    !read_vec2_arg(vm, args[1], &p2, "Batch.texturedTriangle()", 2) ||
                    !read_vec2_arg(vm, args[2], &p3, "Batch.texturedTriangle()", 3) ||
                    !read_vec2_arg(vm, args[3], &uv1, "Batch.texturedTriangle()", 4) ||
                    !read_vec2_arg(vm, args[4], &uv2, "Batch.texturedTriangle()", 5) ||
                    !read_vec2_arg(vm, args[5], &uv3, "Batch.texturedTriangle()", 6) ||
                    !args[6].isNumber())
                {
                    Error("Batch.texturedTriangle() expects (p1, p2, p3, textureId) or (p1, p2, p3, uv1, uv2, uv3, textureId)");
                    return 0;
                }

                handle->batch.TexturedTriangle(p1, p2, p3, uv1, uv2, uv3, (u32)args[6].asInt());
                return 0;
            }

            Error("Batch.texturedTriangle() expects (p1, p2, p3, textureId) or (p1, p2, p3, uv1, uv2, uv3, textureId)");
            return 0;
        }

        static int batch_textured_polygon(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            std::vector<Vector2> points;
            if (!handle || argCount != 2 || !args[1].isNumber())
            {
                Error("Batch.texturedPolygon() expects (points, textureId)");
                return 0;
            }

            if (!read_vec2_array_arg(vm, args[0], &points, "Batch.texturedPolygon()", 1, 3))
                return 0;

            handle->batch.TexturedPolygon(points.data(), (int)points.size(), (u32)args[1].asInt());
            return 0;
        }

        static int batch_textured_polygon_uv(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            (void)vm;
            BatchHandle *handle = as_batch(instance);
            std::vector<TexVertex> vertices;
            if (!handle || argCount != 2 || !args[1].isNumber())
            {
                Error("Batch.texturedPolygonUV() expects (vertices, textureId)");
                return 0;
            }

            if (!read_texvertex_array_arg(args[0], &vertices, "Batch.texturedPolygonUV()", 1, 3))
                return 0;

            handle->batch.TexturedPolygonCustomUV(vertices.data(), (int)vertices.size(), (u32)args[1].asInt());
            return 0;
        }

        static int batch_bezier_quadratic(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            Vector2 p0, p1, p2;
            int segments = 20;
            if (!handle || argCount < 3 || argCount > 4 ||
                !read_vec2_arg(vm, args[0], &p0, "Batch.bezierQuadratic()", 1) ||
                !read_vec2_arg(vm, args[1], &p1, "Batch.bezierQuadratic()", 2) ||
                !read_vec2_arg(vm, args[2], &p2, "Batch.bezierQuadratic()", 3))
                return 0;

            if (argCount == 4)
            {
                if (!args[3].isNumber())
                {
                    Error("Batch.bezierQuadratic() segments must be numeric");
                    return 0;
                }
                segments = args[3].asInt();
            }

            handle->batch.BezierQuadratic(p0, p1, p2, segments);
            return 0;
        }

        static int batch_bezier_cubic(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            Vector2 p0, p1, p2, p3;
            int segments = 30;
            if (!handle || argCount < 4 || argCount > 5 ||
                !read_vec2_arg(vm, args[0], &p0, "Batch.bezierCubic()", 1) ||
                !read_vec2_arg(vm, args[1], &p1, "Batch.bezierCubic()", 2) ||
                !read_vec2_arg(vm, args[2], &p2, "Batch.bezierCubic()", 3) ||
                !read_vec2_arg(vm, args[3], &p3, "Batch.bezierCubic()", 4))
                return 0;

            if (argCount == 5)
            {
                if (!args[4].isNumber())
                {
                    Error("Batch.bezierCubic() segments must be numeric");
                    return 0;
                }
                segments = args[4].asInt();
            }

            handle->batch.BezierCubic(p0, p1, p2, p3, segments);
            return 0;
        }

        static int batch_catmull_rom_spline(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            std::vector<Vector2> points;
            int segments = 20;
            if (!handle || argCount < 1 || argCount > 2)
            {
                Error("Batch.catmullRomSpline() expects (points[, segments])");
                return 0;
            }

            if (!read_vec2_array_arg(vm, args[0], &points, "Batch.catmullRomSpline()", 1, 4))
                return 0;

            if (argCount == 2)
            {
                if (!args[1].isNumber())
                {
                    Error("Batch.catmullRomSpline() segments must be numeric");
                    return 0;
                }
                segments = args[1].asInt();
            }

            handle->batch.CatmullRomSpline(points.data(), (int)points.size(), segments);
            return 0;
        }

        static int batch_bspline(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            std::vector<Vector2> points;
            int segments = 20;
            int degree = 3;
            if (!handle || argCount < 1 || argCount > 3)
            {
                Error("Batch.bspline() expects (points[, segments[, degree]])");
                return 0;
            }

            if (!read_vec2_array_arg(vm, args[0], &points, "Batch.bspline()", 1, 4))
                return 0;

            if (argCount >= 2)
            {
                if (!args[1].isNumber())
                {
                    Error("Batch.bspline() segments must be numeric");
                    return 0;
                }
                segments = args[1].asInt();
            }

            if (argCount == 3)
            {
                if (!args[2].isNumber())
                {
                    Error("Batch.bspline() degree must be numeric");
                    return 0;
                }
                degree = args[2].asInt();
            }

            handle->batch.BSpline(points.data(), (int)points.size(), segments, degree);
            return 0;
        }

        static int batch_hermite_spline(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            std::vector<HermitePoint> points;
            int segments = 20;
            if (!handle || argCount < 1 || argCount > 2)
            {
                Error("Batch.hermiteSpline() expects (points[, segments])");
                return 0;
            }

            if (!read_hermite_point_array_arg(vm, args[0], &points, "Batch.hermiteSpline()", 1, 2))
                return 0;

            if (argCount == 2)
            {
                if (!args[1].isNumber())
                {
                    Error("Batch.hermiteSpline() segments must be numeric");
                    return 0;
                }
                segments = args[1].asInt();
            }

            handle->batch.HermiteSpline(points.data(), (int)points.size(), segments);
            return 0;
        }

        static int batch_thick_spline(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            std::vector<Vector2> points;
            int segments = 20;
            if (!handle || argCount < 2 || argCount > 3)
            {
                Error("Batch.thickSpline() expects (points, thickness[, segments])");
                return 0;
            }

            if (!read_vec2_array_arg(vm, args[0], &points, "Batch.thickSpline()", 1, 2))
                return 0;

            if (!args[1].isNumber())
            {
                Error("Batch.thickSpline() thickness must be numeric");
                return 0;
            }

            if (argCount == 3)
            {
                if (!args[2].isNumber())
                {
                    Error("Batch.thickSpline() segments must be numeric");
                    return 0;
                }
                segments = args[2].asInt();
            }

            handle->batch.ThickSpline(points.data(), (int)points.size(), (float)args[1].asNumber(), segments);
            return 0;
        }

        static int batch_vertex2f(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            if (!handle || argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
            {
                Error("Batch.vertex2f() expects (x, y)");
                return 0;
            }

            handle->batch.Vertex2f((float)args[0].asNumber(), (float)args[1].asNumber());
            return 0;
        }

        static int batch_vertex3f(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            if (!handle || argCount != 3 || !args[0].isNumber() || !args[1].isNumber() || !args[2].isNumber())
            {
                Error("Batch.vertex3f() expects (x, y, z)");
                return 0;
            }

            handle->batch.Vertex3f((float)args[0].asNumber(), (float)args[1].asNumber(), (float)args[2].asNumber());
            return 0;
        }

        static int batch_texcoord2f(Interpreter *vm, void *instance, int argCount, Value *args)
        {
            BatchHandle *handle = as_batch(instance);
            if (!handle || argCount != 2 || !args[0].isNumber() || !args[1].isNumber())
            {
                Error("Batch.texCoord2f() expects (u, v)");
                return 0;
            }

            handle->batch.TexCoord2f((float)args[0].asNumber(), (float)args[1].asNumber());
            return 0;
        }
    }

        // ─── SpriteFont ───────────────────────────────────────────────────────

        struct FontHandle
        {
            SpriteFont font;
        };

        static FontHandle *as_font(void *p) { return (FontHandle *)p; }

        static void *font_ctor(Interpreter *vm, int argCount, Value *args)
        {
            (void)vm; (void)args;
            if (argCount != 0)
            {
                Error("SpriteFont() expects no arguments");
                return nullptr;
            }
            return new FontHandle();
        }

        static void font_dtor(Interpreter *vm, void *p)
        {
            (void)vm;
            FontHandle *h = as_font(p);
            if (!h) return;
            h->font.Release();
            delete h;
        }

        static int font_release(Interpreter *vm, void *p, int argc, Value *args)
        {
            (void)vm; (void)args;
            FontHandle *h = as_font(p);
            if (!h || argc != 0) { Error("SpriteFont.release() expects no arguments"); return 0; }
            h->font.Release();
            return 0;
        }

        static int font_load_default(Interpreter *vm, void *p, int argc, Value *args)
        {
            (void)args;
            FontHandle *h = as_font(p);
            if (!h || argc != 0) { Error("SpriteFont.loadDefault() expects no arguments"); return 0; }
            vm->pushBool(h->font.LoadDefaultFont());
            return 1;
        }

        static int font_load_bmfont(Interpreter *vm, void *p, int argc, Value *args)
        {
            FontHandle *h = as_font(p);
            if (!h || argc < 1 || argc > 2 || !args[0].isString())
            {
                Error("SpriteFont.loadBMFont(fntPath[, texturePath])");
                return 0;
            }
            const char *texPath = (argc == 2 && args[1].isString()) ? args[1].asStringChars() : nullptr;
            vm->pushBool(h->font.LoadBMFont(args[0].asStringChars(), texPath));
            return 1;
        }

        static int font_load_ttf(Interpreter *vm, void *p, int argc, Value *args)
        {
            FontHandle *h = as_font(p);
            if (!h || argc < 2 || argc > 3 || !args[0].isString() || !args[1].isNumber())
            {
                Error("SpriteFont.loadTTF(path, fontSize[, atlasSize])");
                return 0;
            }
            int atlasSize = (argc == 3 && args[2].isNumber()) ? args[2].asInt() : 512;
            vm->pushBool(h->font.LoadTTF(args[0].asStringChars(), (float)args[1].asNumber(), atlasSize));
            return 1;
        }

        static int font_set_batch(Interpreter *vm, void *p, int argc, Value *args)
        {
            (void)vm;
            FontHandle *h = as_font(p);
            if (!h || argc != 1 || !args[0].isNativeClassInstance())
            {
                Error("SpriteFont.setBatch(batch)");
                return 0;
            }
            NativeClassInstance *inst = args[0].asNativeClassInstance();
            if (!inst || !inst->userData) { Error("SpriteFont.setBatch: invalid batch"); return 0; }
            BatchHandle *bh = (BatchHandle *)inst->userData;
            h->font.SetBatch(&bh->batch);
            return 0;
        }

        static int font_set_color(Interpreter *vm, void *p, int argc, Value *args)
        {
            (void)vm;
            FontHandle *h = as_font(p);
            if (!h || (argc != 3 && argc != 4))
            {
                Error("SpriteFont.setColor(r, g, b[, a])");
                return 0;
            }
            u8 r = (u8)args[0].asInt();
            u8 g = (u8)args[1].asInt();
            u8 b = (u8)args[2].asInt();
            u8 a = (argc == 4) ? (u8)args[3].asInt() : 255;
            h->font.SetColor(r, g, b, a);
            return 0;
        }

        static int font_set_font_size(Interpreter *vm, void *p, int argc, Value *args)
        {
            (void)vm;
            FontHandle *h = as_font(p);
            if (!h || argc != 1 || !args[0].isNumber()) { Error("SpriteFont.setFontSize(size)"); return 0; }
            h->font.SetFontSize((float)args[0].asNumber());
            return 0;
        }

        static int font_set_spacing(Interpreter *vm, void *p, int argc, Value *args)
        {
            (void)vm;
            FontHandle *h = as_font(p);
            if (!h || argc != 1 || !args[0].isNumber()) { Error("SpriteFont.setSpacing(value)"); return 0; }
            h->font.SetSpacing((float)args[0].asNumber());
            return 0;
        }

        static int font_set_line_spacing(Interpreter *vm, void *p, int argc, Value *args)
        {
            (void)vm;
            FontHandle *h = as_font(p);
            if (!h || argc != 1 || !args[0].isNumber()) { Error("SpriteFont.setLineSpacing(value)"); return 0; }
            h->font.SetLineSpacing((float)args[0].asNumber());
            return 0;
        }

        static int font_print(Interpreter *vm, void *p, int argc, Value *args)
        {
            (void)vm;
            FontHandle *h = as_font(p);
            if (!h || argc < 3 || !args[0].isString() || !args[1].isNumber() || !args[2].isNumber())
            {
                Error("SpriteFont.print(text, x, y)");
                return 0;
            }
            h->font.Print(args[0].asStringChars(), (float)args[1].asNumber(), (float)args[2].asNumber());
            return 0;
        }

        static int font_print_wrapped(Interpreter *vm, void *p, int argc, Value *args)
        {
            (void)vm;
            FontHandle *h = as_font(p);
            if (!h || argc != 4 || !args[0].isString() ||
                !args[1].isNumber() || !args[2].isNumber() || !args[3].isNumber())
            {
                Error("SpriteFont.printWrapped(text, x, y, maxWidth)");
                return 0;
            }
            h->font.PrintWrapped(args[0].asStringChars(),
                                 (float)args[1].asNumber(),
                                 (float)args[2].asNumber(),
                                 (float)args[3].asNumber());
            return 0;
        }

        static int font_get_text_width(Interpreter *vm, void *p, int argc, Value *args)
        {
            FontHandle *h = as_font(p);
            if (!h || argc != 1 || !args[0].isString()) { Error("SpriteFont.getTextWidth(text)"); return 0; }
            vm->pushDouble(h->font.GetTextWidth(args[0].asStringChars()));
            return 1;
        }

        static int font_get_text_height(Interpreter *vm, void *p, int argc, Value *args)
        {
            FontHandle *h = as_font(p);
            if (!h || argc != 1 || !args[0].isString()) { Error("SpriteFont.getTextHeight(text)"); return 0; }
            vm->pushDouble(h->font.GetTextHeight(args[0].asStringChars()));
            return 1;
        }

        static int font_is_valid(Interpreter *vm, void *p, int argc, Value *args)
        {
            (void)args;
            FontHandle *h = as_font(p);
            if (!h || argc != 0) { Error("SpriteFont.isValid()"); return 0; }
            vm->pushBool(h->font.IsValid());
            return 1;
        }

    void register_batch(Interpreter &vm, ModuleBuilder &module)
    {
        NativeClassDef *batchClass = vm.registerNativeClass("Batch", batch_ctor, batch_dtor, 2, false);
        vm.addNativeMethod(batchClass, "release", batch_release);
        vm.addNativeMethod(batchClass, "render", batch_render);
        vm.addNativeMethod(batchClass, "setMode", batch_set_mode);
        vm.addNativeMethod(batchClass, "setMatrix", batch_set_matrix);
        vm.addNativeMethod(batchClass, "beginTransform", batch_begin_transform);
        vm.addNativeMethod(batchClass, "endTransform", batch_end_transform);
        vm.addNativeMethod(batchClass, "setColor", batch_set_color);
        vm.addNativeMethod(batchClass, "setColorBytes", batch_set_color_bytes);
        vm.addNativeMethod(batchClass, "setAlpha", batch_set_alpha);
        vm.addNativeMethod(batchClass, "setTexture", batch_set_texture);
        vm.addNativeMethod(batchClass, "line2D", batch_line2d);
        vm.addNativeMethod(batchClass, "line3D", batch_line3d);
        vm.addNativeMethod(batchClass, "rectangle", batch_rectangle);
        vm.addNativeMethod(batchClass, "circle", batch_circle);
        vm.addNativeMethod(batchClass, "triangle", batch_triangle);
        vm.addNativeMethod(batchClass, "triangleLines", batch_triangle_lines);
        vm.addNativeMethod(batchClass, "ellipse", batch_ellipse);
        vm.addNativeMethod(batchClass, "polygon", batch_polygon);
        vm.addNativeMethod(batchClass, "roundedRectangle", batch_rounded_rectangle);
        vm.addNativeMethod(batchClass, "circleSector", batch_circle_sector);
        vm.addNativeMethod(batchClass, "polyline", batch_polyline);
        vm.addNativeMethod(batchClass, "grid", batch_grid);
        vm.addNativeMethod(batchClass, "cube", batch_cube);
        vm.addNativeMethod(batchClass, "sphere", batch_sphere);
        vm.addNativeMethod(batchClass, "cone", batch_cone);
        vm.addNativeMethod(batchClass, "cylinder", batch_cylinder);
        vm.addNativeMethod(batchClass, "quad", batch_quad);
        vm.addNativeMethod(batchClass, "drawQuad", batch_draw_quad);
        vm.addNativeMethod(batchClass, "quadClip", batch_quad_clip);
        vm.addNativeMethod(batchClass, "texturedPolygon", batch_textured_polygon);
        vm.addNativeMethod(batchClass, "texturedPolygonUV", batch_textured_polygon_uv);
        vm.addNativeMethod(batchClass, "texturedQuad", batch_textured_quad);
        vm.addNativeMethod(batchClass, "texturedTriangle", batch_textured_triangle);
        vm.addNativeMethod(batchClass, "bezierQuadratic", batch_bezier_quadratic);
        vm.addNativeMethod(batchClass, "bezierCubic", batch_bezier_cubic);
        vm.addNativeMethod(batchClass, "catmullRomSpline", batch_catmull_rom_spline);
        vm.addNativeMethod(batchClass, "bspline", batch_bspline);
        vm.addNativeMethod(batchClass, "hermiteSpline", batch_hermite_spline);
        vm.addNativeMethod(batchClass, "thickSpline", batch_thick_spline);
        vm.addNativeMethod(batchClass, "vertex2f", batch_vertex2f);
        vm.addNativeMethod(batchClass, "vertex3f", batch_vertex3f);
        vm.addNativeMethod(batchClass, "texCoord2f", batch_texcoord2f);

        module.addInt("BATCH_LINES", kBatchModeLines)
              .addInt("BATCH_TRIANGLES", kBatchModeTriangles)
              .addInt("BATCH_QUAD", kBatchModeQuad);

        NativeClassDef *fontClass = vm.registerNativeClass("SpriteFont", font_ctor, font_dtor, 0, false);
        vm.addNativeMethod(fontClass, "release",        font_release);
        vm.addNativeMethod(fontClass, "loadDefault",    font_load_default);
        vm.addNativeMethod(fontClass, "loadBMFont",     font_load_bmfont);
        vm.addNativeMethod(fontClass, "loadTTF",        font_load_ttf);
        vm.addNativeMethod(fontClass, "setBatch",       font_set_batch);
        vm.addNativeMethod(fontClass, "setColor",       font_set_color);
        vm.addNativeMethod(fontClass, "setFontSize",    font_set_font_size);
        vm.addNativeMethod(fontClass, "setSpacing",     font_set_spacing);
        vm.addNativeMethod(fontClass, "setLineSpacing", font_set_line_spacing);
        vm.addNativeMethod(fontClass, "print",          font_print);
        vm.addNativeMethod(fontClass, "printWrapped",   font_print_wrapped);
        vm.addNativeMethod(fontClass, "getTextWidth",   font_get_text_width);
        vm.addNativeMethod(fontClass, "getTextHeight",  font_get_text_height);
        vm.addNativeMethod(fontClass, "isValid",        font_is_valid);
    }
}
