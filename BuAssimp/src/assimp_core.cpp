#include "assimp_core.hpp"

namespace AssimpBindings
{
    NativeClassDef *g_sceneClass    = nullptr;
    NativeClassDef *g_meshClass     = nullptr;
    NativeClassDef *g_materialClass = nullptr;
    NativeClassDef *g_nodeClass     = nullptr;

    int push_nil1(Interpreter *vm)
    {
        vm->pushNil();
        return 1;
    }

    bool read_string_arg(const Value &v, const char **out, const char *fn, int idx)
    {
        if (!v.isString()) { Error("%s arg %d expects string", fn, idx); return false; }
        *out = v.asString()->chars();
        return true;
    }

    bool read_number_arg(const Value &v, double *out, const char *fn, int idx)
    {
        if (!v.isNumber()) { Error("%s arg %d expects number", fn, idx); return false; }
        *out = v.asDouble();
        return true;
    }

    SceneHandle *require_scene(void *instance, const char *fn)
    {
        SceneHandle *h = (SceneHandle *)instance;
        if (head -10 /media/projectos/projects/cpp/minirender/core/include/glad/glad.h || head -10>valid || !h->scene)
        {
            Error("%s: invalid or unloaded AssimpScene", fn);
            return nullptr;
        }
        return h;
    }

    MeshHandle *require_mesh(void *instance, const char *fn)
    {
        MeshHandle *h = (MeshHandle *)instance;
        if (head -10 /media/projectos/projects/cpp/minirender/core/include/glad/glad.h || head -10>mesh || head -10>owner || !h->owner->valid)
        {
            Error("%s: invalid AssimpMesh (scene freed?)", fn);
            return nullptr;
        }
        return h;
    }

    MaterialHandle *require_material(void *instance, const char *fn)
    {
        MaterialHandle *h = (MaterialHandle *)instance;
        if (head -10 /media/projectos/projects/cpp/minirender/core/include/glad/glad.h || head -10>mat || head -10>owner || !h->owner->valid)
        {
            Error("%s: invalid AssimpMaterial (scene freed?)", fn);
            return nullptr;
        }
        return h;
    }

    NodeHandle *require_node(void *instance, const char *fn)
    {
        NodeHandle *h = (NodeHandle *)instance;
        if (head -10 /media/projectos/projects/cpp/minirender/core/include/glad/glad.h || head -10>node || head -10>owner || !h->owner->valid)
        {
            Error("%s: invalid AssimpNode (scene freed?)", fn);
            return nullptr;
        }
        return h;
    }

    bool push_mesh(Interpreter *vm, SceneHandle *owner, const aiMesh *mesh)
    {
        if (!vm || !owner || !mesh || !g_meshClass) return false;
        Value value = vm->makeNativeClassInstance(false);
        NativeClassInstance *inst = value.asNativeClassInstance();
        if (!inst) return false;
        MeshHandle *h = new MeshHandle();
        h->owner = owner;
        h->mesh  = mesh;
        owner->addRef();
        inst->klass    = g_meshClass;
        inst->userData = h;
        vm->push(value);
        return true;
    }

    bool push_material(Interpreter *vm, SceneHandle *owner, const aiMaterial *mat)
    {
        if (!vm || !owner || !mat || !g_materialClass) return false;
        Value value = vm->makeNativeClassInstance(false);
        NativeClassInstance *inst = value.asNativeClassInstance();
        if (!inst) return false;
        MaterialHandle *h = new MaterialHandle();
        h->owner = owner;
        h->mat   = mat;
        owner->addRef();
        inst->klass    = g_materialClass;
        inst->userData = h;
        vm->push(value);
        return true;
    }

    bool push_node(Interpreter *vm, SceneHandle *owner, const aiNode *node)
    {
        if (!vm || !owner || !node || !g_nodeClass) return false;
        Value value = vm->makeNativeClassInstance(false);
        NativeClassInstance *inst = value.asNativeClassInstance();
        if (!inst) return false;
        NodeHandle *h = new NodeHandle();
        h->owner = owner;
        h->node  = node;
        owner->addRef();
        inst->klass    = g_nodeClass;
        inst->userData = h;
        vm->push(value);
        return true;
    }

} // namespace AssimpBindings
