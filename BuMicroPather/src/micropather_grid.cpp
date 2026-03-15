#include "micropather_core.hpp"

#include <algorithm>

namespace MicroPatherBindings
{
    static void *grid_ctor(Interpreter *vm, int argCount, Value *args)
    {
        if (argCount < 2 || argCount > 3)
        {
            Error("GridPathfinder(width, height [, allowDiagonal])");
            return nullptr;
        }

        int width = 0, height = 0;
        if (!read_int_arg(args[0], &width, "GridPathfinder()", 1) ||
            !read_int_arg(args[1], &height, "GridPathfinder()", 2))
        {
            return nullptr;
        }

        if (width < 1 || height < 1)
        {
            Error("GridPathfinder(): width and height must be >= 1");
            return nullptr;
        }

        bool diagonal = false;
        if (argCount == 3 && !read_boolish_arg(args[2], &diagonal, "GridPathfinder()", 3))
            return nullptr;

        GridPathfinderHandle *h = new GridPathfinderHandle();
        h->graph = new GridGraph(width, height, diagonal);

        const long long totalStatesLL = (long long)width * (long long)height;
        const unsigned totalStates = (unsigned)std::max(1ll, std::min(totalStatesLL, 2000000ll));
        const unsigned allocate = std::max(64u, std::min(4096u, totalStates));
        const unsigned typicalAdjacent = diagonal ? 8u : 4u;
        h->solver = new micropather::MicroPather(h->graph, allocate, typicalAdjacent, true);
        h->valid = true;
        h->lastStatus = micropather::MicroPather::NO_SOLUTION;
        h->lastCost = 0.0f;
        return h;
    }

