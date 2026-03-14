#pragma once

#include "assimp_bindings.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <cstring>

namespace AssimpBindings
{
    // SceneHandle  owns the Assimp::Importer and the scene tree.
    // Manual refcount keeps it alive while any Mesh/Material/Node still lives.
    struct SceneHandle
    {
        Assimp::Importer importer;
        const aiScene   *scene    = nullptr;
        bool             valid    = false;
        int              refCount = 1;

        void load(const char *path, unsigned int flags)
        {
            scene = importer.ReadFile(path, flags);
            valid = (scene != nullptr &&
                     !(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) &&
                     scene->mRootNode != nullptr);
        }

        void destroy()
        {
            importer.FreeScene();
            scene = nullptr;
            valid = false;
        }

        void addRef()  { refCount++; }
        void release() { if (--refCount <= 0) delete this; }
    };

    // Borrowed pointer into scene - does NOT own data.
    struct MeshHandle
    {
        SceneHandle    *owner;
        const aiMesh   *mesh;
    };

    struct MaterialHandle
    {
        SceneHandle       *owner;
        const aiMaterial  *mat;
    };

    struct NodeHandle
    {
        SceneHandle  *owner;
        const aiNode *node;
    };

    extern NativeClassDef *g_sceneClass;
    extern NativeClassDef *g_meshClass;
    extern NativeClassDef *g_materialClass;
    extern NativeClassDef *g_nodeClass;

    int  push_nil1(Interpreter *vm);
    bool read_string_arg(const Value &v, const char **out, const char *fn, int idx);
    bool read_number_arg(const Value &v, double *out, const char *fn, int idx);

    SceneHandle    *require_scene(void *instance, const char *fn);
    MeshHandle     *require_mesh(void *instance, const char *fn);
    MaterialHandle *require_material(void *instance, const char *fn);
    NodeHandle     *require_node(void *instance, const char *fn);

    bool push_mesh(Interpreter *vm, SceneHandle *owner, const aiMesh *mesh);
    bool push_material(Interpreter *vm, SceneHandle *owner, const aiMaterial *mat);
    bool push_node(Interpreter *vm, SceneHandle *owner, const aiNode *node);

    void register_scene(Interpreter &vm);
    void register_mesh(Interpreter &vm);
    void register_material(Interpreter &vm);
    void register_node(Interpreter &vm);

} // namespace AssimpBindings
