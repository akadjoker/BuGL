project(BuJolt)
cmake_policy(SET CMP0072 NEW)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")

if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
    if (MSVC)
        if(CMAKE_BUILD_TYPE MATCHES Debug)
            add_compile_options(/RTC1 /Od /Zi)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
        endif()
    endif()
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins)

set(BINDING_SOURCES
    src/jolt_core.cpp
    src/jolt_bindings.cpp
    src/jolt_world.cpp
    src/jolt_body.cpp
    src/jolt_joints.cpp
    src/jolt_vehicle.cpp
)

add_library(bu_jolt SHARED src/plugin.cpp ${BINDING_SOURCES})

target_include_directories(bu_jolt PRIVATE include src ${CMAKE_SOURCE_DIR}/libbu/include)

target_compile_definitions(bu_jolt PRIVATE BU_PLUGIN_BUILD)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    if (UNIX)
        target_compile_options(bu_jolt PRIVATE
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
        target_link_options(bu_jolt PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
            -g
        )
    endif()
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(bu_jolt PRIVATE -O3 -DNDEBUG -fPIC)
    if (BUGL_STRIP_RELEASE AND NOT MSVC)
        target_link_options(bu_jolt PRIVATE -s)
    endif()
endif()

if (UNIX)
    set_target_properties(bu_jolt PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

set_target_properties(bu_jolt PROPERTIES
    OUTPUT_NAME "bu_jolt"
    PREFIX "lib"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins
)

if (WIN32)
    if (MSVC)
        target_link_libraries(bu_jolt PRIVATE Jolt libbu Winmm.lib)
    else()
        target_link_libraries(bu_jolt PRIVATE Jolt libbu winmm)
    endif()
else()
    target_link_libraries(bu_jolt PRIVATE Jolt m dl)
endif()
