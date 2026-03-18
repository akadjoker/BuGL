#include "bindings.hpp"
#include "meshoptimizer.h"
#include <vector>
#include <cstring>
#include <limits>

namespace MeshOptBindings
{
    // =====================================================
    // HELPER: Extract index buffer from a Buffer(UINT32) or Array
    // =====================================================
    static bool extractIndices(Interpreter *vm, const Value &val, std::vector<unsigned int> &out, const char *fnName)
    {
        if (val.isBuffer())
        {
            BufferInstance *buf = val.asBuffer();
            if (!buf || !buf->data)
            {
                Error("%s: invalid buffer", fnName);
                return false;
            }
            if (buf->type != BufferType::UINT32 && buf->type != BufferType::INT32)
            {
                Error("%s: index buffer must be UINT32 or INT32", fnName);
                return false;
            }
            out.resize((size_t)buf->count);
            std::memcpy(out.data(), buf->data, (size_t)buf->count * sizeof(unsigned int));
            return true;
        }

        if (val.isArray())
        {
            ArrayInstance *arr = val.asArray();
            if (!arr)
            {
                Error("%s: invalid array", fnName);
                return false;
            }
            out.resize(arr->values.size());
            for (size_t i = 0; i < arr->values.size(); i++)
            {
                if (!arr->values[i].isNumber())
                {
                    Error("%s: array must contain only numbers", fnName);
                    return false;
                }
                out[i] = (unsigned int)arr->values[i].asInt();
            }
            return true;
        }

        Error("%s: expected buffer or array for indices", fnName);
        return false;
    }

    static Value makeUint32Buffer(Interpreter *vm, const unsigned int *data, size_t count)
    {
        Value v = vm->makeBuffer((int)count, (int)BufferType::UINT32);
        BufferInstance *buf = v.asBuffer();
        if (buf && buf->data && data)
            std::memcpy(buf->data, data, count * sizeof(unsigned int));
        return v;
    }

    // =====================================================
    // meshopt_generateVertexRemap(indices, indexCount, vertexCount)
    // Returns: [uniqueVertices, remapTable(Buffer UINT32)]
    // =====================================================
    static int native_meshopt_generateVertexRemap(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("meshopt_generateVertexRemap expects (indices, indexCount, vertexCount)");
            return 0;
        }

        std::vector<unsigned int> indices;
        const unsigned int *indexPtr = nullptr;
        size_t indexCount = (size_t)args[1].asNumber();
        size_t vertexCount = (size_t)args[2].asNumber();

        if (!args[0].isNil())
        {
            if (!extractIndices(vm, args[0], indices, "meshopt_generateVertexRemap"))
                return 0;
            indexPtr = indices.data();
        }

        std::vector<unsigned int> remap(vertexCount);
        size_t uniqueVertices = meshopt_generateVertexRemap(
            remap.data(), indexPtr, indexCount, nullptr, vertexCount, 0);

