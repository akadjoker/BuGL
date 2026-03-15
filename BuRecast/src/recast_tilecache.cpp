#include "recast_core.hpp"
#include <cmath>

namespace RecastBindings
{
    class PassthroughTileCompressor final : public dtTileCacheCompressor
    {
    public:
        int maxCompressedSize(const int bufferSize) override
        {
            return bufferSize;
        }

        dtStatus compress(const unsigned char *buffer, const int bufferSize,
                          unsigned char *compressed, const int maxCompressedSize,
                          int *compressedSize) override
        {
            if (!buffer || !compressed || !compressedSize || maxCompressedSize < bufferSize)
                return DT_FAILURE;

            memcpy(compressed, buffer, (size_t)bufferSize);
            *compressedSize = bufferSize;
            return DT_SUCCESS;
        }

        dtStatus decompress(const unsigned char *compressed, const int compressedSize,
                            unsigned char *buffer, const int maxBufferSize,
                            int *bufferSize) override
        {
            if (!compressed || !buffer || !bufferSize || maxBufferSize < compressedSize)
                return DT_FAILURE;

            memcpy(buffer, compressed, (size_t)compressedSize);
            *bufferSize = compressedSize;
            return DT_SUCCESS;
        }
    };

    class DefaultTileMeshProcess final : public dtTileCacheMeshProcess
    {
    public:
        void process(dtNavMeshCreateParams *params, unsigned char *polyAreas, unsigned short *polyFlags) override
        {
            if (!params)
                return;

            for (int i = 0; i < params->polyCount; ++i)
            {
                if (polyAreas && polyAreas[i] == DT_TILECACHE_WALKABLE_AREA)
                    polyAreas[i] = 0;
                if (polyFlags)
                    polyFlags[i] = 1;
            }
        }
    };

    static bool tilecache_init_internal(
        NavTileCacheHandle *out,
        const Vector3 &origin,
        int tileSize,
        float cellSize,
        float cellHeight,
        float walkableHeight,
        float walkableRadius,
        float walkableClimb,
        int maxTiles,
        int maxObstacles,
        float maxSimplificationError)
    {
        if (!out)
            return false;

        out->destroy();

        if (tileSize < 8)
            tileSize = 8;
        if (tileSize > 255)
            tileSize = 255;
        if (cellSize <= 0.0001f)
            cellSize = 0.30f;
        if (cellHeight <= 0.0001f)
            cellHeight = 0.20f;
        if (walkableHeight <= 0.0001f)
            walkableHeight = 2.0f;
        if (walkableRadius < 0.0f)
            walkableRadius = 0.0f;
        if (walkableClimb < 0.0f)
            walkableClimb = 0.0f;
        if (maxTiles < 1)
            maxTiles = 1;
        if (maxObstacles < 1)
            maxObstacles = 1;
        if (maxSimplificationError <= 0.0f)
            maxSimplificationError = 1.3f;

        out->alloc = new dtTileCacheAlloc();
        out->compressor = new PassthroughTileCompressor();
        out->meshProc = new DefaultTileMeshProcess();
        out->tileCache = dtAllocTileCache();
        out->navMesh = dtAllocNavMesh();
        out->navQuery = dtAllocNavMeshQuery();

        if (!out->alloc || !out->compressor || !out->meshProc || !out->tileCache || !out->navMesh || !out->navQuery)
        {
            out->destroy();
            return false;
        }

        dtTileCacheParams tcparams;
        memset(&tcparams, 0, sizeof(tcparams));
        tcparams.orig[0] = origin.x;
        tcparams.orig[1] = origin.y;
        tcparams.orig[2] = origin.z;
        tcparams.cs = cellSize;
        tcparams.ch = cellHeight;
        tcparams.width = tileSize;
        tcparams.height = tileSize;
        tcparams.walkableHeight = walkableHeight;
        tcparams.walkableRadius = walkableRadius;
        tcparams.walkableClimb = walkableClimb;
        tcparams.maxSimplificationError = maxSimplificationError;
        tcparams.maxTiles = maxTiles;
        tcparams.maxObstacles = maxObstacles;

        if (dtStatusFailed(out->tileCache->init(&tcparams, out->alloc, out->compressor, out->meshProc)))
        {
            out->destroy();
            return false;
        }

        dtNavMeshParams navParams;
        memset(&navParams, 0, sizeof(navParams));
        rcVcopy(navParams.orig, tcparams.orig);
        navParams.tileWidth = tcparams.width * tcparams.cs;
        navParams.tileHeight = tcparams.height * tcparams.cs;
        navParams.maxTiles = tcparams.maxTiles;
        navParams.maxPolys = 32768;

        if (dtStatusFailed(out->navMesh->init(&navParams)))
        {
            out->destroy();
            return false;
        }

        if (dtStatusFailed(out->navQuery->init(out->navMesh, 2048)))
        {
            out->destroy();
            return false;
        }

        out->valid = true;
        return true;
    }

