#include "recast_core.hpp"

namespace RecastBindings
{
    NativeClassDef  *g_navMeshClass  = nullptr;
    NativeClassDef  *g_navCrowdClass = nullptr;
    NativeClassDef  *g_navTileCacheClass = nullptr;
    NativeStructDef *g_vector3Def    = nullptr;

    // ── Basic helpers ────────────────────────────────────────
    int push_nil1(Interpreter *vm)
    {
        vm->pushNil();
        return 1;
    }

    NativeStructDef *get_native_struct_def(Interpreter *vm, const char *name)
    {
        if (!vm || !name) return nullptr;
        Value v;
        if (!vm->tryGetGlobal(name, &v) || !v.isNativeStruct()) return nullptr;
        Value inst = vm->createNativeStruct(v.asNativeStructId(), 0, nullptr);
        NativeStructInstance *si = inst.asNativeStructInstance();
        return si ? si->def : nullptr;
    }

    bool read_number_arg(const Value &v, double *out, const char *fn, int idx)
    {
        if (!out || !v.isNumber())
        {
            Error("%s arg %d expects number", fn, idx);
            return false;
        }
        *out = v.asNumber();
        return true;
    }

    bool read_vector3_arg(const Value &v, Vector3 *out, const char *fn, int idx)
    {
        if (!out || !v.isNativeStructInstance())
        {
            Error("%s arg %d expects Vector3", fn, idx);
            return false;
        }
        NativeStructInstance *inst = v.asNativeStructInstance();
        if (!inst || !inst->data || (g_vector3Def != nullptr && inst->def != g_vector3Def))
        {
            Error("%s arg %d expects Vector3", fn, idx);
            return false;
        }
        *out = *(Vector3 *)inst->data;
        return true;
    }

    bool push_vector3(Interpreter *vm, const Vector3 &v)
    {
        if (!vm || !g_vector3Def) return false;
        Value out = vm->createNativeStruct(g_vector3Def->id, 0, nullptr);
        NativeStructInstance *inst = out.asNativeStructInstance();
        if (!inst || !inst->data) return false;
        *(Vector3 *)inst->data = v;
        vm->push(out);
        return true;
    }

    NavMeshHandle *require_navmesh(void *instance, const char *fn)
    {
        NavMeshHandle *h = (NavMeshHandle *)instance;
        if (!h || !h->valid || !h->navMesh || !h->navQuery)
        {
            Error("%s: invalid or destroyed NavMesh", fn);
            return nullptr;
        }
        return h;
    }

    NavCrowdHandle *require_crowd(void *instance, const char *fn)
    {
        NavCrowdHandle *h = (NavCrowdHandle *)instance;
        if (!h || !h->valid || !h->crowd || !h->mesh || !h->mesh->valid || !h->mesh->navMesh || !h->mesh->navQuery)
        {
            Error("%s: invalid or destroyed NavCrowd", fn);
            return nullptr;
        }
        return h;
    }

    NavTileCacheHandle *require_tilecache(void *instance, const char *fn)
    {
        NavTileCacheHandle *h = (NavTileCacheHandle *)instance;
        if (!h || !h->valid || !h->tileCache || !h->navMesh || !h->navQuery)
        {
            Error("%s: invalid or destroyed NavTileCache", fn);
            return nullptr;
        }
        return h;
    }

