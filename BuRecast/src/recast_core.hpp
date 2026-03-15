#pragma once

#include "recast_bindings.hpp"
#include "../../main/src/raymath.h"

#include <Recast.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <DetourNavMeshQuery.h>
#include <DetourCrowd.h>
#include <DetourTileCache.h>
#include <DetourTileCacheBuilder.h>

#include <vector>
#include <cstring>

namespace RecastBindings
{
    static const int RC_MAX_POLYS       = 512;
    static const int RC_MAX_SMOOTH      = 2048;

    // ── Handle types ────────────────────────────────────────
    struct NavMeshHandle
    {
        dtNavMesh      *navMesh  = nullptr;
        dtNavMeshQuery *navQuery = nullptr;
        bool            valid    = false;
        int             refCount = 1; // NavMesh instance + borrowed refs from NavCrowd

        void destroy()
        {
            if (navQuery) { dtFreeNavMeshQuery(navQuery); navQuery = nullptr; }
            if (navMesh)  { dtFreeNavMesh(navMesh);  navMesh  = nullptr; }
            valid = false;
        }

        void retain() { refCount++; }

        void release()
        {
            refCount--;
            if (refCount <= 0)
            {
                destroy();
                delete this;
            }
        }
    };

    struct NavCrowdHandle
    {
        dtCrowd       *crowd = nullptr;
        NavMeshHandle *mesh  = nullptr; // shared via NavMeshHandle::retain/release
        bool           valid = false;

        void destroy()
        {
            if (crowd) { dtFreeCrowd(crowd); crowd = nullptr; }
            if (mesh)  { mesh->release(); mesh = nullptr; }
            valid  = false;
        }
    };

    struct NavTileCacheHandle
    {
        dtTileCache          *tileCache  = nullptr;
        dtNavMesh            *navMesh    = nullptr;
        dtNavMeshQuery       *navQuery   = nullptr;
        dtTileCacheAlloc     *alloc      = nullptr;
        dtTileCacheCompressor *compressor = nullptr;
        dtTileCacheMeshProcess *meshProc = nullptr;
        bool                  valid      = false;

        void destroy()
        {
            if (navQuery)   { dtFreeNavMeshQuery(navQuery); navQuery = nullptr; }
            if (navMesh)    { dtFreeNavMesh(navMesh); navMesh = nullptr; }
            if (tileCache)  { dtFreeTileCache(tileCache); tileCache = nullptr; }
            if (meshProc)   { delete meshProc; meshProc = nullptr; }
            if (compressor) { delete compressor; compressor = nullptr; }
            if (alloc)      { delete alloc; alloc = nullptr; }
            valid = false;
        }
    };

    // ── Globals (set during registerAll) ────────────────────
    extern NativeClassDef  *g_navMeshClass;
    extern NativeClassDef  *g_navCrowdClass;
    extern NativeClassDef  *g_navTileCacheClass;
    extern NativeStructDef *g_vector3Def;

    // ── Helpers ─────────────────────────────────────────────
    int             push_nil1(Interpreter *vm);
    NativeStructDef *get_native_struct_def(Interpreter *vm, const char *name);
    bool            read_number_arg(const Value &v, double *out, const char *fn, int idx);
    bool            read_vector3_arg(const Value &v, Vector3 *out, const char *fn, int idx);
    bool            push_vector3(Interpreter *vm, const Vector3 &v);

    NavMeshHandle  *require_navmesh(void *instance, const char *fn);
    NavCrowdHandle *require_crowd(void *instance, const char *fn);
    NavTileCacheHandle *require_tilecache(void *instance, const char *fn);

    // ── Sub-registrations ───────────────────────────────────
    void register_navmesh(Interpreter &vm);
    void register_crowd(Interpreter &vm);
    void register_tilecache(Interpreter &vm);

    // ── Internal build helper ────────────────────────────────
    struct RecastConfig
    {
        float cellSize         = 0.30f;
        float cellHeight       = 0.20f;
        float agentHeight      = 2.00f;
        float agentRadius      = 0.60f;
        float agentMaxClimb    = 0.90f;
        float agentMaxSlope    = 45.0f;
        int   regionMinSize    = 8;
        int   regionMergeSize  = 20;
        float edgeMaxLen       = 12.0f;
        float edgeMaxError     = 1.30f;
        int   vertsPerPoly     = 6;
        float detailSampleDist = 6.00f;
        float detailSampleMaxError = 1.00f;
    };

    bool build_navmesh_internal(
        const float *verts, int nVerts,
        const int   *tris,  int nTris,
        const RecastConfig &cfg,
        NavMeshHandle *out);

} // namespace RecastBindings
