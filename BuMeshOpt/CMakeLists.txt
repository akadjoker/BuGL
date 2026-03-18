project(BuMeshOpt)
cmake_policy(SET CMP0072 NEW)

set(CMAKE_CXX_STANDARD 17)

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins)

# meshoptimizer source files (compiled directly into the plugin)
set(MESHOPT_SOURCES
    ${CMAKE_SOURCE_DIR}/vendor/meshoptimizer/src/allocator.cpp
    ${CMAKE_SOURCE_DIR}/vendor/meshoptimizer/src/clusterizer.cpp
    ${CMAKE_SOURCE_DIR}/vendor/meshoptimizer/src/indexgenerator.cpp
    ${CMAKE_SOURCE_DIR}/vendor/meshoptimizer/src/overdrawoptimizer.cpp
    ${CMAKE_SOURCE_DIR}/vendor/meshoptimizer/src/simplifier.cpp
    ${CMAKE_SOURCE_DIR}/vendor/meshoptimizer/src/spatialorder.cpp
    ${CMAKE_SOURCE_DIR}/vendor/meshoptimizer/src/stripifier.cpp
    ${CMAKE_SOURCE_DIR}/vendor/meshoptimizer/src/vcacheoptimizer.cpp
    ${CMAKE_SOURCE_DIR}/vendor/meshoptimizer/src/vfetchoptimizer.cpp
    ${CMAKE_SOURCE_DIR}/vendor/meshoptimizer/src/indexanalyzer.cpp
    ${CMAKE_SOURCE_DIR}/vendor/meshoptimizer/src/quantization.cpp
)

set(BINDING_SOURCES
    src/meshopt_bindings.cpp
)

add_library(bu_meshopt SHARED
    src/plugin.cpp
    ${BINDING_SOURCES}
    ${MESHOPT_SOURCES}
)

target_include_directories(bu_meshopt PRIVATE
    src
    ${CMAKE_SOURCE_DIR}/libbu/include
    ${CMAKE_SOURCE_DIR}/vendor/meshoptimizer/src
)

target_compile_definitions(bu_meshopt PRIVATE BU_PLUGIN_BUILD)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    if(UNIX)
        target_compile_options(bu_meshopt PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
            -g
            -O1
            -fno-omit-frame-pointer
            -D_DEBUG
            -DVERBOSE
            -fPIC
        )
        target_link_options(bu_meshopt PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
            -g
        )
    endif()
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(bu_meshopt PRIVATE -O3 -DNDEBUG -fPIC)
    if(BUGL_STRIP_RELEASE AND NOT MSVC)
        target_link_options(bu_meshopt PRIVATE -s)
    endif()
endif()

if(UNIX)
    set_target_properties(bu_meshopt PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

set_target_properties(bu_meshopt PROPERTIES
    OUTPUT_NAME "bu_meshopt"
    PREFIX "lib"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins
)

if(WIN32)
    target_link_libraries(bu_meshopt PRIVATE libbu)
else()
    target_link_libraries(bu_meshopt PRIVATE libbu m dl)
endif()
