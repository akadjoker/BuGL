#include "assimp_core.hpp"

namespace AssimpBindings
{
    static const aiNode *find_node_by_name_recursive(const aiNode *n, const char *name)
    {
        if (!n || !name) return nullptr;
        if (strcmp(n->mName.C_Str(), name) == 0) return n;
        for (unsigned int i = 0; i < n->mNumChildren; ++i)
        {
            const aiNode *hit = find_node_by_name_recursive(n->mChildren[i], name);
            if (hit) return hit;
        }
        return nullptr;
    }

    static void *scene_ctor(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm; (void)argCount; (void)args;
        return new SceneHandle();
    }

    static void scene_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        SceneHandle *h = (SceneHandle *)instance;
        if (h) delete h;
    }

    // load(path [, flags]) -> bool
    static int scene_load(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 1) { Error("AssimpScene.load(path [, flags])"); return push_nil1(vm); }
        SceneHandle *h = (SceneHandle *)instance;
        if (!h) return push_nil1(vm);
        const char *path = nullptr;
        if (!read_string_arg(args[0], &path, "AssimpScene.load()", 1)) return push_nil1(vm);
        unsigned int flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenNormals | aiProcess_FlipUVs;
        if (argCount > 1)
        {
            double flagsD = 0.0;
            if (!read_number_arg(args[1], &flagsD, "AssimpScene.load()", 2)) return push_nil1(vm);
            flags = (unsigned int)flagsD;
        }
        h->destroy();
        h->load(path, flags);
        vm->pushBool(h->valid);
        return 1;
    }

    // isValid() -> bool
    static int scene_is_valid(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        SceneHandle *h = (SceneHandle *)instance;
        vm->pushBool(h && h->valid);
        return 1;
    }

    // getMeshCount() -> int
    static int scene_get_mesh_count(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        SceneHandle *h = require_scene(instance, "AssimpScene.getMeshCount()");
        if (!h) return push_nil1(vm);
        vm->pushInt((int)h->scene->mNumMeshes);
        return 1;
    }

    // getMaterialCount() -> int
    static int scene_get_material_count(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        SceneHandle *h = require_scene(instance, "AssimpScene.getMaterialCount()");
        if (!h) return push_nil1(vm);
        vm->pushInt((int)h->scene->mNumMaterials);
        return 1;
    }

    // getMesh(idx) -> AssimpMesh
    static int scene_get_mesh(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1) { Error("AssimpScene.getMesh(idx)"); return push_nil1(vm); }
        SceneHandle *h = require_scene(instance, "AssimpScene.getMesh()");
        if (!h) return push_nil1(vm);
        double idxd = 0.0;
        if (!read_number_arg(args[0], &idxd, "AssimpScene.getMesh()", 1)) return push_nil1(vm);
        int idx = (int)idxd;
        if (idx < 0 || idx >= (int)h->scene->mNumMeshes)
        {
            Error("AssimpScene.getMesh(): index %d out of range (0..%d)", idx, (int)h->scene->mNumMeshes - 1);
            return push_nil1(vm);
        }
        return push_mesh(vm, h, h->scene->mMeshes[idx]) ? 1 : push_nil1(vm);
    }

    // getMaterial(idx) -> AssimpMaterial
    static int scene_get_material(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1) { Error("AssimpScene.getMaterial(idx)"); return push_nil1(vm); }
        SceneHandle *h = require_scene(instance, "AssimpScene.getMaterial()");
        if (!h) return push_nil1(vm);
        double idxd = 0.0;
        if (!read_number_arg(args[0], &idxd, "AssimpScene.getMaterial()", 1)) return push_nil1(vm);
        int idx = (int)idxd;
        if (idx < 0 || idx >= (int)h->scene->mNumMaterials)
        {
            Error("AssimpScene.getMaterial(): index %d out of range (0..%d)", idx, (int)h->scene->mNumMaterials - 1);
            return push_nil1(vm);
        }
        return push_material(vm, h, h->scene->mMaterials[idx]) ? 1 : push_nil1(vm);
    }

    // getRootNode() -> AssimpNode
    static int scene_get_root_node(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        SceneHandle *h = require_scene(instance, "AssimpScene.getRootNode()");
        if (!h || !h->scene->mRootNode) return push_nil1(vm);
        return push_node(vm, h, h->scene->mRootNode) ? 1 : push_nil1(vm);
    }

    // findNodeByName(name) -> AssimpNode | nil
    static int scene_find_node_by_name(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1) { Error("AssimpScene.findNodeByName(name)"); return push_nil1(vm); }
        SceneHandle *h = require_scene(instance, "AssimpScene.findNodeByName()");
        if (!h) return push_nil1(vm);
        const char *name = nullptr;
        if (!read_string_arg(args[0], &name, "AssimpScene.findNodeByName()", 1)) return push_nil1(vm);

        const aiNode *node = find_node_by_name_recursive(h->scene->mRootNode, name);
        if (!node) return push_nil1(vm);
        return push_node(vm, h, node) ? 1 : push_nil1(vm);
    }

    // getAnimationCount() -> int
    static int scene_get_animation_count(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        SceneHandle *h = require_scene(instance, "AssimpScene.getAnimationCount()");
        if (!h) return push_nil1(vm);
        vm->pushInt((int)h->scene->mNumAnimations);
        return 1;
    }

    // getAnimation(idx) -> AssimpAnimation
    static int scene_get_animation(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1) { Error("AssimpScene.getAnimation(idx)"); return push_nil1(vm); }
        SceneHandle *h = require_scene(instance, "AssimpScene.getAnimation()");
        if (!h) return push_nil1(vm);

        double idxd = 0.0;
        if (!read_number_arg(args[0], &idxd, "AssimpScene.getAnimation()", 1)) return push_nil1(vm);
        int idx = (int)idxd;
        if (idx < 0 || idx >= (int)h->scene->mNumAnimations)
        {
            Error("AssimpScene.getAnimation(): index %d out of range (0..%d)", idx, (int)h->scene->mNumAnimations - 1);
            return push_nil1(vm);
        }

        return push_animation(vm, h, h->scene->mAnimations[idx]) ? 1 : push_nil1(vm);
    }

    // getError() -> string
    static int scene_get_error(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        SceneHandle *h = (SceneHandle *)instance;
        if (!h) return push_nil1(vm);
        const char *err = h->importer.GetErrorString();
        vm->pushString(err ? err : "");
        return 1;
    }

    // free()
    static int scene_free(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm; (void)argCount; (void)args;
        SceneHandle *h = (SceneHandle *)instance;
        if (h) h->destroy();
        return 0;
    }

    void register_scene(Interpreter &vm)
    {
        g_sceneClass = vm.registerNativeClass("AssimpScene", scene_ctor, scene_dtor, -1, false);
        vm.addNativeMethod(g_sceneClass, "load",             scene_load);
        vm.addNativeMethod(g_sceneClass, "isValid",          scene_is_valid);
        vm.addNativeMethod(g_sceneClass, "getMeshCount",     scene_get_mesh_count);
        vm.addNativeMethod(g_sceneClass, "getMaterialCount", scene_get_material_count);
        vm.addNativeMethod(g_sceneClass, "getMesh",          scene_get_mesh);
        vm.addNativeMethod(g_sceneClass, "getMaterial",      scene_get_material);
        vm.addNativeMethod(g_sceneClass, "getRootNode",      scene_get_root_node);
        vm.addNativeMethod(g_sceneClass, "findNodeByName",   scene_find_node_by_name);
        vm.addNativeMethod(g_sceneClass, "getAnimationCount",scene_get_animation_count);
        vm.addNativeMethod(g_sceneClass, "getAnimation",     scene_get_animation);
        vm.addNativeMethod(g_sceneClass, "getError",         scene_get_error);
        vm.addNativeMethod(g_sceneClass, "free",             scene_free);
    }

} // namespace AssimpBindings