        // Return [uniqueVertices, remapBuffer]
        Value result = vm->makeArray();
        ArrayInstance *arr = result.asArray();
        arr->values.push(vm->makeInt((int)uniqueVertices));
        arr->values.push(makeUint32Buffer(vm, remap.data(), remap.size()));
        vm->push(result);
        return 1;
    }

    // =====================================================
    // meshopt_optimizeVertexCache(indices, indexCount, vertexCount)
    // Returns: optimized index buffer (Buffer UINT32)
    // =====================================================
    static int native_meshopt_optimizeVertexCache(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("meshopt_optimizeVertexCache expects (indices, indexCount, vertexCount)");
            return 0;
        }

        std::vector<unsigned int> indices;
        if (!extractIndices(vm, args[0], indices, "meshopt_optimizeVertexCache"))
            return 0;

        size_t indexCount = (size_t)args[1].asNumber();
        size_t vertexCount = (size_t)args[2].asNumber();

        if (indexCount > indices.size())
            indexCount = indices.size();

        std::vector<unsigned int> optimized(indexCount);
        meshopt_optimizeVertexCache(optimized.data(), indices.data(), indexCount, vertexCount);

        vm->push(makeUint32Buffer(vm, optimized.data(), optimized.size()));
        return 1;
    }

    // =====================================================
    // meshopt_optimizeVertexCacheStrip(indices, indexCount, vertexCount)
    // Returns: optimized index buffer (Buffer UINT32) 
    // =====================================================
    static int native_meshopt_optimizeVertexCacheStrip(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("meshopt_optimizeVertexCacheStrip expects (indices, indexCount, vertexCount)");
            return 0;
        }

        std::vector<unsigned int> indices;
        if (!extractIndices(vm, args[0], indices, "meshopt_optimizeVertexCacheStrip"))
            return 0;

        size_t indexCount = (size_t)args[1].asNumber();
        size_t vertexCount = (size_t)args[2].asNumber();

        if (indexCount > indices.size())
            indexCount = indices.size();

        std::vector<unsigned int> optimized(indexCount);
        meshopt_optimizeVertexCacheStrip(optimized.data(), indices.data(), indexCount, vertexCount);

        vm->push(makeUint32Buffer(vm, optimized.data(), optimized.size()));
        return 1;
    }

    // =====================================================
    // meshopt_optimizeOverdraw(indices, indexCount, vertexPositions, vertexCount, vertexStride, threshold)
    // Returns: optimized index buffer (Buffer UINT32)
    // =====================================================
    static int native_meshopt_optimizeOverdraw(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 6)
        {
            Error("meshopt_optimizeOverdraw expects (indices, indexCount, vertexPositions, vertexCount, vertexPositionsStride, threshold)");
            return 0;
        }

        std::vector<unsigned int> indices;
        if (!extractIndices(vm, args[0], indices, "meshopt_optimizeOverdraw"))
            return 0;

        size_t indexCount = (size_t)args[1].asNumber();
        size_t vertexCount = (size_t)args[3].asNumber();
        size_t vertexPositionsStride = (size_t)args[4].asNumber();
        float threshold = (float)args[5].asNumber();

        if (indexCount > indices.size())
            indexCount = indices.size();

        // vertexPositions must be a Buffer(FLOAT)
        if (!args[2].isBuffer())
        {
            Error("meshopt_optimizeOverdraw: vertexPositions must be a buffer");
            return 0;
        }
        BufferInstance *posBuf = args[2].asBuffer();
        if (!posBuf || !posBuf->data)
        {
            Error("meshopt_optimizeOverdraw: invalid vertex positions buffer");
            return 0;
        }

        std::vector<unsigned int> optimized(indexCount);
        meshopt_optimizeOverdraw(optimized.data(), indices.data(), indexCount,
                                 (const float *)posBuf->data, vertexCount,
                                 vertexPositionsStride, threshold);

        vm->push(makeUint32Buffer(vm, optimized.data(), optimized.size()));
        return 1;
    }

    // =====================================================
    // meshopt_optimizeVertexFetch(indices, indexCount, vertices, vertexCount, vertexSize)
    // Returns: [optimizedVerticesBuffer, optimizedIndicesBuffer]
    // =====================================================
    static int native_meshopt_optimizeVertexFetch(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5)
        {
            Error("meshopt_optimizeVertexFetch expects (indices, indexCount, vertices, vertexCount, vertexSize)");
            return 0;
        }

        std::vector<unsigned int> indices;
        if (!extractIndices(vm, args[0], indices, "meshopt_optimizeVertexFetch"))
            return 0;

        size_t indexCount = (size_t)args[1].asNumber();
        size_t vertexCount = (size_t)args[3].asNumber();
        size_t vertexSize = (size_t)args[4].asNumber();

        if (indexCount > indices.size())
            indexCount = indices.size();

        if (!args[2].isBuffer())
        {
            Error("meshopt_optimizeVertexFetch: vertices must be a buffer");
            return 0;
        }
        BufferInstance *vertBuf = args[2].asBuffer();
        if (!vertBuf || !vertBuf->data)
        {
            Error("meshopt_optimizeVertexFetch: invalid vertex buffer");
            return 0;
        }

        // Output vertex buffer (same size as input)
        size_t totalVertexBytes = vertexCount * vertexSize;
        std::vector<unsigned char> optimizedVertices(totalVertexBytes);

        size_t uniqueVertexCount = meshopt_optimizeVertexFetch(
            optimizedVertices.data(), indices.data(), indexCount,
            vertBuf->data, vertexCount, vertexSize);

        // Return [newVertexBuffer, newIndexBuffer, uniqueVertexCount]
        Value result = vm->makeArray();
        ArrayInstance *arr = result.asArray();

        // Create a UINT8 buffer with the optimized vertex data
        Value vbuf = vm->makeBuffer((int)totalVertexBytes, (int)BufferType::UINT8);
        BufferInstance *outVBuf = vbuf.asBuffer();
        if (outVBuf && outVBuf->data)
            std::memcpy(outVBuf->data, optimizedVertices.data(), totalVertexBytes);

        arr->values.push(vbuf);
        arr->values.push(makeUint32Buffer(vm, indices.data(), indexCount));
        arr->values.push(vm->makeInt((int)uniqueVertexCount));

        vm->push(result);
        return 1;
    }

    // =====================================================
    // meshopt_simplify(indices, indexCount, vertexPositions, vertexCount, vertexPositionsStride, targetIndexCount, targetError)
    // Returns: [simplifiedIndices (Buffer UINT32), resultError (float)]
    // =====================================================
    static int native_meshopt_simplify(Interpreter *vm, int argc, Value *args)
    {
        if (argc < 7 || argc > 8)
        {
            Error("meshopt_simplify expects (indices, indexCount, vertexPositions, vertexCount, vertexPositionsStride, targetIndexCount, targetError [, options])");
            return 0;
        }

        std::vector<unsigned int> indices;
        if (!extractIndices(vm, args[0], indices, "meshopt_simplify"))
            return 0;

        size_t indexCount = (size_t)args[1].asNumber();
        size_t vertexCount = (size_t)args[3].asNumber();
        size_t vertexPositionsStride = (size_t)args[4].asNumber();
        size_t targetIndexCount = (size_t)args[5].asNumber();
        float targetError = (float)args[6].asNumber();
        unsigned int options = (argc >= 8) ? (unsigned int)args[7].asNumber() : 0;

        if (indexCount > indices.size())
            indexCount = indices.size();

        if (!args[2].isBuffer())
        {
            Error("meshopt_simplify: vertexPositions must be a buffer");
            return 0;
        }
        BufferInstance *posBuf = args[2].asBuffer();
        if (!posBuf || !posBuf->data)
        {
            Error("meshopt_simplify: invalid vertex positions buffer");
            return 0;
        }

        std::vector<unsigned int> simplified(indexCount);
        float resultError = 0.0f;

        size_t resultCount = meshopt_simplify(
            simplified.data(), indices.data(), indexCount,
            (const float *)posBuf->data, vertexCount, vertexPositionsStride,
            targetIndexCount, targetError, options, &resultError);

        simplified.resize(resultCount);

        Value result = vm->makeArray();
        ArrayInstance *arr = result.asArray();
        arr->values.push(makeUint32Buffer(vm, simplified.data(), simplified.size()));
        arr->values.push(vm->makeFloat(resultError));
        arr->values.push(vm->makeInt((int)resultCount));
        vm->push(result);
        return 1;
    }

    // =====================================================
    // meshopt_simplifySloppy(indices, indexCount, vertexPositions, vertexCount, vertexPositionsStride, targetIndexCount, targetError)
    // Returns: [simplifiedIndices (Buffer UINT32), resultError (float)]
    // =====================================================
    static int native_meshopt_simplifySloppy(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 7)
        {
            Error("meshopt_simplifySloppy expects (indices, indexCount, vertexPositions, vertexCount, vertexPositionsStride, targetIndexCount, targetError)");
            return 0;
        }

        std::vector<unsigned int> indices;
        if (!extractIndices(vm, args[0], indices, "meshopt_simplifySloppy"))
            return 0;

        size_t indexCount = (size_t)args[1].asNumber();
        size_t vertexCount = (size_t)args[3].asNumber();
        size_t vertexPositionsStride = (size_t)args[4].asNumber();
        size_t targetIndexCount = (size_t)args[5].asNumber();
        float targetError = (float)args[6].asNumber();

        if (indexCount > indices.size())
            indexCount = indices.size();

        if (!args[2].isBuffer())
        {
            Error("meshopt_simplifySloppy: vertexPositions must be a buffer");
            return 0;
        }
        BufferInstance *posBuf = args[2].asBuffer();
        if (!posBuf || !posBuf->data)
        {
            Error("meshopt_simplifySloppy: invalid vertex positions buffer");
            return 0;
        }

        std::vector<unsigned int> simplified(indexCount);
        float resultError = 0.0f;

        size_t resultCount = meshopt_simplifySloppy(
            simplified.data(), indices.data(), indexCount,
            (const float *)posBuf->data, vertexCount, vertexPositionsStride,
            targetIndexCount, targetError, &resultError);

        simplified.resize(resultCount);

        Value result = vm->makeArray();
        ArrayInstance *arr = result.asArray();
        arr->values.push(makeUint32Buffer(vm, simplified.data(), simplified.size()));
        arr->values.push(vm->makeFloat(resultError));
        arr->values.push(vm->makeInt((int)resultCount));
        vm->push(result);
        return 1;
    }

    // =====================================================
    // meshopt_stripify(indices, indexCount, vertexCount, restartIndex)
    // Returns: strip index buffer (Buffer UINT32)
    // =====================================================
    static int native_meshopt_stripify(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("meshopt_stripify expects (indices, indexCount, vertexCount, restartIndex)");
            return 0;
        }

        std::vector<unsigned int> indices;
        if (!extractIndices(vm, args[0], indices, "meshopt_stripify"))
            return 0;

        size_t indexCount = (size_t)args[1].asNumber();
        size_t vertexCount = (size_t)args[2].asNumber();
        unsigned int restartIndex = (unsigned int)args[3].asNumber();

        if (indexCount > indices.size())
            indexCount = indices.size();

        // Worst case: meshopt_stripifyBound
        size_t maxStripSize = meshopt_stripifyBound(indexCount);
        std::vector<unsigned int> strip(maxStripSize);

        size_t stripSize = meshopt_stripify(
            strip.data(), indices.data(), indexCount, vertexCount, restartIndex);

        strip.resize(stripSize);
        vm->push(makeUint32Buffer(vm, strip.data(), strip.size()));
        return 1;
    }

    // =====================================================
    // meshopt_unstripify(indices, indexCount, restartIndex)
    // Returns: triangle index buffer (Buffer UINT32)
    // =====================================================
    static int native_meshopt_unstripify(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("meshopt_unstripify expects (indices, indexCount, restartIndex)");
            return 0;
        }

        std::vector<unsigned int> indices;
        if (!extractIndices(vm, args[0], indices, "meshopt_unstripify"))
            return 0;

        size_t indexCount = (size_t)args[1].asNumber();
        unsigned int restartIndex = (unsigned int)args[2].asNumber();

        if (indexCount > indices.size())
            indexCount = indices.size();

        size_t maxTriSize = meshopt_unstripifyBound(indexCount);
        std::vector<unsigned int> tris(maxTriSize);

        size_t triSize = meshopt_unstripify(
            tris.data(), indices.data(), indexCount, restartIndex);

        tris.resize(triSize);
        vm->push(makeUint32Buffer(vm, tris.data(), tris.size()));
        return 1;
    }

    // =====================================================
    // meshopt_analyzeVertexCache(indices, indexCount, vertexCount, cacheSize, warpSize, primgroupSize)
    // Returns: map {ACMR, ATVR}
    // =====================================================
    static int native_meshopt_analyzeVertexCache(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 6)
        {
            Error("meshopt_analyzeVertexCache expects (indices, indexCount, vertexCount, cacheSize, warpSize, primgroupSize)");
            return 0;
        }

        std::vector<unsigned int> indices;
        if (!extractIndices(vm, args[0], indices, "meshopt_analyzeVertexCache"))
            return 0;

        size_t indexCount = (size_t)args[1].asNumber();
        size_t vertexCount = (size_t)args[2].asNumber();
        unsigned int cacheSize = (unsigned int)args[3].asNumber();
        unsigned int warpSize = (unsigned int)args[4].asNumber();
        unsigned int primgroupSize = (unsigned int)args[5].asNumber();

        if (indexCount > indices.size())
            indexCount = indices.size();

        meshopt_VertexCacheStatistics stats = meshopt_analyzeVertexCache(
            indices.data(), indexCount, vertexCount, cacheSize, warpSize, primgroupSize);

        Value result = vm->makeArray();
        ArrayInstance *arr = result.asArray();
        // [ACMR, ATVR, verticesTrans, warpsExecuted, triangles]
        arr->values.push(vm->makeFloat(stats.acmr));
        arr->values.push(vm->makeFloat(stats.atvr));
        arr->values.push(vm->makeInt((int)stats.vertices_transformed));
        arr->values.push(vm->makeInt((int)stats.warps_executed));
        vm->push(result);
        return 1;
    }

    // =====================================================
    // meshopt_analyzeOverdraw(indices, indexCount, vertexPositions, vertexCount, vertexPositionsStride)
    // Returns: [overdraw, pixelsShaded, pixelsCovered]
    // =====================================================
    static int native_meshopt_analyzeOverdraw(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 5)
        {
            Error("meshopt_analyzeOverdraw expects (indices, indexCount, vertexPositions, vertexCount, vertexPositionsStride)");
            return 0;
        }

        std::vector<unsigned int> indices;
        if (!extractIndices(vm, args[0], indices, "meshopt_analyzeOverdraw"))
            return 0;

        size_t indexCount = (size_t)args[1].asNumber();
        size_t vertexCount = (size_t)args[3].asNumber();
        size_t vertexPositionsStride = (size_t)args[4].asNumber();

        if (indexCount > indices.size())
            indexCount = indices.size();

        if (!args[2].isBuffer())
        {
            Error("meshopt_analyzeOverdraw: vertexPositions must be a buffer");
            return 0;
        }
        BufferInstance *posBuf = args[2].asBuffer();
        if (!posBuf || !posBuf->data)
        {
            Error("meshopt_analyzeOverdraw: invalid vertex positions buffer");
            return 0;
        }

        meshopt_OverdrawStatistics stats = meshopt_analyzeOverdraw(
            indices.data(), indexCount,
            (const float *)posBuf->data, vertexCount, vertexPositionsStride);

        Value result = vm->makeArray();
        ArrayInstance *arr = result.asArray();
        arr->values.push(vm->makeFloat(stats.overdraw));
        arr->values.push(vm->makeInt((int)stats.pixels_shaded));
        arr->values.push(vm->makeInt((int)stats.pixels_covered));
        vm->push(result);
        return 1;
    }

    // =====================================================
    // meshopt_analyzeVertexFetch(indices, indexCount, vertexCount, vertexSize)
    // Returns: [overfetch, bytesTotal]
    // =====================================================
    static int native_meshopt_analyzeVertexFetch(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("meshopt_analyzeVertexFetch expects (indices, indexCount, vertexCount, vertexSize)");
            return 0;
        }

        std::vector<unsigned int> indices;
        if (!extractIndices(vm, args[0], indices, "meshopt_analyzeVertexFetch"))
            return 0;

        size_t indexCount = (size_t)args[1].asNumber();
        size_t vertexCount = (size_t)args[2].asNumber();
        size_t vertexSize = (size_t)args[3].asNumber();

        if (indexCount > indices.size())
            indexCount = indices.size();

        meshopt_VertexFetchStatistics stats = meshopt_analyzeVertexFetch(
            indices.data(), indexCount, vertexCount, vertexSize);

        Value result = vm->makeArray();
        ArrayInstance *arr = result.asArray();
        arr->values.push(vm->makeFloat(stats.overfetch));
        arr->values.push(vm->makeInt((int)stats.bytes_fetched));
        vm->push(result);
        return 1;
    }

    // =====================================================
    // meshopt_buildMeshlets(indices, indexCount, vertexPositions, vertexCount, vertexPositionsStride, maxVertices, maxTriangles, coneWeight)
    // Returns: array of meshlets, each is [vertexOffset, triangleOffset, vertexCount, triangleCount]
    // =====================================================
    static int native_meshopt_buildMeshlets(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 8)
        {
            Error("meshopt_buildMeshlets expects (indices, indexCount, vertexPositions, vertexCount, vertexPositionsStride, maxVertices, maxTriangles, coneWeight)");
            return 0;
        }

        std::vector<unsigned int> indices;
        if (!extractIndices(vm, args[0], indices, "meshopt_buildMeshlets"))
            return 0;

        size_t indexCount = (size_t)args[1].asNumber();
        size_t vertexCount = (size_t)args[3].asNumber();
        size_t vertexPositionsStride = (size_t)args[4].asNumber();
        size_t maxVertices = (size_t)args[5].asNumber();
        size_t maxTriangles = (size_t)args[6].asNumber();
        float coneWeight = (float)args[7].asNumber();

        if (indexCount > indices.size())
            indexCount = indices.size();

        if (!args[2].isBuffer())
        {
            Error("meshopt_buildMeshlets: vertexPositions must be a buffer");
            return 0;
        }
        BufferInstance *posBuf = args[2].asBuffer();
        if (!posBuf || !posBuf->data)
        {
            Error("meshopt_buildMeshlets: invalid vertex positions buffer");
            return 0;
        }

        size_t maxMeshlets = meshopt_buildMeshletsBound(indexCount, maxVertices, maxTriangles);
        std::vector<meshopt_Meshlet> meshlets(maxMeshlets);
        std::vector<unsigned int> meshletVertices(maxMeshlets * maxVertices);
        std::vector<unsigned char> meshletTriangles(maxMeshlets * maxTriangles * 3);

        size_t meshletCount = meshopt_buildMeshlets(
            meshlets.data(), meshletVertices.data(), meshletTriangles.data(),
            indices.data(), indexCount,
            (const float *)posBuf->data, vertexCount, vertexPositionsStride,
            maxVertices, maxTriangles, coneWeight);

        meshlets.resize(meshletCount);

        // Return array of meshlets: each meshlet = [vertexOffset, triangleOffset, vertexCount, triangleCount]
        Value result = vm->makeArray();
        ArrayInstance *arr = result.asArray();
        for (size_t i = 0; i < meshletCount; i++)
        {
            Value m = vm->makeArray();
            ArrayInstance *marr = m.asArray();
            marr->values.push(vm->makeInt((int)meshlets[i].vertex_offset));
            marr->values.push(vm->makeInt((int)meshlets[i].triangle_offset));
            marr->values.push(vm->makeInt((int)meshlets[i].vertex_count));
            marr->values.push(vm->makeInt((int)meshlets[i].triangle_count));
            arr->values.push(m);
        }
        vm->push(result);
        return 1;
    }

    // =====================================================
    // meshopt_spatialSortRemap(vertexPositions, vertexCount, vertexPositionsStride)
    // Returns: remap table (Buffer UINT32)
    // =====================================================
    static int native_meshopt_spatialSortRemap(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("meshopt_spatialSortRemap expects (vertexPositions, vertexCount, vertexPositionsStride)");
            return 0;
        }

        if (!args[0].isBuffer())
        {
            Error("meshopt_spatialSortRemap: vertexPositions must be a buffer");
            return 0;
        }
        BufferInstance *posBuf = args[0].asBuffer();
        if (!posBuf || !posBuf->data)
        {
            Error("meshopt_spatialSortRemap: invalid vertex positions buffer");
            return 0;
        }

        size_t vertexCount = (size_t)args[1].asNumber();
        size_t vertexPositionsStride = (size_t)args[2].asNumber();

        std::vector<unsigned int> remap(vertexCount);
        meshopt_spatialSortRemap(remap.data(),
                                 (const float *)posBuf->data, vertexCount,
                                 vertexPositionsStride);

        vm->push(makeUint32Buffer(vm, remap.data(), remap.size()));
        return 1;
    }

    // =====================================================
    // REGISTRATION
    // =====================================================
    void registerAll(Interpreter &vm)
    {
        ModuleBuilder module = vm.addModule("MeshOpt");

        module
            // Optimization
            .addFunction("generateVertexRemap", native_meshopt_generateVertexRemap, 3)
            .addFunction("optimizeVertexCache", native_meshopt_optimizeVertexCache, 3)
            .addFunction("optimizeVertexCacheStrip", native_meshopt_optimizeVertexCacheStrip, 3)
            .addFunction("optimizeOverdraw", native_meshopt_optimizeOverdraw, 6)
            .addFunction("optimizeVertexFetch", native_meshopt_optimizeVertexFetch, 5)

            // Simplification
            .addFunction("simplify", native_meshopt_simplify, -1)
            .addFunction("simplifySloppy", native_meshopt_simplifySloppy, 7)

            // Strip generation
            .addFunction("stripify", native_meshopt_stripify, 4)
            .addFunction("unstripify", native_meshopt_unstripify, 3)

            // Analysis
            .addFunction("analyzeVertexCache", native_meshopt_analyzeVertexCache, 6)
            .addFunction("analyzeOverdraw", native_meshopt_analyzeOverdraw, 5)
            .addFunction("analyzeVertexFetch", native_meshopt_analyzeVertexFetch, 4)

            // Meshlets (GPU-driven rendering)
            .addFunction("buildMeshlets", native_meshopt_buildMeshlets, 8)

            // Spatial sorting
            .addFunction("spatialSortRemap", native_meshopt_spatialSortRemap, 3)

            // Simplify options flags
            .addInt("SIMPLIFY_LOCK_BORDER", meshopt_SimplifyLockBorder)
            .addInt("SIMPLIFY_SPARSE", meshopt_SimplifySparse)
            .addInt("SIMPLIFY_ERROR_ABSOLUTE", meshopt_SimplifyErrorAbsolute);
    }

    void cleanup()
    {
        // meshoptimizer is stateless, nothing to clean up
    }
}