    // ── Full Recast → Detour pipeline ────────────────────────
    bool build_navmesh_internal(
        const float *verts, int nVerts,
        const int   *tris,  int nTris,
        const RecastConfig &rcfg,
        NavMeshHandle *out)
    {
        rcContext ctx(false);

        float bmin[3], bmax[3];
        rcCalcBounds(verts, nVerts, bmin, bmax);

        rcConfig cfg;
        memset(&cfg, 0, sizeof(cfg));
        cfg.cs                 = rcfg.cellSize;
        cfg.ch                 = rcfg.cellHeight;
        cfg.walkableSlopeAngle = rcfg.agentMaxSlope;
        cfg.walkableHeight     = (int)ceilf(rcfg.agentHeight   / cfg.ch);
        cfg.walkableClimb      = (int)floorf(rcfg.agentMaxClimb / cfg.ch);
        cfg.walkableRadius     = (int)ceilf(rcfg.agentRadius    / cfg.cs);
        cfg.maxEdgeLen         = (int)(rcfg.edgeMaxLen / cfg.cs);
        cfg.maxSimplificationError = rcfg.edgeMaxError;
        cfg.minRegionArea      = (int)rcSqr(rcfg.regionMinSize);
        cfg.mergeRegionArea    = (int)rcSqr(rcfg.regionMergeSize);
        cfg.maxVertsPerPoly    = rcfg.vertsPerPoly;
        cfg.detailSampleDist   = rcfg.detailSampleDist < 0.9f ? 0.0f : cfg.cs * rcfg.detailSampleDist;
        cfg.detailSampleMaxError = cfg.ch * rcfg.detailSampleMaxError;
        rcCalcGridSize(bmin, bmax, cfg.cs, &cfg.width, &cfg.height);
        rcVcopy(cfg.bmin, bmin);
        rcVcopy(cfg.bmax, bmax);

        // 1. Heightfield
        rcHeightfield *hf = rcAllocHeightfield();
        if (!rcCreateHeightfield(&ctx, *hf, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch))
        {
            rcFreeHeightField(hf);
            return false;
        }

        std::vector<unsigned char> areas(nTris, 0);
        rcMarkWalkableTriangles(&ctx, cfg.walkableSlopeAngle, verts, nVerts, tris, nTris, areas.data());
        if (!rcRasterizeTriangles(&ctx, verts, nVerts, tris, areas.data(), nTris, *hf, cfg.walkableClimb))
        {
            rcFreeHeightField(hf);
            return false;
        }

        rcFilterLowHangingWalkableObstacles(&ctx, cfg.walkableClimb, *hf);
        rcFilterLedgeSpans(&ctx, cfg.walkableHeight, cfg.walkableClimb, *hf);
        rcFilterWalkableLowHeightSpans(&ctx, cfg.walkableHeight, *hf);

        // 2. Compact heightfield
        rcCompactHeightfield *chf = rcAllocCompactHeightfield();
        if (!rcBuildCompactHeightfield(&ctx, cfg.walkableHeight, cfg.walkableClimb, *hf, *chf))
        {
            rcFreeHeightField(hf);
            rcFreeCompactHeightfield(chf);
            return false;
        }
        rcFreeHeightField(hf);

        if (!rcErodeWalkableArea(&ctx, cfg.walkableRadius, *chf) ||
            !rcBuildDistanceField(&ctx, *chf) ||
            !rcBuildRegions(&ctx, *chf, 0, cfg.minRegionArea, cfg.mergeRegionArea))
        {
            rcFreeCompactHeightfield(chf);
            return false;
        }

        // 3. Contours
        rcContourSet *cset = rcAllocContourSet();
        if (!rcBuildContours(&ctx, *chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *cset))
        {
            rcFreeCompactHeightfield(chf);
            rcFreeContourSet(cset);
            return false;
        }

        // 4. Poly mesh
        rcPolyMesh *pmesh = rcAllocPolyMesh();
        if (!rcBuildPolyMesh(&ctx, *cset, cfg.maxVertsPerPoly, *pmesh))
        {
            rcFreeCompactHeightfield(chf);
            rcFreeContourSet(cset);
            rcFreePolyMesh(pmesh);
            return false;
        }

        // 5. Detail mesh
        rcPolyMeshDetail *dmesh = rcAllocPolyMeshDetail();
        if (!rcBuildPolyMeshDetail(&ctx, *pmesh, *chf, cfg.detailSampleDist, cfg.detailSampleMaxError, *dmesh))
        {
            rcFreeCompactHeightfield(chf);
            rcFreeContourSet(cset);
            rcFreePolyMesh(pmesh);
            rcFreePolyMeshDetail(dmesh);
            return false;
        }
        rcFreeCompactHeightfield(chf);
        rcFreeContourSet(cset);

        // Mark all polys walkable
        for (int i = 0; i < pmesh->npolys; ++i)
            pmesh->flags[i] = 1;

        // 6. Detour navmesh data
        dtNavMeshCreateParams params;
        memset(&params, 0, sizeof(params));
        params.verts             = pmesh->verts;
        params.vertCount         = pmesh->nverts;
        params.polys             = pmesh->polys;
        params.polyAreas         = pmesh->areas;
        params.polyFlags         = pmesh->flags;
        params.polyCount         = pmesh->npolys;
        params.nvp               = pmesh->nvp;
        params.detailMeshes      = dmesh->meshes;
        params.detailVerts       = dmesh->verts;
        params.detailVertsCount  = dmesh->nverts;
        params.detailTris        = dmesh->tris;
        params.detailTriCount    = dmesh->ntris;
        params.walkableHeight    = rcfg.agentHeight;
        params.walkableRadius    = rcfg.agentRadius;
        params.walkableClimb     = rcfg.agentMaxClimb;
        rcVcopy(params.bmin, pmesh->bmin);
        rcVcopy(params.bmax, pmesh->bmax);
        params.cs         = cfg.cs;
        params.ch         = cfg.ch;
        params.buildBvTree = true;

        unsigned char *navData     = nullptr;
        int            navDataSize = 0;
        if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
        {
            rcFreePolyMesh(pmesh);
            rcFreePolyMeshDetail(dmesh);
            return false;
        }
        rcFreePolyMesh(pmesh);
        rcFreePolyMeshDetail(dmesh);

        // 7. Init dtNavMesh
        dtNavMesh *navMesh = dtAllocNavMesh();
        if (dtStatusFailed(navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA)))
        {
            dtFree(navData);
            dtFreeNavMesh(navMesh);
            return false;
        }

        dtNavMeshQuery *navQuery = dtAllocNavMeshQuery();
        if (dtStatusFailed(navQuery->init(navMesh, 2048)))
        {
            dtFreeNavMeshQuery(navQuery);
            dtFreeNavMesh(navMesh);
            return false;
        }

        out->navMesh  = navMesh;
        out->navQuery = navQuery;
        out->valid    = true;
        return true;
    }

} // namespace RecastBindings
