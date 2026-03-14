#include "recast_core.hpp"

namespace RecastBindings
{
    // ── Constructor / Destructor ─────────────────────────────
    static void *navmesh_ctor(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm; (void)args;
        if (argCount != 0)
        {
            Error("NavMesh() expects no arguments");
            return nullptr;
        }
        NavMeshHandle *h = new NavMeshHandle();
        return h;
    }

    static void navmesh_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        NavMeshHandle *h = (NavMeshHandle *)instance;
        if (h) { h->destroy(); delete h; }
    }

    // ── NavMesh.build(verts, tris [, cellSize, cellHeight,
    //                  agentHeight, agentRadius, agentMaxClimb, agentMaxSlope])
    //   verts: flat array of numbers  [x0,y0,z0, x1,y1,z1, ...]
    //   tris:  flat array of integers [i0,j0,k0, i1,j1,k1, ...]
    //   Returns true on success
    static int navmesh_build(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 2)
        {
            Error("NavMesh.build(verts, tris [, cellSize, cellHeight, agentHeight, agentRadius, agentMaxClimb, agentMaxSlope])");
            return push_nil1(vm);
        }

        NavMeshHandle *h = (NavMeshHandle *)instance;
        if (!h) return push_nil1(vm);

        // Read verts array
        if (!args[0].isArray())
        {
            Error("NavMesh.build: arg 1 must be an array of numbers (x,y,z flat)");
            return push_nil1(vm);
        }
        ArrayInstance *vertsArr = args[0].asArray();
        int nVertsFlat = (int)vertsArr->values.size();
        if (nVertsFlat % 3 != 0 || nVertsFlat == 0)
        {
            Error("NavMesh.build: verts must have length divisible by 3");
            return push_nil1(vm);
        }
        int nVerts = nVertsFlat / 3;

        std::vector<float> verts(nVertsFlat);
        for (int i = 0; i < nVertsFlat; i++)
        {
            if (!vertsArr->values[i].isNumber()) { Error("NavMesh.build: verts[%d] is not a number", i); return push_nil1(vm); }
            verts[i] = (float)vertsArr->values[i].asDouble();
        }

        // Read tris array
        if (!args[1].isArray())
        {
            Error("NavMesh.build: arg 2 must be an array of integers (i0,j0,k0 flat)");
            return push_nil1(vm);
        }
        ArrayInstance *trisArr = args[1].asArray();
        int nTrisFlat = (int)trisArr->values.size();
        if (nTrisFlat % 3 != 0 || nTrisFlat == 0)
        {
            Error("NavMesh.build: tris must have length divisible by 3");
            return push_nil1(vm);
        }
        int nTris = nTrisFlat / 3;

        std::vector<int> tris(nTrisFlat);
        for (int i = 0; i < nTrisFlat; i++)
        {
            if (!trisArr->values[i].isNumber()) { Error("NavMesh.build: tris[%d] is not a number", i); return push_nil1(vm); }
            tris[i] = (int)trisArr->values[i].asDouble();
        }

        // Optional config overrides
        RecastConfig cfg;
        if (argCount > 2 && args[2].isNumber()) cfg.cellSize       = (float)args[2].asDouble();
        if (argCount > 3 && args[3].isNumber()) cfg.cellHeight      = (float)args[3].asDouble();
        if (argCount > 4 && args[4].isNumber()) cfg.agentHeight     = (float)args[4].asDouble();
        if (argCount > 5 && args[5].isNumber()) cfg.agentRadius     = (float)args[5].asDouble();
        if (argCount > 6 && args[6].isNumber()) cfg.agentMaxClimb   = (float)args[6].asDouble();
        if (argCount > 7 && args[7].isNumber()) cfg.agentMaxSlope   = (float)args[7].asDouble();

        // Destroy previous mesh if any
        h->destroy();

        bool ok = build_navmesh_internal(verts.data(), nVerts, tris.data(), nTris, cfg, h);
        vm->pushBool(ok);
        return 1;
    }

    // ── NavMesh.isValid() → bool ─────────────────────────────
    static int navmesh_is_valid(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0) { Error("NavMesh.isValid() expects 0 arguments"); return push_nil1(vm); }
        NavMeshHandle *h = (NavMeshHandle *)instance;
        vm->pushBool(h && h->valid);
        return 1;
    }

    // ── NavMesh.destroy() ────────────────────────────────────
    static int navmesh_destroy(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm; (void)args;
        if (argCount != 0) { Error("NavMesh.destroy() expects 0 arguments"); return 0; }
        NavMeshHandle *h = (NavMeshHandle *)instance;
        if (h) h->destroy();
        return 0;
    }

    // ── NavMesh.findPath(start, end [, extX, extY, extZ]) → array<Vector3>
    //   Returns flat array of Vector3 waypoints, or nil on failure.
    static int navmesh_find_path(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 2)
        {
            Error("NavMesh.findPath(start, end [, extX, extY, extZ])");
            return push_nil1(vm);
        }

        NavMeshHandle *h = require_navmesh(instance, "NavMesh.findPath()");
        if (!h) return push_nil1(vm);

        Vector3 startPt, endPt;
        if (!read_vector3_arg(args[0], &startPt, "NavMesh.findPath()", 1)) return push_nil1(vm);
        if (!read_vector3_arg(args[1], &endPt,   "NavMesh.findPath()", 2)) return push_nil1(vm);

        float ext[3] = {2.0f, 4.0f, 2.0f};
        if (argCount > 2 && args[2].isNumber()) ext[0] = (float)args[2].asDouble();
        if (argCount > 3 && args[3].isNumber()) ext[1] = (float)args[3].asDouble();
        if (argCount > 4 && args[4].isNumber()) ext[2] = (float)args[4].asDouble();

        dtQueryFilter filter;
        filter.setIncludeFlags(0xffff);
        filter.setExcludeFlags(0);

        const float sp[3] = {startPt.x, startPt.y, startPt.z};
        const float ep[3] = {endPt.x,   endPt.y,   endPt.z};

        dtPolyRef startRef = 0, endRef = 0;
        float nearestPt[3];
        h->navQuery->findNearestPoly(sp, ext, &filter, &startRef, nearestPt);
        h->navQuery->findNearestPoly(ep, ext, &filter, &endRef,   nearestPt);

        if (!startRef || !endRef)
            return push_nil1(vm);

        dtPolyRef polys[RC_MAX_POLYS];
        int       nPolys = 0;
        h->navQuery->findPath(startRef, endRef, sp, ep, &filter, polys, &nPolys, RC_MAX_POLYS);

        if (nPolys == 0)
            return push_nil1(vm);

        // Clamp end if partial path
        float endClamped[3];
        rcVcopy(endClamped, ep);
        if (polys[nPolys - 1] != endRef)
            h->navQuery->closestPointOnPoly(polys[nPolys - 1], ep, endClamped, nullptr);

        float straightPath[RC_MAX_SMOOTH * 3];
        unsigned char flags[RC_MAX_SMOOTH];
        dtPolyRef pathPolys[RC_MAX_SMOOTH];
        int nStraight = 0;
        h->navQuery->findStraightPath(sp, endClamped, polys, nPolys,
            straightPath, flags, pathPolys, &nStraight, RC_MAX_SMOOTH);

        if (nStraight == 0)
            return push_nil1(vm);

        // Build BuLang array of Vector3
        Value arrVal = vm->makeArray();
        ArrayInstance *arr = arrVal.as.array;

        for (int i = 0; i < nStraight; i++)
        {
            Vector3 pt;
            pt.x = straightPath[i * 3 + 0];
            pt.y = straightPath[i * 3 + 1];
            pt.z = straightPath[i * 3 + 2];

            Value ptVal = vm->createNativeStruct(g_vector3Def->id, 0, nullptr);
            NativeStructInstance *inst = ptVal.asNativeStructInstance();
            if (inst && inst->data)
                *(Vector3 *)inst->data = pt;
            arr->values.push(ptVal);
        }

        vm->push(arrVal);
        return 1;
    }

    // ── NavMesh.findNearestPoint(pos [, extX, extY, extZ]) → Vector3 | nil
    static int navmesh_find_nearest(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 1)
        {
            Error("NavMesh.findNearestPoint(pos [, extX, extY, extZ])");
            return push_nil1(vm);
        }

        NavMeshHandle *h = require_navmesh(instance, "NavMesh.findNearestPoint()");
        if (!h) return push_nil1(vm);

        Vector3 pos;
        if (!read_vector3_arg(args[0], &pos, "NavMesh.findNearestPoint()", 1)) return push_nil1(vm);

        float ext[3] = {2.0f, 4.0f, 2.0f};
        if (argCount > 1 && args[1].isNumber()) ext[0] = (float)args[1].asDouble();
        if (argCount > 2 && args[2].isNumber()) ext[1] = (float)args[2].asDouble();
        if (argCount > 3 && args[3].isNumber()) ext[2] = (float)args[3].asDouble();

        dtQueryFilter filter;
        filter.setIncludeFlags(0xffff);
        filter.setExcludeFlags(0);

        const float p[3] = {pos.x, pos.y, pos.z};
        dtPolyRef ref = 0;
        float nearestPt[3];
        dtStatus status = h->navQuery->findNearestPoly(p, ext, &filter, &ref, nearestPt);

        if (dtStatusFailed(status) || !ref)
            return push_nil1(vm);

        Vector3 result = {nearestPt[0], nearestPt[1], nearestPt[2]};
        return push_vector3(vm, result) ? 1 : push_nil1(vm);
    }

    // ── Registration ─────────────────────────────────────────
    void register_navmesh(Interpreter &vm)
    {
        g_navMeshClass = vm.addNativeClass("NavMesh",
            navmesh_ctor, navmesh_dtor, sizeof(void *));

        vm.addNativeMethod(g_navMeshClass, "build",           navmesh_build);
        vm.addNativeMethod(g_navMeshClass, "isValid",         navmesh_is_valid);
        vm.addNativeMethod(g_navMeshClass, "destroy",         navmesh_destroy);
        vm.addNativeMethod(g_navMeshClass, "findPath",        navmesh_find_path);
        vm.addNativeMethod(g_navMeshClass, "findNearestPoint",navmesh_find_nearest);
    }

} // namespace RecastBindings
