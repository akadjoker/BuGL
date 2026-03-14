#include "assimp_core.hpp"

namespace AssimpBindings
{
    static void *mesh_ctor_error(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm; (void)argCount; (void)args;
        Error("AssimpMesh cannot be constructed directly; use AssimpScene.getMesh()");
        return nullptr;
    }

    static void mesh_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        MeshHandle *h = (MeshHandle *)instance;
        if (!h) return;
        h->owner->release();
        delete h;
    }

    static int mesh_get_name(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        MeshHandle *h = require_mesh(instance, "AssimpMesh.getName()");
        if (!h) return push_nil1(vm);
        vm->pushString(h->mesh->mName.C_Str());
        return 1;
    }

    static int mesh_get_vertex_count(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        MeshHandle *h = require_mesh(instance, "AssimpMesh.getVertexCount()");
        if (!h) return push_nil1(vm);
        vm->pushInt((int)h->mesh->mNumVertices);
        return 1;
    }

    static int mesh_get_face_count(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        MeshHandle *h = require_mesh(instance, "AssimpMesh.getFaceCount()");
        if (!h) return push_nil1(vm);
        vm->pushInt((int)h->mesh->mNumFaces);
        return 1;
    }

    static int mesh_get_material_index(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        MeshHandle *h = require_mesh(instance, "AssimpMesh.getMaterialIndex()");
        if (!h) return push_nil1(vm);
        vm->pushInt((int)h->mesh->mMaterialIndex);
        return 1;
    }

    static int mesh_has_normals(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        MeshHandle *h = require_mesh(instance, "AssimpMesh.hasNormals()");
        if (!h) return push_nil1(vm);
        vm->pushBool(h->mesh->mNormals != nullptr);
        return 1;
    }

    static int mesh_has_uvs(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        MeshHandle *h = require_mesh(instance, "AssimpMesh.hasUVs()");
        if (!h) return push_nil1(vm);
        int ch = (argCount > 0 && args[0].isNumber()) ? (int)args[0].asDouble() : 0;
        bool ok = (ch >= 0 && ch < AI_MAX_NUMBER_OF_TEXTURECOORDS && h->mesh->mTextureCoords[ch] != nullptr);
        vm->pushBool(ok);
        return 1;
    }

    static int mesh_has_tangents(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        MeshHandle *h = require_mesh(instance, "AssimpMesh.hasTangents()");
        if (!h) return push_nil1(vm);
        vm->pushBool(h->mesh->mTangents != nullptr);
        return 1;
    }

    // getVertex(idx) -> x, y, z
    static int mesh_get_vertex(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 1) { Error("AssimpMesh.getVertex(idx)"); return push_nil1(vm); }
        MeshHandle *h = require_mesh(instance, "AssimpMesh.getVertex()");
        if (!h) return push_nil1(vm);
        double idxd = 0.0;
        if (!read_number_arg(args[0], &idxd, "AssimpMesh.getVertex()", 1)) return push_nil1(vm);
        int idx = (int)idxd;
        if (idx < 0 || idx >= (int)h->mesh->mNumVertices) { Error("AssimpMesh.getVertex(): index %d out of range", idx); return push_nil1(vm); }
        vm->pushDouble((double)h->mesh->mVertices[idx].x);
        vm->pushDouble((double)h->mesh->mVertices[idx].y);
        vm->pushDouble((double)h->mesh->mVertices[idx].z);
        return 3;
    }

    // getNormal(idx) -> x, y, z
    static int mesh_get_normal(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 1) { Error("AssimpMesh.getNormal(idx)"); return push_nil1(vm); }
        MeshHandle *h = require_mesh(instance, "AssimpMesh.getNormal()");
        if (head -10 /media/projectos/projects/cpp/minirender/core/include/glad/glad.h || head -10>mesh->mNormals) { vm->pushNil(); vm->pushNil(); vm->pushNil(); return 3; }
        double idxd = 0.0;
        if (!read_number_arg(args[0], &idxd, "AssimpMesh.getNormal()", 1)) return push_nil1(vm);
        int idx = (int)idxd;
        if (idx < 0 || idx >= (int)h->mesh->mNumVertices) { Error("AssimpMesh.getNormal(): index %d out of range", idx); return push_nil1(vm); }
        vm->pushDouble((double)h->mesh->mNormals[idx].x);
        vm->pushDouble((double)h->mesh->mNormals[idx].y);
        vm->pushDouble((double)h->mesh->mNormals[idx].z);
        return 3;
    }

    // getUV(idx [, channel=0]) -> u, v
    static int mesh_get_uv(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 1) { Error("AssimpMesh.getUV(idx [, channel])"); return push_nil1(vm); }
        MeshHandle *h = require_mesh(instance, "AssimpMesh.getUV()");
        if (!h) return push_nil1(vm);
        double idxd = 0.0;
        if (!read_number_arg(args[0], &idxd, "AssimpMesh.getUV()", 1)) return push_nil1(vm);
        int idx = (int)idxd;
        int ch  = (argCount > 1 && args[1].isNumber()) ? (int)args[1].asDouble() : 0;
        if (ch < 0 || ch >= AI_MAX_NUMBER_OF_TEXTURECOORDS || !h->mesh->mTextureCoords[ch])
            return push_nil1(vm);
        if (idx < 0 || idx >= (int)h->mesh->mNumVertices) { Error("AssimpMesh.getUV(): index %d out of range", idx); return push_nil1(vm); }
        vm->pushDouble((double)h->mesh->mTextureCoords[ch][idx].x);
        vm->pushDouble((double)h->mesh->mTextureCoords[ch][idx].y);
        return 2;
    }

    // getTangent(idx) -> x, y, z
    static int mesh_get_tangent(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 1) { Error("AssimpMesh.getTangent(idx)"); return push_nil1(vm); }
        MeshHandle *h = require_mesh(instance, "AssimpMesh.getTangent()");
        if (head -10 /media/projectos/projects/cpp/minirender/core/include/glad/glad.h || head -10>mesh->mTangents) { vm->pushNil(); vm->pushNil(); vm->pushNil(); return 3; }
        double idxd = 0.0;
        if (!read_number_arg(args[0], &idxd, "AssimpMesh.getTangent()", 1)) return push_nil1(vm);
        int idx = (int)idxd;
        if (idx < 0 || idx >= (int)h->mesh->mNumVertices) { Error("AssimpMesh.getTangent(): index %d out of range", idx); return push_nil1(vm); }
        vm->pushDouble((double)h->mesh->mTangents[idx].x);
        vm->pushDouble((double)h->mesh->mTangents[idx].y);
        vm->pushDouble((double)h->mesh->mTangents[idx].z);
        return 3;
    }

    // getFace(idx) -> i0, i1, i2
    static int mesh_get_face(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 1) { Error("AssimpMesh.getFace(idx)"); return push_nil1(vm); }
        MeshHandle *h = require_mesh(instance, "AssimpMesh.getFace()");
        if (!h) return push_nil1(vm);
        double idxd = 0.0;
        if (!read_number_arg(args[0], &idxd, "AssimpMesh.getFace()", 1)) return push_nil1(vm);
        int idx = (int)idxd;
        if (idx < 0 || idx >= (int)h->mesh->mNumFaces) { Error("AssimpMesh.getFace(): index %d out of range", idx); return push_nil1(vm); }
        const aiFace &face = h->mesh->mFaces[idx];
        if (face.mNumIndices != 3) { Error("AssimpMesh.getFace(): not a triangle; load with aiProcess_Triangulate"); return push_nil1(vm); }
        vm->pushInt((int)face.mIndices[0]);
        vm->pushInt((int)face.mIndices[1]);
        vm->pushInt((int)face.mIndices[2]);
        return 3;
    }

    // getVertices() -> flat array [x,y,z,...]
    static int mesh_get_vertices(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        MeshHandle *h = require_mesh(instance, "AssimpMesh.getVertices()");
        if (!h) return push_nil1(vm);
        Value arr = vm->makeArray();
        ArrayInstance *a = arr.as.array;
        a->values.reserve(h->mesh->mNumVertices * 3);
        for (unsigned int i = 0; i < h->mesh->mNumVertices; i++)
        {
            a->values.push(vm->makeDouble((double)h->mesh->mVertices[i].x));
            a->values.push(vm->makeDouble((double)h->mesh->mVertices[i].y));
            a->values.push(vm->makeDouble((double)h->mesh->mVertices[i].z));
        }
        vm->push(arr);
        return 1;
    }

    // getNormals() -> flat array [nx,ny,nz,...]
    static int mesh_get_normals(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        MeshHandle *h = require_mesh(instance, "AssimpMesh.getNormals()");
        if (head -10 /media/projectos/projects/cpp/minirender/core/include/glad/glad.h || head -10>mesh->mNormals) return push_nil1(vm);
        Value arr = vm->makeArray();
        ArrayInstance *a = arr.as.array;
        a->values.reserve(h->mesh->mNumVertices * 3);
        for (unsigned int i = 0; i < h->mesh->mNumVertices; i++)
        {
            a->values.push(vm->makeDouble((double)h->mesh->mNormals[i].x));
            a->values.push(vm->makeDouble((double)h->mesh->mNormals[i].y));
            a->values.push(vm->makeDouble((double)h->mesh->mNormals[i].z));
        }
        vm->push(arr);
        return 1;
    }

    // getUVs([channel=0]) -> flat array [u,v,...]
    static int mesh_get_uvs(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        MeshHandle *h = require_mesh(instance, "AssimpMesh.getUVs()");
        if (!h) return push_nil1(vm);
        int ch = (argCount > 0 && args[0].isNumber()) ? (int)args[0].asDouble() : 0;
        if (ch < 0 || ch >= AI_MAX_NUMBER_OF_TEXTURECOORDS || !h->mesh->mTextureCoords[ch])
            return push_nil1(vm);
        Value arr = vm->makeArray();
        ArrayInstance *a = arr.as.array;
        a->values.reserve(h->mesh->mNumVertices * 2);
        for (unsigned int i = 0; i < h->mesh->mNumVertices; i++)
        {
            a->values.push(vm->makeDouble((double)h->mesh->mTextureCoords[ch][i].x));
            a->values.push(vm->makeDouble((double)h->mesh->mTextureCoords[ch][i].y));
        }
        vm->push(arr);
        return 1;
    }

    // getTangents() -> flat array [tx,ty,tz,...]
    static int mesh_get_tangents(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        MeshHandle *h = require_mesh(instance, "AssimpMesh.getTangents()");
        if (head -10 /media/projectos/projects/cpp/minirender/core/include/glad/glad.h || head -10>mesh->mTangents) return push_nil1(vm);
        Value arr = vm->makeArray();
        ArrayInstance *a = arr.as.array;
        a->values.reserve(h->mesh->mNumVertices * 3);
        for (unsigned int i = 0; i < h->mesh->mNumVertices; i++)
        {
            a->values.push(vm->makeDouble((double)h->mesh->mTangents[i].x));
            a->values.push(vm->makeDouble((double)h->mesh->mTangents[i].y));
            a->values.push(vm->makeDouble((double)h->mesh->mTangents[i].z));
        }
        vm->push(arr);
        return 1;
    }

    // getIndices() -> flat array [i0,i1,i2,...]
    static int mesh_get_indices(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        MeshHandle *h = require_mesh(instance, "AssimpMesh.getIndices()");
        if (!h) return push_nil1(vm);
        Value arr = vm->makeArray();
        ArrayInstance *a = arr.as.array;
        a->values.reserve(h->mesh->mNumFaces * 3);
        for (unsigned int f = 0; f < h->mesh->mNumFaces; f++)
        {
            const aiFace &face = h->mesh->mFaces[f];
            for (unsigned int k = 0; k < face.mNumIndices; k++)
                a->values.push(vm->makeInt((int)face.mIndices[k]));
        }
        vm->push(arr);
        return 1;
    }

    void register_mesh(Interpreter &vm)
    {
        g_meshClass = vm.registerNativeClass("AssimpMesh", mesh_ctor_error, mesh_dtor, 0, false);
        vm.addNativeMethod(g_meshClass, "getName",          mesh_get_name);
        vm.addNativeMethod(g_meshClass, "getVertexCount",   mesh_get_vertex_count);
        vm.addNativeMethod(g_meshClass, "getFaceCount",     mesh_get_face_count);
        vm.addNativeMethod(g_meshClass, "getMaterialIndex", mesh_get_material_index);
        vm.addNativeMethod(g_meshClass, "hasNormals",       mesh_has_normals);
        vm.addNativeMethod(g_meshClass, "hasUVs",           mesh_has_uvs);
        vm.addNativeMethod(g_meshClass, "hasTangents",      mesh_has_tangents);
        vm.addNativeMethod(g_meshClass, "getVertex",        mesh_get_vertex);
        vm.addNativeMethod(g_meshClass, "getNormal",        mesh_get_normal);
        vm.addNativeMethod(g_meshClass, "getUV",            mesh_get_uv);
        vm.addNativeMethod(g_meshClass, "getTangent",       mesh_get_tangent);
        vm.addNativeMethod(g_meshClass, "getFace",          mesh_get_face);
        vm.addNativeMethod(g_meshClass, "getVertices",      mesh_get_vertices);
        vm.addNativeMethod(g_meshClass, "getNormals",       mesh_get_normals);
        vm.addNativeMethod(g_meshClass, "getUVs",           mesh_get_uvs);
        vm.addNativeMethod(g_meshClass, "getTangents",      mesh_get_tangents);
        vm.addNativeMethod(g_meshClass, "getIndices",       mesh_get_indices);
    }

} // namespace AssimpBindings
