project(BuBox2d)
cmake_policy(SET CMP0072 NEW)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")

if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}   -D_CRT_SECURE_NO_WARNINGS")
    if (MSVC)
        if(CMAKE_BUILD_TYPE MATCHES Debug)
            add_compile_options(/RTC1 /Od /Zi)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
        endif()
    endif()
endif()

 
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins)
 

 
set(BINDING_SOURCES

    src/box2d.cpp
    src/box2d_joints.cpp
)

 

 

# ============================================
# libbu_box2d Plugin (shared library)
# ============================================
add_library(bu_box2d SHARED src/plugin.cpp ${BINDING_SOURCES}  )

target_include_directories(bu_box2d PRIVATE include src ${CMAKE_SOURCE_DIR}/libbu/include )

# Plugin-specific compile definitions
target_compile_definitions(bu_box2d PRIVATE BU_PLUGIN_BUILD)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    if (UNIX)
        target_compile_options(bu_box2d PRIVATE
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
        target_link_options(bu_box2d PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
            -g
        )
    endif()
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(bu_box2d PRIVATE -O3 -DNDEBUG -fPIC)
    if (BUGL_STRIP_RELEASE AND NOT MSVC)
        target_link_options(bu_box2d PRIVATE -s)
    endif()
endif()

# On Unix, shared libraries need -fPIC
if (UNIX)
    set_target_properties(bu_box2d PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# Set output name to libbu_box2d.so (or .dll on Windows)
set_target_properties(bu_box2d PROPERTIES
    OUTPUT_NAME "bu_box2d"
    PREFIX "lib"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins
)

# Link against the vendored Box2D target so its headers and symbols are available.
if (WIN32)
    # On Windows, we need to link libbu since we can't use -rdynamic
    if (MSVC)
        target_link_libraries(bu_box2d PRIVATE box2d libbu Winmm.lib)
    else()
        target_link_libraries(bu_box2d PRIVATE box2d libbu winmm)
    endif()
else()
    # On Unix, symbols will be resolved from the main executable
    target_link_libraries(bu_box2d PRIVATE box2d m dl)
endif()
