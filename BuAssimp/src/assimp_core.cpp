#include "assimp_core.hpp"

namespace AssimpBindings
{
    NativeClassDef  *g_sceneClass    = nullptr;
    NativeClassDef  *g_meshClass     = nullptr;
    NativeClassDef  *g_materialClass = nullptr;
    NativeClassDef  *g_nodeClass     = nullptr;
    NativeClassDef  *g_animationClass = nullptr;
    NativeClassDef  *g_nodeAnimClass  = nullptr;
    NativeStructDef *g_matrixDef     = nullptr;

    NativeStructDef *get_native_struct_def(Interpreter *vm, const char *name)
    {
        if (!vm || !name) return nullptr;
        Value sv;
        if (!vm->tryGetGlobal(name, &sv) || !sv.isNativeStruct()) return nullptr;
        Value iv = vm->createNativeStruct(sv.asNativeStructId(), 0, nullptr);
        NativeStructInstance *inst = iv.asNativeStructInstance();
        return inst ? inst->def : nullptr;
    }

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

    bool read_boolish_arg(const Value &v, bool *out, const char *fn, int idx)
    {
        if (!out) return false;
        if (v.isBool())   { *out = v.asBool(); return true; }
        if (v.isNumber()) { *out = (v.asDouble() != 0.0); return true; }
        Error("%s arg %d expects bool or number", fn, idx);
        return false;
    }

    bool push_matrix(Interpreter *vm, const aiMatrix4x4 &a)
    {
        if (!vm || !g_matrixDef) return false;
        Value out = vm->createNativeStruct(g_matrixDef->id, 0, nullptr);
        NativeStructInstance *inst = out.asNativeStructInstance();
        if (!inst || !inst->data) return false;

        // aiMatrix4x4 is row-major; Matrix (raymath) is column-major.
        float *f = (float *)inst->data;
        f[0]  = a.a1; f[1]  = a.b1; f[2]  = a.c1; f[3]  = a.d1;
        f[4]  = a.a2; f[5]  = a.b2; f[6]  = a.c2; f[7]  = a.d2;
        f[8]  = a.a3; f[9]  = a.b3; f[10] = a.c3; f[11] = a.d3;
        f[12] = a.a4; f[13] = a.b4; f[14] = a.c4; f[15] = a.d4;

        vm->push(out);
        return true;
    }

    SceneHandle *require_scene(void *instance, const char *fn)
    {
        SceneHandle *h = (SceneHandle *)instance;
        if (!h || !h->valid || !h->scene)
        {
            Error("%s: invalid or unloaded AssimpScene", fn);
            return nullptr;
        }
        return h;
    }

    MeshHandle *require_mesh(void *instance, const char *fn)
    {
        MeshHandle *h = (MeshHandle *)instance;
        if (!h || !h->mesh || !h->owner || !h->owner->valid)
        {
            Error("%s: invalid AssimpMesh (scene freed?)", fn);
            return nullptr;
        }
        return h;
    }

    MaterialHandle *require_material(void *instance, const char *fn)
    {
        MaterialHandle *h = (MaterialHandle *)instance;
        if (!h || !h->mat || !h->owner || !h->owner->valid)
        {
            Error("%s: invalid AssimpMaterial (scene freed?)", fn);
            return nullptr;
        }
        return h;
    }

    NodeHandle *require_node(void *instance, const char *fn)
    {
        NodeHandle *h = (NodeHandle *)instance;
        if (!h || !h->node || !h->owner || !h->owner->valid)
        {
            Error("%s: invalid AssimpNode (scene freed?)", fn);
            return nullptr;
        }
        return h;
    }

    AnimationHandle *require_animation(void *instance, const char *fn)
    {
        AnimationHandle *h = (AnimationHandle *)instance;
        if (!h || !h->anim || !h->owner || !h->owner->valid)
        {
            Error("%s: invalid AssimpAnimation (scene freed?)", fn);
            return nullptr;
        }
        return h;
    }

    NodeAnimHandle *require_node_anim(void *instance, const char *fn)
    {
        NodeAnimHandle *h = (NodeAnimHandle *)instance;
        if (!h || !h->channel || !h->anim || !h->owner || !h->owner->valid)
        {
            Error("%s: invalid AssimpNodeAnim (scene freed?)", fn);
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
        inst->klass    = g_nodeClass;
        inst->userData = h;
        vm->push(value);
        return true;
    }

    bool push_animation(Interpreter *vm, SceneHandle *owner, const aiAnimation *anim)
    {
        if (!vm || !owner || !anim || !g_animationClass) return false;
        Value value = vm->makeNativeClassInstance(false);
        NativeClassInstance *inst = value.asNativeClassInstance();
        if (!inst) return false;
        AnimationHandle *h = new AnimationHandle();
        h->owner = owner;
        h->anim  = anim;
        inst->klass = g_animationClass;
        inst->userData = h;
        vm->push(value);
        return true;
    }

    bool push_node_anim(Interpreter *vm, SceneHandle *owner, const aiAnimation *anim, const aiNodeAnim *channel)
    {
        if (!vm || !owner || !anim || !channel || !g_nodeAnimClass) return false;
        Value value = vm->makeNativeClassInstance(false);
        NativeClassInstance *inst = value.asNativeClassInstance();
        if (!inst) return false;
        NodeAnimHandle *h = new NodeAnimHandle();
        h->owner = owner;
        h->anim = anim;
        h->channel = channel;
        inst->klass = g_nodeAnimClass;
        inst->userData = h;
        vm->push(value);
        return true;
    }

} // namespace AssimpBindings
