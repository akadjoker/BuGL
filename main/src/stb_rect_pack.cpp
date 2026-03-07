#include "bindings.hpp"
 #define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#include <limits>
#include <vector>

namespace SDLBindings
{
    static constexpr const char *kClassRectPack = "RectPack";

    struct RectPackData
    {
        stbrp_context context;
        std::vector<stbrp_node> nodes;
        std::vector<stbrp_rect> rects;
        int width;
        int height;
        int initialized;
        int nextId;
    };

    static RectPackData *as_rectpack(void *instance)
    {
        return (RectPackData *)instance;
    }

    static bool init_rectpack(RectPackData *rp, int width, int height, int nodeCount)
    {
        if (!rp || width <= 0 || height <= 0)
            return false;

        if (nodeCount <= 0)
            nodeCount = width;
        if (nodeCount <= 0)
            return false;

        rp->nodes.clear();
        rp->nodes.resize((size_t)nodeCount);
        rp->rects.clear();
        rp->nextId = 0;
        rp->width = width;
        rp->height = height;
        rp->initialized = 1;
        stbrp_init_target(&rp->context, width, height, rp->nodes.data(), nodeCount);
        return true;
    }

    static void *rectpack_ctor(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm;
        RectPackData *rp = new RectPackData();
        rp->width = 0;
        rp->height = 0;
        rp->initialized = 0;
        rp->nextId = 0;

        if (argCount == 0)
            return rp;

        if (argCount < 2 || argCount > 3 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("RectPack expects 0 args or (width, height[, nodeCount])");
            delete rp;
            return nullptr;
        }

        int nodeCount = (argCount == 3) ? args[2].asInt() : args[0].asInt();
        if (!init_rectpack(rp, args[0].asInt(), args[1].asInt(), nodeCount))
        {
            Error("RectPack failed to initialize");
            delete rp;
            return nullptr;
        }
        return rp;
    }

