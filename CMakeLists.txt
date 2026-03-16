cmake_minimum_required(VERSION 3.20)
project(BuDebugHost VERSION 2.0.0)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUGL_PORTABLE_RELEASE "Build release binaries without CPU-specific native flags" ON)
option(BUGL_STRIP_RELEASE "Strip symbols from release binaries when supported" ON)
option(BUGL_ENABLE_COMPILER_CACHE "Use compiler cache launcher (sccache/ccache) when available" ON)

if(BUGL_ENABLE_COMPILER_CACHE)
    if(NOT CMAKE_C_COMPILER_LAUNCHER AND NOT CMAKE_CXX_COMPILER_LAUNCHER)
        find_program(BUGL_COMPILER_CACHE_PROGRAM NAMES sccache ccache)
        if(BUGL_COMPILER_CACHE_PROGRAM)
            message(STATUS "BuGL: enabling compiler cache launcher: ${BUGL_COMPILER_CACHE_PROGRAM}")
            set(CMAKE_C_COMPILER_LAUNCHER "${BUGL_COMPILER_CACHE_PROGRAM}" CACHE STRING "C compiler launcher" FORCE)
            set(CMAKE_CXX_COMPILER_LAUNCHER "${BUGL_COMPILER_CACHE_PROGRAM}" CACHE STRING "CXX compiler launcher" FORCE)
        else()
            message(STATUS "BuGL: compiler cache launcher not found (sccache/ccache)")
        endif()
    endif()
endif()

add_subdirectory(vendor/miniz)
if(TARGET miniz)
    set_target_properties(miniz PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

add_subdirectory(libbu)
add_subdirectory(BuDebugHost)
