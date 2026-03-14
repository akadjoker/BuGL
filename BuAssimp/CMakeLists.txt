project(BuAssimp)
cmake_policy(SET CMP0072 NEW)

set(CMAKE_CXX_STANDARD 17)

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins)

set(BINDING_SOURCES
    src/assimp_core.cpp
    src/assimp_scene.cpp
    src/assimp_mesh.cpp
    src/assimp_material.cpp
    src/assimp_node.cpp
    src/assimp_bindings.cpp
)

add_library(bu_assimp SHARED
    src/plugin.cpp
    ${BINDING_SOURCES}
)

target_include_directories(bu_assimp PRIVATE
    src
    ${CMAKE_SOURCE_DIR}/libbu/include
    ${CMAKE_SOURCE_DIR}/main/src
    ${CMAKE_SOURCE_DIR}/vendor/assimp/include
    ${CMAKE_BINARY_DIR}/vendor/assimp/include   # generated config.h / revision.h
)

target_compile_definitions(bu_assimp PRIVATE BU_PLUGIN_BUILD)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    if(UNIX)
        target_compile_options(bu_assimp PRIVATE -g -O1 -fPIC -fno-omit-frame-pointer -D_DEBUG)
    endif()
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(bu_assimp PRIVATE -O3 -DNDEBUG -fPIC)
    if(BUGL_STRIP_RELEASE AND NOT MSVC)
        target_link_options(bu_assimp PRIVATE -s)
    endif()
endif()

if(UNIX)
    set_target_properties(bu_assimp PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

set_target_properties(bu_assimp PROPERTIES
    OUTPUT_NAME "bu_assimp"
    PREFIX "lib"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins
)

target_link_libraries(bu_assimp PRIVATE libbu assimp)
