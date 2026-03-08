#include "bindings.hpp"

#define PAR_SHAPES_IMPLEMENTATION
#include "par_shapes.h"

#include <cstring>
#include <unordered_map>

namespace SDLBindings
{
    namespace
    {
        struct ShapeStore
        {
            std::unordered_map<int, par_shapes_mesh *> items;
            int nextId = 1;

            ~ShapeStore()
            {
                clear();
            }

            int add(par_shapes_mesh *mesh)
            {
                if (!mesh)
                    return 0;

                int id = nextId++;
                if (nextId <= 0)
                    nextId = 1;

                items[id] = mesh;
                return id;
            }

            par_shapes_mesh *get(int id)
            {
                auto it = items.find(id);
                if (it == items.end())
                    return nullptr;
                return it->second;
            }

            bool remove(int id)
            {
                auto it = items.find(id);
                if (it == items.end())
                    return false;
                par_shapes_free_mesh(it->second);
                items.erase(it);
                return true;
            }

            void clear()
            {
                for (auto &pair : items)
                {
                    par_shapes_free_mesh(pair.second);
                }
                items.clear();
            }
        };

        ShapeStore &shape_store()
        {
            static ShapeStore store;
            return store;
        }

        par_shapes_mesh *get_shape_or_error(Value *args, int index, const char *fnName)
        {
            if (!args[index].isNumber())
            {
                Error("%s expects shapeId at arg %d", fnName, index + 1);
                return nullptr;
            }

            const int shapeId = args[index].asInt();
            par_shapes_mesh *mesh = shape_store().get(shapeId);
            if (!mesh)
            {
                Error("%s invalid shapeId: %d", fnName, shapeId);
                return nullptr;
            }
            return mesh;
        }

        Value make_float_buffer(Interpreter *vm, const float *src, int count)
        {
            Value out = vm->makeBuffer(count, (int)BufferType::FLOAT);
            BufferInstance *buf = out.asBuffer();
            if (buf && buf->data && src && count > 0)
            {
                std::memcpy(buf->data, src, (size_t)count * sizeof(float));
            }
            return out;
        }

        Value make_u16_buffer(Interpreter *vm, const uint16_t *src, int count)
        {
            Value out = vm->makeBuffer(count, (int)BufferType::UINT16);
            BufferInstance *buf = out.asBuffer();
            if (buf && buf->data && src && count > 0)
            {
                std::memcpy(buf->data, src, (size_t)count * sizeof(uint16_t));
            }
            return out;
        }
    }

    static_assert(sizeof(PAR_SHAPES_T) == sizeof(uint16_t), "ShapeGetIndices currently assumes 16-bit indices.");

    static int native_ShapeCreateBox(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3 || !args[0].isNumber() || !args[1].isNumber() || !args[2].isNumber())
        {
            Error("ShapeCreateBox expects (width, height, depth)");
            return 0;
        }

        const float w = (float)args[0].asNumber();
        const float h = (float)args[1].asNumber();
        const float d = (float)args[2].asNumber();

        par_shapes_mesh *mesh = par_shapes_create_cube();
        if (!mesh)
        {
            vm->pushInt(0);
            return 1;
        }