    static void rectpack_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        RectPackData *rp = as_rectpack(instance);
        if (!rp)
            return;
        delete rp;
    }

    static int rectpack_init(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        RectPackData *rp = as_rectpack(instance);
        if (!rp || argCount < 2 || argCount > 3 || !args[0].isNumber() || !args[1].isNumber())
        {
            Error("RectPack.init() expects (width, height[, nodeCount])");
            return 0;
        }

        int nodeCount = (argCount == 3) ? args[2].asInt() : args[0].asInt();
        const int ok = init_rectpack(rp, args[0].asInt(), args[1].asInt(), nodeCount) ? 1 : 0;
        vm->pushInt(ok);
        return 1;
    }

    static int rectpack_clear(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        RectPackData *rp = as_rectpack(instance);
        if (!rp || argCount != 0)
        {
            Error("RectPack.clear() expects 0 arguments");
            return 0;
        }

        rp->rects.clear();
        rp->nextId = 0;
        if (rp->initialized && !rp->nodes.empty())
            stbrp_init_target(&rp->context, rp->width, rp->height, rp->nodes.data(), (int)rp->nodes.size());
        vm->pushInt(1);
        return 1;
    }

    static int rectpack_add(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        RectPackData *rp = as_rectpack(instance);
        if (!rp || (argCount != 2 && argCount != 3))
        {
            Error("RectPack.add() expects (w, h) or (id, w, h)");
            return 0;
        }
        if (!rp->initialized)
        {
            Error("RectPack.add() called before init()");
            return 0;
        }

        int id = 0;
        int w = 0;
        int h = 0;
        if (argCount == 2)
        {
            if (!args[0].isNumber() || !args[1].isNumber())
            {
                Error("RectPack.add() expects numeric arguments");
                return 0;
            }
            id = rp->nextId++;
            w = args[0].asInt();
            h = args[1].asInt();
        }
        else
        {
            if (!args[0].isNumber() || !args[1].isNumber() || !args[2].isNumber())
            {
                Error("RectPack.add() expects numeric arguments");
                return 0;
            }
            id = args[0].asInt();
            w = args[1].asInt();
            h = args[2].asInt();
            if (id >= rp->nextId)
                rp->nextId = id + 1;
        }

        if (w <= 0 || h <= 0 || w > STBRP__MAXVAL || h > STBRP__MAXVAL)
        {
            Error("RectPack.add() invalid rectangle size");
            return 0;
        }

        stbrp_rect r = {};
        r.id = id;
        r.w = (stbrp_coord)w;
        r.h = (stbrp_coord)h;
        r.x = 0;
        r.y = 0;
        r.was_packed = 0;
        rp->rects.push_back(r);

        vm->pushInt(id);
        return 1;
    }

    static int rectpack_pack(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        RectPackData *rp = as_rectpack(instance);
        if (!rp || argCount != 0)
        {
            Error("RectPack.pack() expects 0 arguments");
            return 0;
        }
        if (!rp->initialized)
        {
            Error("RectPack.pack() called before init()");
            return 0;
        }

        int allPacked = 1;
        if (!rp->rects.empty())
            allPacked = stbrp_pack_rects(&rp->context, rp->rects.data(), (int)rp->rects.size()) ? 1 : 0;

        Value out = vm->makeArray();
        ArrayInstance *arr = out.asArray();
        arr->values.reserve(rp->rects.size() * 6u);
        for (const stbrp_rect &r : rp->rects)
        {
            arr->values.push(vm->makeInt(r.id));
            arr->values.push(vm->makeInt((int)r.x));
            arr->values.push(vm->makeInt((int)r.y));
            arr->values.push(vm->makeInt((int)r.w));
            arr->values.push(vm->makeInt((int)r.h));
            arr->values.push(vm->makeInt(r.was_packed ? 1 : 0));
        }

        vm->pushInt(allPacked);
        vm->push(out);
        return 2;
    }

    static int rectpack_results(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        RectPackData *rp = as_rectpack(instance);
        if (!rp || argCount != 0)
        {
            Error("RectPack.results() expects 0 arguments");
            return 0;
        }

        Value out = vm->makeArray();
        ArrayInstance *arr = out.asArray();
        arr->values.reserve(rp->rects.size() * 6u);
        for (const stbrp_rect &r : rp->rects)
        {
            arr->values.push(vm->makeInt(r.id));
            arr->values.push(vm->makeInt((int)r.x));
            arr->values.push(vm->makeInt((int)r.y));
            arr->values.push(vm->makeInt((int)r.w));
            arr->values.push(vm->makeInt((int)r.h));
            arr->values.push(vm->makeInt(r.was_packed ? 1 : 0));
        }
        vm->push(out);
        return 1;
    }

    static int rectpack_allow_oom(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        RectPackData *rp = as_rectpack(instance);
        if (!rp || argCount != 1 || !args[0].isNumber())
        {
            Error("RectPack.allowOutOfMem() expects (flag)");
            return 0;
        }
        if (!rp->initialized)
        {
            Error("RectPack.allowOutOfMem() called before init()");
            return 0;
        }

        stbrp_setup_allow_out_of_mem(&rp->context, args[0].asInt() ? 1 : 0);
        vm->pushInt(1);
        return 1;
    }

    static int rectpack_heuristic(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        RectPackData *rp = as_rectpack(instance);
        if (!rp || argCount != 1 || !args[0].isNumber())
        {
            Error("RectPack.heuristic() expects (value)");
            return 0;
        }
        if (!rp->initialized)
        {
            Error("RectPack.heuristic() called before init()");
            return 0;
        }

        const int h = args[0].asInt();
        if (h != STBRP_HEURISTIC_Skyline_BL_sortHeight && h != STBRP_HEURISTIC_Skyline_BF_sortHeight)
        {
            Error("RectPack.heuristic() invalid value");
            return 0;
        }
        stbrp_setup_heuristic(&rp->context, h);
        vm->pushInt(1);
        return 1;
    }

    static Value rectpack_get_width(Interpreter *vm, void *instance)
    {
        RectPackData *rp = as_rectpack(instance);
        return vm->makeInt(rp ? rp->width : 0);
    }

    static Value rectpack_get_height(Interpreter *vm, void *instance)
    {
        RectPackData *rp = as_rectpack(instance);
        return vm->makeInt(rp ? rp->height : 0);
    }

    static Value rectpack_get_count(Interpreter *vm, void *instance)
    {
        RectPackData *rp = as_rectpack(instance);
        if (!rp)
            return vm->makeInt(0);
        if (rp->rects.size() > (size_t)std::numeric_limits<int>::max())
            return vm->makeInt(0);
        return vm->makeInt((int)rp->rects.size());
    }

    static Value rectpack_get_initialized(Interpreter *vm, void *instance)
    {
        RectPackData *rp = as_rectpack(instance);
        return vm->makeInt((rp && rp->initialized) ? 1 : 0);
    }

    void register_stb_rect_pack(ModuleBuilder &module, Interpreter &vm)
    {
        NativeClassDef *rpClass = vm.registerNativeClass(kClassRectPack, rectpack_ctor, rectpack_dtor, -1, false);
        vm.addNativeMethod(rpClass, "init", rectpack_init);
        vm.addNativeMethod(rpClass, "clear", rectpack_clear);
        vm.addNativeMethod(rpClass, "add", rectpack_add);
        vm.addNativeMethod(rpClass, "pack", rectpack_pack);
        vm.addNativeMethod(rpClass, "results", rectpack_results);
        vm.addNativeMethod(rpClass, "allowOutOfMem", rectpack_allow_oom);
        vm.addNativeMethod(rpClass, "heuristic", rectpack_heuristic);
        vm.addNativeProperty(rpClass, "width", rectpack_get_width, nullptr);
        vm.addNativeProperty(rpClass, "height", rectpack_get_height, nullptr);
        vm.addNativeProperty(rpClass, "count", rectpack_get_count, nullptr);
        vm.addNativeProperty(rpClass, "initialized", rectpack_get_initialized, nullptr);

        module.addInt("STBRP_HEURISTIC_Skyline_BL_sortHeight", STBRP_HEURISTIC_Skyline_BL_sortHeight)
            .addInt("STBRP_HEURISTIC_Skyline_BF_sortHeight", STBRP_HEURISTIC_Skyline_BF_sortHeight);
    }
}
