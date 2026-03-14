#include "assimp_core.hpp"

namespace AssimpBindings
{
    static void *node_ctor_error(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm; (void)argCount; (void)args;
        Error("AssimpNode cannot be constructed directly; use AssimpScene.getRootNode() or AssimpNode.getChild()");
        return nullptr;
    }

    static void node_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        NodeHandle *h = (NodeHandle *)instance;
        if (!h) return;
        h->owner->release();
        delete h;
    }

    // getName() -> string
    static int node_get_name(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        NodeHandle *h = require_node(instance, "AssimpNode.getName()");
        if (!h) return push_nil1(vm);
        vm->pushString(h->node->mName.C_Str());
        return 1;
    }

    // getChildCount() -> int
    static int node_get_child_count(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        NodeHandle *h = require_node(instance, "AssimpNode.getChildCount()");
        if (!h) return push_nil1(vm);
        vm->pushInt((int)h->node->mNumChildren);
        return 1;
    }

    // getChild(idx) -> AssimpNode
    static int node_get_child(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 1) { Error("AssimpNode.getChild(idx)"); return push_nil1(vm); }
        NodeHandle *h = require_node(instance, "AssimpNode.getChild()");
        if (!h) return push_nil1(vm);
        double idxd = 0.0;
        if (!read_number_arg(args[0], &idxd, "AssimpNode.getChild()", 1)) return push_nil1(vm);
        int idx = (int)idxd;
        if (idx < 0 || idx >= (int)h->node->mNumChildren) { Error("AssimpNode.getChild(): index %d out of range", idx); return push_nil1(vm); }
        return push_node(vm, h->owner, h->node->mChildren[idx]) ? 1 : push_nil1(vm);
    }

    // getMeshCount() -> int  (number of mesh indices this node references)
    static int node_get_mesh_count(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        NodeHandle *h = require_node(instance, "AssimpNode.getMeshCount()");
        if (!h) return push_nil1(vm);
        vm->pushInt((int)h->node->mNumMeshes);
        return 1;
    }

    // getMeshIndex(i) -> int  (scene-level mesh index)
    static int node_get_mesh_index(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 1) { Error("AssimpNode.getMeshIndex(i)"); return push_nil1(vm); }
        NodeHandle *h = require_node(instance, "AssimpNode.getMeshIndex()");
        if (!h) return push_nil1(vm);
        double idxd = 0.0;
        if (!read_number_arg(args[0], &idxd, "AssimpNode.getMeshIndex()", 1)) return push_nil1(vm);
        int idx = (int)idxd;
        if (idx < 0 || idx >= (int)h->node->mNumMeshes) { Error("AssimpNode.getMeshIndex(): index %d out of range", idx); return push_nil1(vm); }
        vm->pushInt((int)h->node->mMeshes[idx]);
        return 1;
    }

    // getParent() -> AssimpNode or nil
    static int node_get_parent(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        NodeHandle *h = require_node(instance, "AssimpNode.getParent()");
        if (!h || !h->node->mParent) return push_nil1(vm);
        return push_node(vm, h->owner, h->node->mParent) ? 1 : push_nil1(vm);
    }

    // getTransform() -> flat array[16] row-major 4x4
    static int node_get_transform(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        NodeHandle *h = require_node(instance, "AssimpNode.getTransform()");
        if (!h) return push_nil1(vm);
        const aiMatrix4x4 &m = h->node->mTransformation;
        Value arr = vm->makeArray();
        ArrayInstance *a = arr.as.array;
        a->values.reserve(16);
        // row-major: a1..a4, b1..b4, c1..c4, d1..d4
        a->values.push(vm->makeDouble(m.a1)); a->values.push(vm->makeDouble(m.a2)); a->values.push(vm->makeDouble(m.a3)); a->values.push(vm->makeDouble(m.a4));
        a->values.push(vm->makeDouble(m.b1)); a->values.push(vm->makeDouble(m.b2)); a->values.push(vm->makeDouble(m.b3)); a->values.push(vm->makeDouble(m.b4));
        a->values.push(vm->makeDouble(m.c1)); a->values.push(vm->makeDouble(m.c2)); a->values.push(vm->makeDouble(m.c3)); a->values.push(vm->makeDouble(m.c4));
        a->values.push(vm->makeDouble(m.d1)); a->values.push(vm->makeDouble(m.d2)); a->values.push(vm->makeDouble(m.d3)); a->values.push(vm->makeDouble(m.d4));
        vm->push(arr);
        return 1;
    }

    void register_node(Interpreter &vm)
    {
        g_nodeClass = vm.registerNativeClass("AssimpNode", node_ctor_error, node_dtor, 0, false);
        vm.addNativeMethod(g_nodeClass, "getName",         node_get_name);
        vm.addNativeMethod(g_nodeClass, "getChildCount",   node_get_child_count);
        vm.addNativeMethod(g_nodeClass, "getChild",        node_get_child);
        vm.addNativeMethod(g_nodeClass, "getMeshCount",    node_get_mesh_count);
        vm.addNativeMethod(g_nodeClass, "getMeshIndex",    node_get_mesh_index);
        vm.addNativeMethod(g_nodeClass, "getParent",       node_get_parent);
        vm.addNativeMethod(g_nodeClass, "getTransform",    node_get_transform);
    }

} // namespace AssimpBindings