    static void grid_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        GridPathfinderHandle *h = (GridPathfinderHandle *)instance;
        if (!h)
            return;
        h->destroy();
        delete h;
    }

    static int grid_destroy(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("GridPathfinder.destroy() expects 0 arguments");
            return 0;
        }

        GridPathfinderHandle *h = (GridPathfinderHandle *)instance;
        if (h)
            h->destroy();
        return 0;
    }

    static int grid_is_valid(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("GridPathfinder.isValid() expects 0 arguments");
            return push_nil1(vm);
        }

        GridPathfinderHandle *h = (GridPathfinderHandle *)instance;
        vm->pushBool(h && h->valid && h->graph && h->solver);
        return 1;
    }

    static int grid_get_width(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("GridPathfinder.getWidth() expects 0 arguments");
            return push_nil1(vm);
        }
        GridPathfinderHandle *h = require_grid(instance, "GridPathfinder.getWidth()");
        if (!h)
            return push_nil1(vm);
        vm->push(vm->makeInt(h->graph->width));
        return 1;
    }

    static int grid_get_height(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("GridPathfinder.getHeight() expects 0 arguments");
            return push_nil1(vm);
        }
        GridPathfinderHandle *h = require_grid(instance, "GridPathfinder.getHeight()");
        if (!h)
            return push_nil1(vm);
        vm->push(vm->makeInt(h->graph->height));
        return 1;
    }

    static int grid_set_diagonal(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("GridPathfinder.setDiagonal(enabled)");
            return 0;
        }
        GridPathfinderHandle *h = require_grid(instance, "GridPathfinder.setDiagonal()");
        if (!h)
            return 0;

        bool enabled = false;
        if (!read_boolish_arg(args[0], &enabled, "GridPathfinder.setDiagonal()", 1))
            return 0;

        h->graph->setDiagonal(enabled);
        h->solver->Reset();
        return 0;
    }

    static int grid_clear(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        GridPathfinderHandle *h = require_grid(instance, "GridPathfinder.clear()");
        if (!h)
            return 0;

        bool blocked = false;
        if (argCount > 1)
        {
            Error("GridPathfinder.clear([blocked])");
            return 0;
        }
        if (argCount == 1 && !read_boolish_arg(args[0], &blocked, "GridPathfinder.clear()", 1))
            return 0;

        h->graph->clear(blocked);
        h->solver->Reset();
        return 0;
    }

    static int grid_reset_cache(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("GridPathfinder.resetCache() expects 0 arguments");
            return 0;
        }

        GridPathfinderHandle *h = require_grid(instance, "GridPathfinder.resetCache()");
        if (!h)
            return 0;
        h->solver->Reset();
        return 0;
    }

    static int grid_in_bounds(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("GridPathfinder.inBounds(x, y)");
            return push_nil1(vm);
        }
        GridPathfinderHandle *h = require_grid(instance, "GridPathfinder.inBounds()");
        if (!h)
            return push_nil1(vm);

        int x = 0, y = 0;
        if (!read_int_arg(args[0], &x, "GridPathfinder.inBounds()", 1) ||
            !read_int_arg(args[1], &y, "GridPathfinder.inBounds()", 2))
        {
            return push_nil1(vm);
        }

        vm->pushBool(h->graph->inBounds(x, y));
        return 1;
    }

    static int grid_set_blocked(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 3)
        {
            Error("GridPathfinder.setBlocked(x, y, blocked)");
            return push_nil1(vm);
        }
        GridPathfinderHandle *h = require_grid(instance, "GridPathfinder.setBlocked()");
        if (!h)
            return push_nil1(vm);

        int x = 0, y = 0;
        bool blocked = false;
        if (!read_int_arg(args[0], &x, "GridPathfinder.setBlocked()", 1) ||
            !read_int_arg(args[1], &y, "GridPathfinder.setBlocked()", 2) ||
            !read_boolish_arg(args[2], &blocked, "GridPathfinder.setBlocked()", 3))
        {
            return push_nil1(vm);
        }

        if (!h->graph->inBounds(x, y))
        {
            vm->pushBool(false);
            return 1;
        }

        h->graph->setBlocked(x, y, blocked);
        h->solver->Reset();
        vm->pushBool(true);
        return 1;
    }

    static int grid_is_blocked(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("GridPathfinder.isBlocked(x, y)");
            return push_nil1(vm);
        }
        GridPathfinderHandle *h = require_grid(instance, "GridPathfinder.isBlocked()");
        if (!h)
            return push_nil1(vm);

        int x = 0, y = 0;
        if (!read_int_arg(args[0], &x, "GridPathfinder.isBlocked()", 1) ||
            !read_int_arg(args[1], &y, "GridPathfinder.isBlocked()", 2))
        {
            return push_nil1(vm);
        }
        vm->pushBool(h->graph->isBlocked(x, y));
        return 1;
    }

    static int grid_set_rect_blocked(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 5)
        {
            Error("GridPathfinder.setRectBlocked(x, y, w, h, blocked)");
            return push_nil1(vm);
        }
        GridPathfinderHandle *h = require_grid(instance, "GridPathfinder.setRectBlocked()");
        if (!h)
            return push_nil1(vm);

        int x = 0, y = 0, w = 0, hh = 0;
        bool blocked = false;
        if (!read_int_arg(args[0], &x, "GridPathfinder.setRectBlocked()", 1) ||
            !read_int_arg(args[1], &y, "GridPathfinder.setRectBlocked()", 2) ||
            !read_int_arg(args[2], &w, "GridPathfinder.setRectBlocked()", 3) ||
            !read_int_arg(args[3], &hh, "GridPathfinder.setRectBlocked()", 4) ||
            !read_boolish_arg(args[4], &blocked, "GridPathfinder.setRectBlocked()", 5))
        {
            return push_nil1(vm);
        }

        if (w < 0 || hh < 0)
        {
            Error("GridPathfinder.setRectBlocked(): w and h must be >= 0");
            return push_nil1(vm);
        }

        int changed = h->graph->setRectBlocked(x, y, w, hh, blocked);
        h->solver->Reset();
        vm->push(vm->makeInt(changed));
        return 1;
    }

    static int grid_get_last_status(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("GridPathfinder.getLastStatus() expects 0 arguments");
            return push_nil1(vm);
        }
        GridPathfinderHandle *h = require_grid(instance, "GridPathfinder.getLastStatus()");
        if (!h)
            return push_nil1(vm);
        vm->push(vm->makeInt(h->lastStatus));
        return 1;
    }

    static int grid_get_last_cost(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("GridPathfinder.getLastCost() expects 0 arguments");
            return push_nil1(vm);
        }
        GridPathfinderHandle *h = require_grid(instance, "GridPathfinder.getLastCost()");
        if (!h)
            return push_nil1(vm);
        vm->push(vm->makeDouble(h->lastCost));
        return 1;
    }

    static int grid_solve(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 4)
        {
            Error("GridPathfinder.solve(startX, startY, goalX, goalY)");
            return push_nil1(vm);
        }

        GridPathfinderHandle *h = require_grid(instance, "GridPathfinder.solve()");
        if (!h)
            return push_nil1(vm);

        int sx = 0, sy = 0, gx = 0, gy = 0;
        if (!read_int_arg(args[0], &sx, "GridPathfinder.solve()", 1) ||
            !read_int_arg(args[1], &sy, "GridPathfinder.solve()", 2) ||
            !read_int_arg(args[2], &gx, "GridPathfinder.solve()", 3) ||
            !read_int_arg(args[3], &gy, "GridPathfinder.solve()", 4))
        {
            return push_nil1(vm);
        }

        if (!h->graph->inBounds(sx, sy) || !h->graph->inBounds(gx, gy) ||
            h->graph->isBlocked(sx, sy) || h->graph->isBlocked(gx, gy))
        {
            h->lastStatus = micropather::MicroPather::NO_SOLUTION;
            h->lastCost = 0.0f;
            return push_nil1(vm);
        }

        void *startState = h->graph->encodeState(sx, sy);
        void *goalState = h->graph->encodeState(gx, gy);
        if (!startState || !goalState)
        {
            h->lastStatus = micropather::MicroPather::NO_SOLUTION;
            h->lastCost = 0.0f;
            return push_nil1(vm);
        }

        micropather::MPVector<void *> path;
        float totalCost = 0.0f;
        int status = h->solver->Solve(startState, goalState, &path, &totalCost);
        h->lastStatus = status;
        h->lastCost = totalCost;

        if (status == micropather::MicroPather::NO_SOLUTION)
            return push_nil1(vm);

        Value outVal = vm->makeArray();
        ArrayInstance *outArr = outVal.asArray();
        if (outArr)
            outArr->values.reserve(path.size() * 2u);

        for (unsigned i = 0; i < path.size(); ++i)
        {
            int x = 0, y = 0;
            if (!h->graph->decodeState(path[i], &x, &y))
                continue;
            outArr->values.push(vm->makeInt(x));
            outArr->values.push(vm->makeInt(y));
        }

        if (status == micropather::MicroPather::START_END_SAME && outArr->values.empty())
        {
            outArr->values.push(vm->makeInt(sx));
            outArr->values.push(vm->makeInt(sy));
        }

        vm->push(outVal);
        return 1;
    }

    void register_grid(Interpreter &vm)
    {
        g_gridPathfinderClass = vm.registerNativeClass("GridPathfinder",
            grid_ctor, grid_dtor, -1, false);

        vm.addNativeMethod(g_gridPathfinderClass, "destroy",       grid_destroy);
        vm.addNativeMethod(g_gridPathfinderClass, "isValid",       grid_is_valid);
        vm.addNativeMethod(g_gridPathfinderClass, "getWidth",      grid_get_width);
        vm.addNativeMethod(g_gridPathfinderClass, "getHeight",     grid_get_height);
        vm.addNativeMethod(g_gridPathfinderClass, "setDiagonal",   grid_set_diagonal);
        vm.addNativeMethod(g_gridPathfinderClass, "clear",         grid_clear);
        vm.addNativeMethod(g_gridPathfinderClass, "resetCache",    grid_reset_cache);
        vm.addNativeMethod(g_gridPathfinderClass, "inBounds",      grid_in_bounds);
        vm.addNativeMethod(g_gridPathfinderClass, "setBlocked",    grid_set_blocked);
        vm.addNativeMethod(g_gridPathfinderClass, "isBlocked",     grid_is_blocked);
        vm.addNativeMethod(g_gridPathfinderClass, "setRectBlocked",grid_set_rect_blocked);
        vm.addNativeMethod(g_gridPathfinderClass, "solve",         grid_solve);
        vm.addNativeMethod(g_gridPathfinderClass, "getLastStatus", grid_get_last_status);
        vm.addNativeMethod(g_gridPathfinderClass, "getLastCost",   grid_get_last_cost);
    }
}
