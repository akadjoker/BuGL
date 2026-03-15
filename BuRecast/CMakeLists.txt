project(BuRecast)
cmake_policy(SET CMP0072 NEW)

set(CMAKE_CXX_STANDARD 17)

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins)

set(RECAST_ROOT ${CMAKE_SOURCE_DIR}/vendor/recastnavigation)

set(RECAST_SOURCES
    ${RECAST_ROOT}/Recast/Source/Recast.cpp
    ${RECAST_ROOT}/Recast/Source/RecastAlloc.cpp
    ${RECAST_ROOT}/Recast/Source/RecastArea.cpp
    ${RECAST_ROOT}/Recast/Source/RecastAssert.cpp
    ${RECAST_ROOT}/Recast/Source/RecastContour.cpp
    ${RECAST_ROOT}/Recast/Source/RecastFilter.cpp
    ${RECAST_ROOT}/Recast/Source/RecastLayers.cpp
    ${RECAST_ROOT}/Recast/Source/RecastMesh.cpp
    ${RECAST_ROOT}/Recast/Source/RecastMeshDetail.cpp
    ${RECAST_ROOT}/Recast/Source/RecastRasterization.cpp
    ${RECAST_ROOT}/Recast/Source/RecastRegion.cpp
)

set(DETOUR_SOURCES
    ${RECAST_ROOT}/Detour/Source/DetourAlloc.cpp
    ${RECAST_ROOT}/Detour/Source/DetourAssert.cpp
    ${RECAST_ROOT}/Detour/Source/DetourCommon.cpp
    ${RECAST_ROOT}/Detour/Source/DetourNavMesh.cpp
    ${RECAST_ROOT}/Detour/Source/DetourNavMeshBuilder.cpp
    ${RECAST_ROOT}/Detour/Source/DetourNavMeshQuery.cpp
    ${RECAST_ROOT}/Detour/Source/DetourNode.cpp
)

set(DETOUR_CROWD_SOURCES
    ${RECAST_ROOT}/DetourCrowd/Source/DetourCrowd.cpp
    ${RECAST_ROOT}/DetourCrowd/Source/DetourLocalBoundary.cpp
    ${RECAST_ROOT}/DetourCrowd/Source/DetourObstacleAvoidance.cpp
    ${RECAST_ROOT}/DetourCrowd/Source/DetourPathCorridor.cpp
    ${RECAST_ROOT}/DetourCrowd/Source/DetourPathQueue.cpp
    ${RECAST_ROOT}/DetourCrowd/Source/DetourProximityGrid.cpp
)

set(DETOUR_TILECACHE_SOURCES
    ${RECAST_ROOT}/DetourTileCache/Source/DetourTileCache.cpp
    ${RECAST_ROOT}/DetourTileCache/Source/DetourTileCacheBuilder.cpp
)

set(BINDING_SOURCES
    src/recast_core.cpp
    src/recast_navmesh.cpp
    src/recast_crowd.cpp
    src/recast_tilecache.cpp
    src/recast_bindings.cpp
)

add_library(bu_recast SHARED
    src/plugin.cpp
    ${BINDING_SOURCES}
    ${RECAST_SOURCES}
    ${DETOUR_SOURCES}
    ${DETOUR_CROWD_SOURCES}
    ${DETOUR_TILECACHE_SOURCES}
)

target_include_directories(bu_recast PRIVATE
    src
    ${CMAKE_SOURCE_DIR}/libbu/include
    ${CMAKE_SOURCE_DIR}/main/src
    ${RECAST_ROOT}/Recast/Include
    ${RECAST_ROOT}/Detour/Include
    ${RECAST_ROOT}/DetourCrowd/Include
    ${RECAST_ROOT}/DetourTileCache/Include
)

target_compile_definitions(bu_recast PRIVATE BU_PLUGIN_BUILD)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    if(UNIX)
        target_compile_options(bu_recast PRIVATE -g -O1 -fPIC -fno-omit-frame-pointer -D_DEBUG)
    endif()
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(bu_recast PRIVATE -O3 -DNDEBUG -fPIC)
    if(BUGL_STRIP_RELEASE AND NOT MSVC)
        target_link_options(bu_recast PRIVATE -s)
    endif()
endif()

if(UNIX)
    set_target_properties(bu_recast PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

set_target_properties(bu_recast PROPERTIES
    OUTPUT_NAME "bu_recast"
    PREFIX "lib"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins
)

if(WIN32)
    target_link_libraries(bu_recast PRIVATE libbu)
else()
    target_link_libraries(bu_recast PRIVATE libbu)
endif()
