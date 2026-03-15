project(BuEnTT)
cmake_policy(SET CMP0072 NEW)

set(CMAKE_CXX_STANDARD 17)

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins)

set(BINDING_SOURCES
    src/entt_core.cpp
    src/entt_bindings.cpp
    src/entt_world.cpp
    src/entt_entity.cpp
)

add_library(bu_entt SHARED
    src/plugin.cpp
    ${BINDING_SOURCES}
)

target_include_directories(bu_entt PRIVATE
    src
    ${CMAKE_SOURCE_DIR}/libbu/include
    ${CMAKE_SOURCE_DIR}/vendor/entt/src
)

target_compile_definitions(bu_entt PRIVATE BU_PLUGIN_BUILD)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    if(UNIX)
        target_compile_options(bu_entt PRIVATE
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
        target_link_options(bu_entt PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
            -g
        )
    endif()
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(bu_entt PRIVATE -O3 -DNDEBUG -fPIC)
    if(BUGL_STRIP_RELEASE AND NOT MSVC)
        target_link_options(bu_entt PRIVATE -s)
    endif()
endif()

if(UNIX)
    set_target_properties(bu_entt PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

set_target_properties(bu_entt PROPERTIES
    OUTPUT_NAME "bu_entt"
    PREFIX "lib"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins
)

if(WIN32)
    target_link_libraries(bu_entt PRIVATE libbu)
else()
    target_link_libraries(bu_entt PRIVATE libbu m dl)
endif()
