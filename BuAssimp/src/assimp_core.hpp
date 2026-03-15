#pragma once

#include "assimp_bindings.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <assimp/anim.h>
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

    struct AnimationHandle
    {
        SceneHandle        *owner;
        const aiAnimation  *anim;
    };

    struct NodeAnimHandle
    {
        SceneHandle       *owner;
        const aiAnimation *anim;
        const aiNodeAnim  *channel;
    };

    extern NativeClassDef  *g_sceneClass;
    extern NativeClassDef  *g_meshClass;
    extern NativeClassDef  *g_materialClass;
    extern NativeClassDef  *g_nodeClass;
    extern NativeClassDef  *g_animationClass;
    extern NativeClassDef  *g_nodeAnimClass;
    extern NativeStructDef *g_matrixDef;

    NativeStructDef *get_native_struct_def(Interpreter *vm, const char *name);

    int  push_nil1(Interpreter *vm);
    bool read_string_arg(const Value &v, const char **out, const char *fn, int idx);
    bool read_number_arg(const Value &v, double *out, const char *fn, int idx);
    bool read_boolish_arg(const Value &v, bool *out, const char *fn, int idx);
    bool push_matrix(Interpreter *vm, const aiMatrix4x4 &m);

    SceneHandle    *require_scene(void *instance, const char *fn);
    MeshHandle     *require_mesh(void *instance, const char *fn);
    MaterialHandle *require_material(void *instance, const char *fn);
    NodeHandle     *require_node(void *instance, const char *fn);
    AnimationHandle *require_animation(void *instance, const char *fn);
    NodeAnimHandle *require_node_anim(void *instance, const char *fn);

    bool push_mesh(Interpreter *vm, SceneHandle *owner, const aiMesh *mesh);
    bool push_material(Interpreter *vm, SceneHandle *owner, const aiMaterial *mat);
    bool push_node(Interpreter *vm, SceneHandle *owner, const aiNode *node);
    bool push_animation(Interpreter *vm, SceneHandle *owner, const aiAnimation *anim);
    bool push_node_anim(Interpreter *vm, SceneHandle *owner, const aiAnimation *anim, const aiNodeAnim *channel);

    void register_scene(Interpreter &vm);
    void register_mesh(Interpreter &vm);
    void register_material(Interpreter &vm);
    void register_node(Interpreter &vm);
    void register_animation(Interpreter &vm);

} // namespace AssimpBindings