    static int calc_height_cell(const dtTileCacheParams *params, float elevation)
    {
        if (!params || params->ch <= 0.0001f)
            return 0;

        int cell = (int)floorf(elevation / params->ch);
        if (cell < 0)
            cell = 0;
        if (cell > 255)
            cell = 255;
        return cell;
    }

    static int build_flat_tile(Interpreter *vm, NavTileCacheHandle *h, int tx, int ty, float elevation)
    {
        const dtTileCacheParams *params = h->tileCache->getParams();
        if (!params)
            return push_nil1(vm);

        const int width = params->width;
        const int height = params->height;
        if (width < 1 || width > 255 || height < 1 || height > 255)
            return push_nil1(vm);

        const int cellCount = width * height;
        std::vector<unsigned char> heights((size_t)cellCount, 0u);
        std::vector<unsigned char> areas((size_t)cellCount, DT_TILECACHE_WALKABLE_AREA);
        std::vector<unsigned char> cons((size_t)cellCount, 0u);

        const int elevCell = calc_height_cell(params, elevation);
        for (int i = 0; i < cellCount; ++i)
            heights[(size_t)i] = (unsigned char)elevCell;

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                unsigned char con = 0;
                if (x > 0)
                    con |= (1u << 0); // left
                if (y < height - 1)
                    con |= (1u << 1); // up
                if (x < width - 1)
                    con |= (1u << 2); // right
                if (y > 0)
                    con |= (1u << 3); // down
                cons[(size_t)(y * width + x)] = con;
            }
        }

        dtTileCacheLayerHeader header;
        memset(&header, 0, sizeof(header));
        header.magic = DT_TILECACHE_MAGIC;
        header.version = DT_TILECACHE_VERSION;
        header.tx = tx;
        header.ty = ty;
        header.tlayer = 0;
        header.width = (unsigned char)width;
        header.height = (unsigned char)height;
        header.minx = 0;
        header.maxx = (unsigned char)(width - 1);
        header.miny = 0;
        header.maxy = (unsigned char)(height - 1);
        header.hmin = (unsigned short)elevCell;
        header.hmax = (unsigned short)elevCell;

        const float tileWorldW = params->width * params->cs;
        const float tileWorldH = params->height * params->cs;
        header.bmin[0] = params->orig[0] + tx * tileWorldW;
        header.bmin[1] = params->orig[1] + elevation;
        header.bmin[2] = params->orig[2] + ty * tileWorldH;
        header.bmax[0] = header.bmin[0] + tileWorldW;
        header.bmax[1] = header.bmin[1] + params->walkableHeight + params->ch * 2.0f;
        header.bmax[2] = header.bmin[2] + tileWorldH;

        unsigned char *tileData = nullptr;
        int tileDataSize = 0;
        dtStatus status = dtBuildTileCacheLayer(
            h->compressor,
            &header,
            heights.data(),
            areas.data(),
            cons.data(),
            &tileData,
            &tileDataSize);

        if (dtStatusFailed(status) || !tileData || tileDataSize <= 0)
            return push_nil1(vm);

        dtCompressedTileRef tileRef = 0;
        status = h->tileCache->addTile(tileData, tileDataSize, DT_COMPRESSEDTILE_FREE_DATA, &tileRef);
        if (dtStatusFailed(status) || tileRef == 0)
        {
            dtFree(tileData);
            return push_nil1(vm);
        }

        status = h->tileCache->buildNavMeshTile(tileRef, h->navMesh);
        if (dtStatusFailed(status))
            return push_nil1(vm);

        vm->push(vm->makeDouble((double)tileRef));
        return 1;
    }

    static int tilecache_find_nearest(Interpreter *vm, NavTileCacheHandle *h, const Vector3 &pos, int argCount, Value *args, int extStartArg)
    {
        float ext[3] = {2.0f, 4.0f, 2.0f};
        if (argCount > extStartArg + 0 && args[extStartArg + 0].isNumber()) ext[0] = (float)args[extStartArg + 0].asDouble();
        if (argCount > extStartArg + 1 && args[extStartArg + 1].isNumber()) ext[1] = (float)args[extStartArg + 1].asDouble();
        if (argCount > extStartArg + 2 && args[extStartArg + 2].isNumber()) ext[2] = (float)args[extStartArg + 2].asDouble();

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

    static void *tilecache_ctor(Interpreter *vm, int argCount, Value *args)
    {
        (void)args;
        NavTileCacheHandle *h = new NavTileCacheHandle();

        Vector3 origin = {0.0f, 0.0f, 0.0f};
        int tileSize = 48;
        float cellSize = 0.30f;
        float cellHeight = 0.20f;
        float walkableHeight = 2.00f;
        float walkableRadius = 0.60f;
        float walkableClimb = 0.90f;
        int maxTiles = 64;
        int maxObstacles = 128;
        float maxSimplificationError = 1.3f;

        if (argCount > 0 && args[0].isNumber()) tileSize = (int)args[0].asDouble();
        if (argCount > 1 && args[1].isNumber()) cellSize = (float)args[1].asDouble();
        if (argCount > 2 && args[2].isNumber()) cellHeight = (float)args[2].asDouble();
        if (argCount > 3 && args[3].isNumber()) walkableHeight = (float)args[3].asDouble();
        if (argCount > 4 && args[4].isNumber()) walkableRadius = (float)args[4].asDouble();
        if (argCount > 5 && args[5].isNumber()) walkableClimb = (float)args[5].asDouble();
        if (argCount > 6 && args[6].isNumber()) maxTiles = (int)args[6].asDouble();
        if (argCount > 7 && args[7].isNumber()) maxObstacles = (int)args[7].asDouble();
        if (argCount > 8 && args[8].isNumber()) origin.x = (float)args[8].asDouble();
        if (argCount > 9 && args[9].isNumber()) origin.y = (float)args[9].asDouble();
        if (argCount > 10 && args[10].isNumber()) origin.z = (float)args[10].asDouble();
        if (argCount > 11 && args[11].isNumber()) maxSimplificationError = (float)args[11].asDouble();

        if (!tilecache_init_internal(h, origin, tileSize, cellSize, cellHeight,
                                     walkableHeight, walkableRadius, walkableClimb,
                                     maxTiles, maxObstacles, maxSimplificationError))
        {
            delete h;
            Error("NavTileCache(): init failed");
            return nullptr;
        }

        return h;
    }

    static void tilecache_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        NavTileCacheHandle *h = (NavTileCacheHandle *)instance;
        if (!h)
            return;

        h->destroy();
        delete h;
    }

    static int tilecache_is_valid(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("NavTileCache.isValid() expects 0 arguments");
            return push_nil1(vm);
        }

        NavTileCacheHandle *h = (NavTileCacheHandle *)instance;
        vm->pushBool(h && h->valid && h->tileCache && h->navMesh && h->navQuery);
        return 1;
    }

    static int tilecache_destroy(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)vm;
        (void)args;
        if (argCount != 0)
        {
            Error("NavTileCache.destroy() expects 0 arguments");
            return 0;
        }

        NavTileCacheHandle *h = (NavTileCacheHandle *)instance;
        if (h)
            h->destroy();
        return 0;
    }

    static int tilecache_init(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        NavTileCacheHandle *h = (NavTileCacheHandle *)instance;
        if (!h)
            return push_nil1(vm);

        Vector3 origin = {0.0f, 0.0f, 0.0f};
        int tileSize = 48;
        float cellSize = 0.30f;
        float cellHeight = 0.20f;
        float walkableHeight = 2.00f;
        float walkableRadius = 0.60f;
        float walkableClimb = 0.90f;
        int maxTiles = 64;
        int maxObstacles = 128;
        float maxSimplificationError = 1.3f;

        if (argCount > 0 && args[0].isNumber()) tileSize = (int)args[0].asDouble();
        if (argCount > 1 && args[1].isNumber()) cellSize = (float)args[1].asDouble();
        if (argCount > 2 && args[2].isNumber()) cellHeight = (float)args[2].asDouble();
        if (argCount > 3 && args[3].isNumber()) walkableHeight = (float)args[3].asDouble();
        if (argCount > 4 && args[4].isNumber()) walkableRadius = (float)args[4].asDouble();
        if (argCount > 5 && args[5].isNumber()) walkableClimb = (float)args[5].asDouble();
        if (argCount > 6 && args[6].isNumber()) maxTiles = (int)args[6].asDouble();
        if (argCount > 7 && args[7].isNumber()) maxObstacles = (int)args[7].asDouble();
        if (argCount > 8 && args[8].isNumber()) origin.x = (float)args[8].asDouble();
        if (argCount > 9 && args[9].isNumber()) origin.y = (float)args[9].asDouble();
        if (argCount > 10 && args[10].isNumber()) origin.z = (float)args[10].asDouble();
        if (argCount > 11 && args[11].isNumber()) maxSimplificationError = (float)args[11].asDouble();

        bool ok = tilecache_init_internal(h, origin, tileSize, cellSize, cellHeight,
                                          walkableHeight, walkableRadius, walkableClimb,
                                          maxTiles, maxObstacles, maxSimplificationError);
        vm->pushBool(ok);
        return 1;
    }

    static int tilecache_build_flat_tile(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 2 || argCount > 3)
        {
            Error("NavTileCache.buildFlatTile(tx, ty [, elevation])");
            return push_nil1(vm);
        }

        NavTileCacheHandle *h = require_tilecache(instance, "NavTileCache.buildFlatTile()");
        if (!h)
            return push_nil1(vm);

        double txd = 0.0;
        double tyd = 0.0;
        if (!read_number_arg(args[0], &txd, "NavTileCache.buildFlatTile()", 1) ||
            !read_number_arg(args[1], &tyd, "NavTileCache.buildFlatTile()", 2))
        {
            return push_nil1(vm);
        }

        float elevation = 0.0f;
        if (argCount > 2 && args[2].isNumber())
            elevation = (float)args[2].asDouble();

        return build_flat_tile(vm, h, (int)txd, (int)tyd, elevation);
    }

    static int tilecache_add_cylinder_obstacle(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 3)
        {
            Error("NavTileCache.addCylinderObstacle(pos, radius, height)");
            return push_nil1(vm);
        }

        NavTileCacheHandle *h = require_tilecache(instance, "NavTileCache.addCylinderObstacle()");
        if (!h)
            return push_nil1(vm);

        Vector3 pos;
        double radius = 0.0;
        double height = 0.0;
        if (!read_vector3_arg(args[0], &pos, "NavTileCache.addCylinderObstacle()", 1) ||
            !read_number_arg(args[1], &radius, "NavTileCache.addCylinderObstacle()", 2) ||
            !read_number_arg(args[2], &height, "NavTileCache.addCylinderObstacle()", 3))
        {
            return push_nil1(vm);
        }

        dtObstacleRef ref = 0;
        const float p[3] = {pos.x, pos.y, pos.z};
        dtStatus status = h->tileCache->addObstacle(p, (float)radius, (float)height, &ref);
        if (dtStatusFailed(status) || !ref)
            return push_nil1(vm);

        vm->push(vm->makeDouble((double)ref));
        return 1;
    }

    static int tilecache_add_box_obstacle(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 2)
        {
            Error("NavTileCache.addBoxObstacle(bmin, bmax)");
            return push_nil1(vm);
        }

        NavTileCacheHandle *h = require_tilecache(instance, "NavTileCache.addBoxObstacle()");
        if (!h)
            return push_nil1(vm);

        Vector3 bmin;
        Vector3 bmax;
        if (!read_vector3_arg(args[0], &bmin, "NavTileCache.addBoxObstacle()", 1) ||
            !read_vector3_arg(args[1], &bmax, "NavTileCache.addBoxObstacle()", 2))
        {
            return push_nil1(vm);
        }

        dtObstacleRef ref = 0;
        const float vmin[3] = {bmin.x, bmin.y, bmin.z};
        const float vmax[3] = {bmax.x, bmax.y, bmax.z};
        dtStatus status = h->tileCache->addBoxObstacle(vmin, vmax, &ref);
        if (dtStatusFailed(status) || !ref)
            return push_nil1(vm);

        vm->push(vm->makeDouble((double)ref));
        return 1;
    }

    static int tilecache_add_oriented_box_obstacle(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 3)
        {
            Error("NavTileCache.addOrientedBoxObstacle(center, halfExtents, yRadians)");
            return push_nil1(vm);
        }

        NavTileCacheHandle *h = require_tilecache(instance, "NavTileCache.addOrientedBoxObstacle()");
        if (!h)
            return push_nil1(vm);

        Vector3 center;
        Vector3 halfExtents;
        double yRadians = 0.0;
        if (!read_vector3_arg(args[0], &center, "NavTileCache.addOrientedBoxObstacle()", 1) ||
            !read_vector3_arg(args[1], &halfExtents, "NavTileCache.addOrientedBoxObstacle()", 2) ||
            !read_number_arg(args[2], &yRadians, "NavTileCache.addOrientedBoxObstacle()", 3))
        {
            return push_nil1(vm);
        }

        dtObstacleRef ref = 0;
        const float c[3] = {center.x, center.y, center.z};
        const float e[3] = {halfExtents.x, halfExtents.y, halfExtents.z};
        dtStatus status = h->tileCache->addBoxObstacle(c, e, (float)yRadians, &ref);
        if (dtStatusFailed(status) || !ref)
            return push_nil1(vm);

        vm->push(vm->makeDouble((double)ref));
        return 1;
    }

    static int tilecache_remove_obstacle(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("NavTileCache.removeObstacle(ref)");
            return push_nil1(vm);
        }

        NavTileCacheHandle *h = require_tilecache(instance, "NavTileCache.removeObstacle()");
        if (!h)
            return push_nil1(vm);

        double refd = 0.0;
        if (!read_number_arg(args[0], &refd, "NavTileCache.removeObstacle()", 1))
            return push_nil1(vm);

        dtStatus status = h->tileCache->removeObstacle((dtObstacleRef)refd);
        vm->pushBool(dtStatusSucceed(status));
        return 1;
    }

    static int tilecache_get_obstacle_state(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1)
        {
            Error("NavTileCache.getObstacleState(ref)");
            return push_nil1(vm);
        }

        NavTileCacheHandle *h = require_tilecache(instance, "NavTileCache.getObstacleState()");
        if (!h)
            return push_nil1(vm);

        double refd = 0.0;
        if (!read_number_arg(args[0], &refd, "NavTileCache.getObstacleState()", 1))
            return push_nil1(vm);

        const dtTileCacheObstacle *ob = h->tileCache->getObstacleByRef((dtObstacleRef)refd);
        if (!ob)
            return push_nil1(vm);

        vm->push(vm->makeInt((int)ob->state));
        return 1;
    }

    static int tilecache_update(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        float dt = 0.0f;
        if (argCount > 0)
        {
            double dtd = 0.0;
            if (!read_number_arg(args[0], &dtd, "NavTileCache.update()", 1))
                return push_nil1(vm);
            dt = (float)dtd;
        }

        NavTileCacheHandle *h = require_tilecache(instance, "NavTileCache.update()");
        if (!h)
            return push_nil1(vm);

        bool upToDate = false;
        dtStatus status = h->tileCache->update(dt, h->navMesh, &upToDate);
        if (dtStatusFailed(status))
        {
            vm->pushBool(false);
            return 1;
        }

        vm->pushBool(upToDate);
        return 1;
    }

    static int tilecache_find_path(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 2)
        {
            Error("NavTileCache.findPath(start, end [, extX, extY, extZ])");
            return push_nil1(vm);
        }

        NavTileCacheHandle *h = require_tilecache(instance, "NavTileCache.findPath()");
        if (!h)
            return push_nil1(vm);

        Vector3 startPt, endPt;
        if (!read_vector3_arg(args[0], &startPt, "NavTileCache.findPath()", 1) ||
            !read_vector3_arg(args[1], &endPt, "NavTileCache.findPath()", 2))
        {
            return push_nil1(vm);
        }

        float ext[3] = {2.0f, 4.0f, 2.0f};
        if (argCount > 2 && args[2].isNumber()) ext[0] = (float)args[2].asDouble();
        if (argCount > 3 && args[3].isNumber()) ext[1] = (float)args[3].asDouble();
        if (argCount > 4 && args[4].isNumber()) ext[2] = (float)args[4].asDouble();

        dtQueryFilter filter;
        filter.setIncludeFlags(0xffff);
        filter.setExcludeFlags(0);

        const float sp[3] = {startPt.x, startPt.y, startPt.z};
        const float ep[3] = {endPt.x, endPt.y, endPt.z};

        dtPolyRef startRef = 0, endRef = 0;
        float nearestPt[3];
        h->navQuery->findNearestPoly(sp, ext, &filter, &startRef, nearestPt);
        h->navQuery->findNearestPoly(ep, ext, &filter, &endRef, nearestPt);
        if (!startRef || !endRef)
            return push_nil1(vm);

        dtPolyRef polys[RC_MAX_POLYS];
        int nPolys = 0;
        h->navQuery->findPath(startRef, endRef, sp, ep, &filter, polys, &nPolys, RC_MAX_POLYS);
        if (nPolys == 0)
            return push_nil1(vm);

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

        Value arrVal = vm->makeArray();
        ArrayInstance *arr = arrVal.as.array;

        for (int i = 0; i < nStraight; ++i)
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

    static int tilecache_find_nearest_point(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 1)
        {
            Error("NavTileCache.findNearestPoint(pos [, extX, extY, extZ])");
            return push_nil1(vm);
        }

        NavTileCacheHandle *h = require_tilecache(instance, "NavTileCache.findNearestPoint()");
        if (!h)
            return push_nil1(vm);

        Vector3 pos;
        if (!read_vector3_arg(args[0], &pos, "NavTileCache.findNearestPoint()", 1))
            return push_nil1(vm);

        return tilecache_find_nearest(vm, h, pos, argCount, args, 1);
    }

    static int tilecache_get_stats(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)args;
        if (argCount != 0)
        {
            Error("NavTileCache.getStats() expects 0 arguments");
            return push_nil1(vm);
        }

        NavTileCacheHandle *h = require_tilecache(instance, "NavTileCache.getStats()");
        if (!h)
            return push_nil1(vm);

        const dtTileCacheParams *params = h->tileCache->getParams();
        if (!params)
            return push_nil1(vm);

        Value arrVal = vm->makeArray();
        ArrayInstance *arr = arrVal.as.array;
        arr->values.push(vm->makeInt(params->width));
        arr->values.push(vm->makeInt(params->height));
        arr->values.push(vm->makeInt(params->maxTiles));
        arr->values.push(vm->makeInt(params->maxObstacles));
        vm->push(arrVal);
        return 1;
    }

    void register_tilecache(Interpreter &vm)
    {
        g_navTileCacheClass = vm.registerNativeClass("NavTileCache", tilecache_ctor, tilecache_dtor, -1, false);

        vm.addNativeMethod(g_navTileCacheClass, "isValid", tilecache_is_valid);
        vm.addNativeMethod(g_navTileCacheClass, "destroy", tilecache_destroy);
        vm.addNativeMethod(g_navTileCacheClass, "init", tilecache_init);
        vm.addNativeMethod(g_navTileCacheClass, "buildFlatTile", tilecache_build_flat_tile);
        vm.addNativeMethod(g_navTileCacheClass, "addCylinderObstacle", tilecache_add_cylinder_obstacle);
        vm.addNativeMethod(g_navTileCacheClass, "addBoxObstacle", tilecache_add_box_obstacle);
        vm.addNativeMethod(g_navTileCacheClass, "addOrientedBoxObstacle", tilecache_add_oriented_box_obstacle);
        vm.addNativeMethod(g_navTileCacheClass, "removeObstacle", tilecache_remove_obstacle);
        vm.addNativeMethod(g_navTileCacheClass, "getObstacleState", tilecache_get_obstacle_state);
        vm.addNativeMethod(g_navTileCacheClass, "update", tilecache_update);
        vm.addNativeMethod(g_navTileCacheClass, "findPath", tilecache_find_path);
        vm.addNativeMethod(g_navTileCacheClass, "findNearestPoint", tilecache_find_nearest_point);
        vm.addNativeMethod(g_navTileCacheClass, "getStats", tilecache_get_stats);
    }

} // namespace RecastBindings