        par_shapes_scale(mesh, w, h, d);
        par_shapes_translate(mesh, -w * 0.5f, -h * 0.5f, -d * 0.5f);
        vm->pushInt(shape_store().add(mesh));
        return 1;
    }

    static int native_ShapeCreateSphere(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3 || !args[0].isNumber() || !args[1].isNumber() || !args[2].isNumber())
        {
            Error("ShapeCreateSphere expects (radius, slices, stacks)");
            return 0;
        }

        const float radius = (float)args[0].asNumber();
        const int slices = args[1].asInt();
        const int stacks = args[2].asInt();

        par_shapes_mesh *mesh = par_shapes_create_parametric_sphere(slices, stacks);
        if (!mesh)
        {
            vm->pushInt(0);
            return 1;
        }

        par_shapes_scale(mesh, radius, radius, radius);
        vm->pushInt(shape_store().add(mesh));
        return 1;
    }

    static int native_ShapeCreateCylinder(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4 || !args[0].isNumber() || !args[1].isNumber() || !args[2].isNumber() || !args[3].isNumber())
        {
            Error("ShapeCreateCylinder expects (radius, height, slices, stacks)");
            return 0;
        }

        const float radius = (float)args[0].asNumber();
        const float height = (float)args[1].asNumber();
        const int slices = args[2].asInt();
        const int stacks = args[3].asInt();

        par_shapes_mesh *mesh = par_shapes_create_cylinder(slices, stacks);
        if (!mesh)
        {
            vm->pushInt(0);
            return 1;
        }

        par_shapes_scale(mesh, radius, radius, height);
        par_shapes_translate(mesh, 0.0f, 0.0f, -height * 0.5f);
        vm->pushInt(shape_store().add(mesh));
        return 1;
    }

    static int native_ShapeCreateCone(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4 || !args[0].isNumber() || !args[1].isNumber() || !args[2].isNumber() || !args[3].isNumber())
        {
            Error("ShapeCreateCone expects (radius, height, slices, stacks)");
            return 0;
        }

        const float radius = (float)args[0].asNumber();
        const float height = (float)args[1].asNumber();
        const int slices = args[2].asInt();
        const int stacks = args[3].asInt();

        par_shapes_mesh *mesh = par_shapes_create_cone(slices, stacks);
        if (!mesh)
        {
            vm->pushInt(0);
            return 1;
        }

        par_shapes_scale(mesh, radius, radius, height);
        par_shapes_translate(mesh, 0.0f, 0.0f, -height * 0.5f);
        vm->pushInt(shape_store().add(mesh));
        return 1;
    }

    static int native_ShapeCreatePlane(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4 || !args[0].isNumber() || !args[1].isNumber() || !args[2].isNumber() || !args[3].isNumber())
        {
            Error("ShapeCreatePlane expects (width, depth, slices, stacks)");
            return 0;
        }

        const float width = (float)args[0].asNumber();
        const float depth = (float)args[1].asNumber();
        const int slices = args[2].asInt();
        const int stacks = args[3].asInt();

        par_shapes_mesh *mesh = par_shapes_create_plane(slices, stacks);
        if (!mesh)
        {
            vm->pushInt(0);
            return 1;
        }

        par_shapes_scale(mesh, width, depth, 1.0f);
        par_shapes_translate(mesh, -width * 0.5f, -depth * 0.5f, 0.0f);
        vm->pushInt(shape_store().add(mesh));
        return 1;
    }

    static int native_ShapeCreateDisk(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3 || !args[0].isNumber() || !args[1].isNumber() || !args[2].isNumber())
        {
            Error("ShapeCreateDisk expects (radius, slices, stacks)");
            return 0;
        }

        const float radius = (float)args[0].asNumber();
        const int slices = args[1].asInt();
        const int stacks = args[2].asInt();

        par_shapes_mesh *mesh = par_shapes_create_parametric_disk(slices, stacks);
        if (!mesh)
        {
            vm->pushInt(0);
            return 1;
        }

        par_shapes_scale(mesh, radius, radius, 1.0f);
        vm->pushInt(shape_store().add(mesh));
        return 1;
    }

    static int native_ShapeCreateHemisphere(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3 || !args[0].isNumber() || !args[1].isNumber() || !args[2].isNumber())
        {
            Error("ShapeCreateHemisphere expects (radius, slices, stacks)");
            return 0;
        }

        const float radius = (float)args[0].asNumber();
        const int slices = args[1].asInt();
        const int stacks = args[2].asInt();

        par_shapes_mesh *mesh = par_shapes_create_hemisphere(slices, stacks);
        if (!mesh)
        {
            vm->pushInt(0);
            return 1;
        }

        par_shapes_scale(mesh, radius, radius, radius);
        vm->pushInt(shape_store().add(mesh));
        return 1;
    }

    static int native_ShapeCreateTorus(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4 || !args[0].isNumber() || !args[1].isNumber() || !args[2].isNumber() || !args[3].isNumber())
        {
            Error("ShapeCreateTorus expects (majorRadius, minorRadius, slices, stacks)");
            return 0;
        }

        const float majorRadius = (float)args[0].asNumber();
        const float minorRadius = (float)args[1].asNumber();
        const int slices = args[2].asInt();
        const int stacks = args[3].asInt();
        if (majorRadius <= 0.0f || minorRadius <= 0.0f)
        {
            vm->pushInt(0);
            return 1;
        }

        const float minorRatio = minorRadius / majorRadius;
        par_shapes_mesh *mesh = par_shapes_create_torus(slices, stacks, minorRatio);
        if (!mesh)
        {
            vm->pushInt(0);
            return 1;
        }

        par_shapes_scale(mesh, majorRadius, majorRadius, majorRadius);
        vm->pushInt(shape_store().add(mesh));
        return 1;
    }

    static int native_ShapeCreateIcoSphere(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("ShapeCreateIcoSphere expects (radius, subdivisions)");
            return 0;
        }

        const float radius = (float)args[0].asNumber();
        const int subdivisions = args[1].asInt();
        par_shapes_mesh *mesh = par_shapes_create_subdivided_sphere(subdivisions);
        if (!mesh)
        {
            vm->pushInt(0);
            return 1;
        }

        par_shapes_scale(mesh, radius, radius, radius);
        vm->pushInt(shape_store().add(mesh));
        return 1;
    }

    static int native_ShapeCreateIcosahedron(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isNumber())
        {
            Error("ShapeCreateIcosahedron expects (radius)");
            return 0;
        }

        const float radius = (float)args[0].asNumber();
        par_shapes_mesh *mesh = par_shapes_create_icosahedron();
        if (!mesh)
        {
            vm->pushInt(0);
            return 1;
        }

        par_shapes_scale(mesh, radius, radius, radius);
        vm->pushInt(shape_store().add(mesh));
        return 1;
    }

    static int native_ShapeCreateDodecahedron(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isNumber())
        {
            Error("ShapeCreateDodecahedron expects (radius)");
            return 0;
        }

        const float radius = (float)args[0].asNumber();
        par_shapes_mesh *mesh = par_shapes_create_dodecahedron();
        if (!mesh)
        {
            vm->pushInt(0);
            return 1;
        }

        par_shapes_scale(mesh, radius, radius, radius);
        vm->pushInt(shape_store().add(mesh));
        return 1;
    }

    static int native_ShapeCreateOctahedron(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isNumber())
        {
            Error("ShapeCreateOctahedron expects (radius)");
            return 0;
        }

        const float radius = (float)args[0].asNumber();
        par_shapes_mesh *mesh = par_shapes_create_octahedron();
        if (!mesh)
        {
            vm->pushInt(0);
            return 1;
        }

        par_shapes_scale(mesh, radius, radius, radius);
        vm->pushInt(shape_store().add(mesh));
        return 1;
    }

    static int native_ShapeCreateTetrahedron(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isNumber())
        {
            Error("ShapeCreateTetrahedron expects (radius)");
            return 0;
        }

        const float radius = (float)args[0].asNumber();
        par_shapes_mesh *mesh = par_shapes_create_tetrahedron();
        if (!mesh)
        {
            vm->pushInt(0);
            return 1;
        }

        par_shapes_scale(mesh, radius, radius, radius);
        vm->pushInt(shape_store().add(mesh));
        return 1;
    }

    static int native_ShapeDestroy(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isNumber())
        {
            Error("ShapeDestroy expects (shapeId)");
            return 0;
        }

        vm->pushBool(shape_store().remove(args[0].asInt()));
        return 1;
    }

    static int native_ShapeClear(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        (void)args;
        if (argc != 0)
        {
            Error("ShapeClear expects 0 arguments");
            return 0;
        }

        shape_store().clear();
        return 0;
    }

    static int native_ShapeExists(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1 || !args[0].isNumber())
        {
            Error("ShapeExists expects (shapeId)");
            return 0;
        }

        vm->pushBool(shape_store().get(args[0].asInt()) != nullptr);
        return 1;
    }

    static int native_ShapeGetVertexCount(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("ShapeGetVertexCount expects (shapeId)");
            return 0;
        }
        par_shapes_mesh *mesh = get_shape_or_error(args, 0, "ShapeGetVertexCount");
        if (!mesh)
        {
            vm->pushInt(0);
            return 1;
        }
        vm->pushInt(mesh->npoints);
        return 1;
    }

    static int native_ShapeGetTriangleCount(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("ShapeGetTriangleCount expects (shapeId)");
            return 0;
        }
        par_shapes_mesh *mesh = get_shape_or_error(args, 0, "ShapeGetTriangleCount");
        if (!mesh)
        {
            vm->pushInt(0);
            return 1;
        }
        vm->pushInt(mesh->ntriangles);
        return 1;
    }

    static int native_ShapeGetIndexCount(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("ShapeGetIndexCount expects (shapeId)");
            return 0;
        }
        par_shapes_mesh *mesh = get_shape_or_error(args, 0, "ShapeGetIndexCount");
        if (!mesh)
        {
            vm->pushInt(0);
            return 1;
        }
        vm->pushInt(mesh->ntriangles * 3);
        return 1;
    }

    static int native_ShapeGetPositions(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("ShapeGetPositions expects (shapeId)");
            return 0;
        }
        par_shapes_mesh *mesh = get_shape_or_error(args, 0, "ShapeGetPositions");
        if (!mesh || !mesh->points || mesh->npoints <= 0)
        {
            vm->pushNil();
            return 1;
        }

        const int count = mesh->npoints * 3;
        vm->push(make_float_buffer(vm, mesh->points, count));
        return 1;
    }

    static int native_ShapeComputeNormals(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("ShapeComputeNormals expects (shapeId)");
            return 0;
        }
        par_shapes_mesh *mesh = get_shape_or_error(args, 0, "ShapeComputeNormals");
        if (!mesh)
        {
            vm->pushBool(false);
            return 1;
        }

        par_shapes_compute_normals(mesh);
        vm->pushBool(mesh->normals != nullptr);
        return 1;
    }

    static int native_ShapeGetNormals(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("ShapeGetNormals expects (shapeId)");
            return 0;
        }
        par_shapes_mesh *mesh = get_shape_or_error(args, 0, "ShapeGetNormals");
        if (!mesh || mesh->npoints <= 0)
        {
            vm->pushNil();
            return 1;
        }

        if (!mesh->normals)
        {
            par_shapes_compute_normals(mesh);
        }
        if (!mesh->normals)
        {
            vm->pushNil();
            return 1;
        }

        const int count = mesh->npoints * 3;
        vm->push(make_float_buffer(vm, mesh->normals, count));
        return 1;
    }

    static int native_ShapeGetTexCoords(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("ShapeGetTexCoords expects (shapeId)");
            return 0;
        }
        par_shapes_mesh *mesh = get_shape_or_error(args, 0, "ShapeGetTexCoords");
        if (!mesh || !mesh->tcoords || mesh->npoints <= 0)
        {
            vm->pushNil();
            return 1;
        }

        const int count = mesh->npoints * 2;
        vm->push(make_float_buffer(vm, mesh->tcoords, count));
        return 1;
    }

    static int native_ShapeGetIndices(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("ShapeGetIndices expects (shapeId)");
            return 0;
        }
        par_shapes_mesh *mesh = get_shape_or_error(args, 0, "ShapeGetIndices");
        if (!mesh || !mesh->triangles || mesh->ntriangles <= 0)
        {
            vm->pushNil();
            return 1;
        }

        const int count = mesh->ntriangles * 3;
        vm->push(make_u16_buffer(vm, (const uint16_t *)mesh->triangles, count));
        return 1;
    }

    static int native_ShapeScale(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4 || !args[1].isNumber() || !args[2].isNumber() || !args[3].isNumber())
        {
            Error("ShapeScale expects (shapeId, x, y, z)");
            return 0;
        }
        par_shapes_mesh *mesh = get_shape_or_error(args, 0, "ShapeScale");
        if (!mesh)
        {
            vm->pushBool(false);
            return 1;
        }

        par_shapes_scale(mesh, (float)args[1].asNumber(), (float)args[2].asNumber(), (float)args[3].asNumber());
        vm->pushBool(true);
        return 1;
    }

    static int native_ShapeTranslate(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4 || !args[1].isNumber() || !args[2].isNumber() || !args[3].isNumber())
        {
            Error("ShapeTranslate expects (shapeId, x, y, z)");
            return 0;
        }
        par_shapes_mesh *mesh = get_shape_or_error(args, 0, "ShapeTranslate");
        if (!mesh)
        {
            vm->pushBool(false);
            return 1;
        }

        par_shapes_translate(mesh, (float)args[1].asNumber(), (float)args[2].asNumber(), (float)args[3].asNumber());
        vm->pushBool(true);
        return 1;
    }

    static int native_ShapeRotate(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5 || !args[1].isNumber() || !args[2].isNumber() || !args[3].isNumber() || !args[4].isNumber())
        {
            Error("ShapeRotate expects (shapeId, radians, axisX, axisY, axisZ)");
            return 0;
        }
        par_shapes_mesh *mesh = get_shape_or_error(args, 0, "ShapeRotate");
        if (!mesh)
        {
            vm->pushBool(false);
            return 1;
        }

        const float axis[3] = {(float)args[2].asNumber(), (float)args[3].asNumber(), (float)args[4].asNumber()};
        par_shapes_rotate(mesh, (float)args[1].asNumber(), axis);
        vm->pushBool(true);
        return 1;
    }

    static int native_ShapeGetAABB(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("ShapeGetAABB expects (shapeId)");
            return 0;
        }
        par_shapes_mesh *mesh = get_shape_or_error(args, 0, "ShapeGetAABB");
        if (!mesh)
        {
            vm->pushNil();
            vm->pushNil();
            vm->pushNil();
            vm->pushNil();
            vm->pushNil();
            vm->pushNil();
            return 6;
        }

        float aabb[6] = {0, 0, 0, 0, 0, 0};
        par_shapes_compute_aabb(mesh, aabb);
        vm->pushDouble(aabb[0]);
        vm->pushDouble(aabb[1]);
        vm->pushDouble(aabb[2]);
        vm->pushDouble(aabb[3]);
        vm->pushDouble(aabb[4]);
        vm->pushDouble(aabb[5]);
        return 6;
    }

    void register_shapes(ModuleBuilder &module)
    {
        module.addFunction("ShapeCreateBox", native_ShapeCreateBox, 3)
            .addFunction("ShapeCreateSphere", native_ShapeCreateSphere, 3)
            .addFunction("ShapeCreateCylinder", native_ShapeCreateCylinder, 4)
            .addFunction("ShapeCreateCone", native_ShapeCreateCone, 4)
            .addFunction("ShapeCreatePlane", native_ShapeCreatePlane, 4)
            .addFunction("ShapeCreateDisk", native_ShapeCreateDisk, 3)
            .addFunction("ShapeCreateHemisphere", native_ShapeCreateHemisphere, 3)
            .addFunction("ShapeCreateTorus", native_ShapeCreateTorus, 4)
            .addFunction("ShapeCreateIcoSphere", native_ShapeCreateIcoSphere, 2)
            .addFunction("ShapeCreateIcosahedron", native_ShapeCreateIcosahedron, 1)
            .addFunction("ShapeCreateDodecahedron", native_ShapeCreateDodecahedron, 1)
            .addFunction("ShapeCreateOctahedron", native_ShapeCreateOctahedron, 1)
            .addFunction("ShapeCreateTetrahedron", native_ShapeCreateTetrahedron, 1)
            .addFunction("ShapeDestroy", native_ShapeDestroy, 1)
            .addFunction("ShapeClear", native_ShapeClear, 0)
            .addFunction("ShapeExists", native_ShapeExists, 1)
            .addFunction("ShapeGetVertexCount", native_ShapeGetVertexCount, 1)
            .addFunction("ShapeGetTriangleCount", native_ShapeGetTriangleCount, 1)
            .addFunction("ShapeGetIndexCount", native_ShapeGetIndexCount, 1)
            .addFunction("ShapeGetPositions", native_ShapeGetPositions, 1)
            .addFunction("ShapeGetNormals", native_ShapeGetNormals, 1)
            .addFunction("ShapeComputeNormals", native_ShapeComputeNormals, 1)
            .addFunction("ShapeGetTexCoords", native_ShapeGetTexCoords, 1)
            .addFunction("ShapeGetIndices", native_ShapeGetIndices, 1)
            .addFunction("ShapeScale", native_ShapeScale, 4)
            .addFunction("ShapeTranslate", native_ShapeTranslate, 4)
            .addFunction("ShapeRotate", native_ShapeRotate, 5)
            .addFunction("ShapeGetAABB", native_ShapeGetAABB, 1);
    }
}
